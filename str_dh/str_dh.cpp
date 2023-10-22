#include "str_dh.hpp"
#include "MODP2048_256sg.hpp"

#include <random>
#include <cryptopp/nbtheory.h>
#include <cryptopp/oids.h>
#include <cryptopp/asn.h>

#define UNINITIALIZED_ADDRESS "0.0.0.0"

str_dh::str_dh(bool _is_sponsor, service_id_t _service_id, std::uint32_t _member_count, std::uint32_t _scatter_delay_min, std::uint32_t _scatter_delay_max) : is_sponsor_(_is_sponsor), request_scheduled_(false), response_scheduled_(false), higher_member_id_synching_(false), higher_member_id_assigned_(false), synch_token_rcvd_(false), synch_finished_(false), last_member_synch_token_sending_triggered_(false), finish_message_rcvd_(false), service_of_interest_(_service_id), member_count_(_member_count), message_handler_(std::make_unique<message_handler>(this)), statistics_recorder_(statistics_recorder::get_instance()), scatter_timer_(multicast_application_impl::get_io_service()), timeout_timer_(multicast_application_impl::get_io_service()) {
    scatter_delay_ = compute_scatter_delay(_scatter_delay_min, _scatter_delay_max);
    scatter_timer_.expires_from_now(scatter_delay_);
#ifdef DEFAULT_DH
    diffie_hellman_.AccessGroupParameters().Initialize(P, Q, G);
    LOG_DEBUG("[<str_dh>]: Using default DH")
#elif defined(ECC_DH)
    diffie_hellman_.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());
    LOG_DEBUG("[<str_dh>]: Using ECDH")
#endif
    secret_.New(diffie_hellman_.PrivateKeyLength());
    blinded_secret_.New(diffie_hellman_.PublicKeyLength());
    diffie_hellman_.GeneratePrivateKey(rng_, secret_); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);
    diffie_hellman_.GeneratePublicKey(rng_, secret_, blinded_secret_); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);

    str_key_tree_map_.clear();
    pending_requests_.clear();
    assigned_member_key_map_.clear();
    assigned_member_endpoint_map_.clear();

    statistics_recorder_->record_count(count_metric::MEMBER_COUNT_);
    if (is_sponsor_) {
        statistics_recorder_->record_timestamp(time_metric::DURATION_START_);
        member_id_ = INITIAL_SPONSOR_ID;
        keys_computed_count_ = 1;
        str_key_tree_map_[service_of_interest_] = build_str_tree(secret_, blinded_secret_, secret_, blinded_secret_);
        std::unique_ptr<offer_message> initial_offer = std::make_unique<offer_message>();
        initial_offer->offered_service_ = service_of_interest_;
        send(initial_offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);
        send_cyclic_offer();
    } else {
        keys_computed_count_ = 0;
        std::unique_ptr<find_message> initial_find = std::make_unique<find_message>();
        initial_find->required_service_ = service_of_interest_;
        send(initial_find.operator*()); statistics_recorder_->record_count(count_metric::FIND_MESSAGE_COUNT_);
    }
}

str_dh::~str_dh() {

}

void str_dh::received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) {
    std::lock_guard<std::mutex> lock_receive(receive_mutex_);
    if (get_local_endpoint().port() != _remote_endpoint.port()) {
        message_handler_->deserialize_and_callback(_data, _bytes_recvd, _remote_endpoint);
    }
}

void str_dh::process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (is_sponsor_ && service_of_interest_ == _rcvd_find_message.required_service_) {
        std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
        offer->offered_service_ = service_of_interest_;
        send(offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);
    }
}

void str_dh::process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    check_if_higher_member_id_assigned(_remote_endpoint);
    if (!is_assigned() && _rcvd_offer_message.offered_service_ == service_of_interest_ && !request_scheduled_) {
        request_scheduled_ = !request_scheduled_;
        scatter_timer_.expires_from_now(scatter_delay_);
        scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
            if (!_error) {
                std::unique_ptr<request_message> request = std::make_unique<request_message>();
                request->blinded_secret_ = blinded_secret_;
                request->required_service_ = service_of_interest_;
                send(request.operator*()); statistics_recorder_->record_count(count_metric::REQUEST_MESSAGE_COUNT_);
            }
            request_scheduled_ = !request_scheduled_;
        });
    }
}

void str_dh::process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!assigned_member_endpoint_map_[_rcvd_request_message.required_service_].contains(_remote_endpoint)
        && !pending_requests_[_rcvd_request_message.required_service_].contains(_remote_endpoint)) {
        pending_requests_[_rcvd_request_message.required_service_][_remote_endpoint] = _rcvd_request_message.blinded_secret_;
    }

    if(member_id_ == INITIAL_SPONSOR_ID && is_sponsor_) {
        statistics_recorder_->record_timestamp(time_metric::KEY_AGREEMENT_START_);
    }
    process_pending_request();
}

void str_dh::process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    boost::asio::ip::udp::endpoint new_sponsor_endpoint(_rcvd_response_message.new_sponsor.ip_address_, _rcvd_response_message.new_sponsor.port_);
    // Add new assigned sponsor
    if (new_sponsor_endpoint != get_local_endpoint() && !assigned_member_endpoint_map_[_rcvd_response_message.offered_service_].contains(new_sponsor_endpoint)) {
        assigned_member_key_map_[_rcvd_response_message.offered_service_][_rcvd_response_message.new_sponsor.assigned_id_] = _rcvd_response_message.new_sponsor.blinded_secret_;
        assigned_member_endpoint_map_[_rcvd_response_message.offered_service_][new_sponsor_endpoint] = _rcvd_response_message.new_sponsor.assigned_id_;
    }
    // Add old assigned sponsor
    if (!assigned_member_endpoint_map_[_rcvd_response_message.offered_service_].contains(_remote_endpoint)) {
        assigned_member_key_map_[_rcvd_response_message.offered_service_][_rcvd_response_message.new_sponsor.assigned_id_-1] = _rcvd_response_message.blinded_sponsor_secret_;
        assigned_member_endpoint_map_[_rcvd_response_message.offered_service_][_remote_endpoint] = _rcvd_response_message.new_sponsor.assigned_id_-1;
    }
    pending_requests_[_rcvd_response_message.offered_service_].erase(new_sponsor_endpoint);
    pending_requests_[_rcvd_response_message.offered_service_].erase(_remote_endpoint);

    bool become_sponsor = get_local_endpoint() == new_sponsor_endpoint;
    if (!is_assigned() && become_sponsor && _rcvd_response_message.offered_service_ == service_of_interest_) {
        is_sponsor_ = true;
        member_id_ = _rcvd_response_message.new_sponsor.assigned_id_;
        secret_t group_secret(diffie_hellman_.AgreedValueLength());
        diffie_hellman_.Agree(group_secret, secret_, _rcvd_response_message.blinded_group_secret_); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);
        blinded_secret_t blinded_group_secret(diffie_hellman_.PublicKeyLength());
        diffie_hellman_.GeneratePublicKey(rng_, group_secret, blinded_group_secret); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);
        std::unique_ptr<str_key_tree> str_tree = build_str_tree(group_secret,
                                                                blinded_group_secret,
                                                                secret_,
                                                                blinded_secret_);
        std::unique_ptr<str_key_tree> previous_str_tree = build_str_tree(DEFAULT_SECRET,
                                                                        _rcvd_response_message.blinded_group_secret_,
                                                                        DEFAULT_SECRET,
                                                                        _rcvd_response_message.blinded_sponsor_secret_);
        str_tree->next_internal_node_ = std::move(previous_str_tree);
        str_key_tree_map_[service_of_interest_] = std::move(str_tree);

        keys_computed_count_++;
    }

    if (is_last_member() && !last_member_synch_token_sending_triggered_) {
        last_member_synch_token_sending_triggered_ = true;
        is_sponsor_ = false;
        synch_finished_ = true;
        higher_member_id_assigned_ = true;
        send_synch_token_to_next_member();
        send_cyclic_synch_token();
        return;
    }
    check_and_add_next_blinded_key_to_group_secret();
    process_pending_request();
}

void str_dh::process_member_info_request(member_info_request_message _rcvd_member_info_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    check_if_higher_member_id_assigned(_remote_endpoint);
    process_member_info_request_<member_info_request_message, member_info_response_message>(_rcvd_member_info_request_message, _remote_endpoint);
}

void str_dh::process_member_info_response(member_info_response_message _rcvd_member_info_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    process_member_info_response_<member_info_response_message>(_rcvd_member_info_response_message, _remote_endpoint);
    if (is_sponsor_ && all_predecessors_known()) {
        process_pending_request();
    }
}

void str_dh::process_synch_token(synch_token_message _rcvd_synch_token_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    higher_member_id_assigned_ = true;
    if (!higher_member_id_synching_ && _rcvd_synch_token_message.member_id_ > (member_id_ % member_count_)) { higher_member_id_synching_ = true; }
    bool synch = !synch_token_rcvd_ && _rcvd_synch_token_message.member_id_ == member_id_ && !is_last_member();
    if (synch && !all_successors_known()) {
        synch_token_rcvd_ = true;
        send_member_info_synch_request_successors();
        send_cyclic_member_info_synch_request_successors();
    } else if (synch && all_successors_known()) {
        synch_token_rcvd_ = true;
        synch_finished_ = true;
        LOG_DEBUG("[<str_dh>]: member_id=" << member_id_ << ", Keys are calculated. group secret=" << short_secret_repr(str_key_tree_map_[service_of_interest_]->root_node_.group_secret_) << " Sending synch token to next member")
        send_synch_token_to_next_member();
        send_cyclic_synch_token();
    }
    if (_rcvd_synch_token_message.member_id_ == member_count_ && is_last_member() && !synch_token_rcvd_) {
        synch_token_rcvd_ = true;
        LOG_DEBUG("[<str_dh>]: member_id=" << member_id_ << ", Keys are calculated. group secret=" << short_secret_repr(str_key_tree_map_[service_of_interest_]->root_node_.group_secret_) << " Sending finish message")
        send_finish();
        send_cyclic_finish();
    }
}

void str_dh::process_member_info_synch_request(member_info_synch_request_message _rcvd_member_info_synch_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    higher_member_id_assigned_ = true;
    if (!higher_member_id_synching_ && assigned_member_endpoint_map_[service_of_interest_].count(_remote_endpoint) && assigned_member_endpoint_map_[service_of_interest_][_remote_endpoint] > (member_id_ % member_count_)) { higher_member_id_synching_ = true; }
    process_member_info_request_<member_info_synch_request_message, member_info_synch_response_message>(_rcvd_member_info_synch_request_message, _remote_endpoint);
}

void str_dh::process_member_info_synch_response(member_info_synch_response_message _rcvd_member_info_synch_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    higher_member_id_assigned_ = true;
    process_member_info_response_<member_info_synch_response_message>(_rcvd_member_info_synch_response_message, _remote_endpoint);
    if (all_successors_known() && !synch_finished_ && synch_token_rcvd_) {
        synch_finished_ = true;
        LOG_DEBUG("[<str_dh>]: member_id=" << member_id_ << ", Keys are calculated. group secret=" << short_secret_repr(str_key_tree_map_[service_of_interest_]->root_node_.group_secret_) << " Sending synch token to next member")
        send_synch_token_to_next_member();
        send_cyclic_synch_token();
    }
}

template<typename T, typename R> void str_dh::process_member_info_request_(T _rcvd_member_info_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (is_assigned() && !response_scheduled_ && _rcvd_member_info_request_message.required_service_ == service_of_interest_
        && std::find(_rcvd_member_info_request_message.requested_members_.begin(), _rcvd_member_info_request_message.requested_members_.end(), member_id_) != _rcvd_member_info_request_message.requested_members_.end()) {
        response_scheduled_ = !response_scheduled_;
        scatter_timer_.expires_from_now(scatter_delay_);
        scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
            if (!_error) {
                std::unique_ptr<R> member_info_resp_msg = std::make_unique<R>();
                member_info_resp_msg->offered_service_ = service_of_interest_;
                member_info_resp_msg->member_id_ = member_id_;
                member_info_resp_msg->blinded_secret_ = blinded_secret_;
                send(member_info_resp_msg.operator*()); statistics_recorder_->record_count(count_metric::MEMBER_INFO_RESPONSE_MESSAGE_COUNT_);
            }
            response_scheduled_ = !response_scheduled_;
        });
    }
}

template<typename T> void str_dh::process_member_info_response_(T _rcvd_member_info_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!assigned_member_endpoint_map_[_rcvd_member_info_response_message.offered_service_].contains(_remote_endpoint)) {
        assigned_member_key_map_[_rcvd_member_info_response_message.offered_service_][_rcvd_member_info_response_message.member_id_] = _rcvd_member_info_response_message.blinded_secret_;
        assigned_member_endpoint_map_[_rcvd_member_info_response_message.offered_service_][_remote_endpoint] = _rcvd_member_info_response_message.member_id_;
        pending_requests_[_rcvd_member_info_response_message.offered_service_].erase(_remote_endpoint);
        check_and_add_next_blinded_key_to_group_secret();
    }
}

void str_dh::process_finish(finish_message _rcvd_finish_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (member_id_ == INITIAL_SPONSOR_ID && !finish_message_rcvd_) {
        statistics_recorder_->record_timestamp(time_metric::DURATION_END_);
    }
    finish_message_rcvd_ = true;
    higher_member_id_assigned_ = true;
    higher_member_id_synching_ = true;
    if (member_id_ != INITIAL_SPONSOR_ID) {
        scatter_timer_.cancel();
        contribute_statistics();
    }

    if (member_id_ == INITIAL_SPONSOR_ID && assigned_member_endpoint_map_[service_of_interest_].count(_remote_endpoint) && assigned_member_endpoint_map_[service_of_interest_][_remote_endpoint] == member_count_) {
        std::unique_ptr<finish_ack_message> finish_ack = std::make_unique<finish_ack_message>();
        send(finish_ack.operator*()); statistics_recorder_->record_count(count_metric::FINISH_ACK_MESSAGE_COUNT_);
        timeout_timer_.expires_from_now(std::chrono::seconds(3));
        timeout_timer_.async_wait([this](const boost::system::error_code& _error) {
            if (!_error) {
                contribute_statistics();
            }
        });

        std::unique_ptr<finish_message> finish = std::make_unique<finish_message>();
        send(finish.operator*()); // Message is not counted, since its only for triggering other members to contribute statistics and shut down
        scatter_timer_.expires_from_now(std::chrono::milliseconds(scatter_delay_));
        scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
            if (!_error) {
                std::unique_ptr<finish_message> cyclic_finish = std::make_unique<finish_message>();
                send(cyclic_finish.operator*()); // Message is not counted, since its only for triggering other members to contribute statistics and shut down
                std::unique_ptr<finish_message> self_finish = std::make_unique<finish_message>();
                process_finish(self_finish.operator*(), get_local_endpoint());
            }
        });
    } else if (member_id_ == INITIAL_SPONSOR_ID && _remote_endpoint == get_local_endpoint()) {
        std::unique_ptr<finish_message> finish = std::make_unique<finish_message>();
        send(finish.operator*()); // Message is not counted, since its only for triggering other members to contribute statistics and shut down
        scatter_timer_.expires_from_now(std::chrono::milliseconds(scatter_delay_));
        scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
            if (!_error) {
                std::unique_ptr<finish_message> cyclic_finish = std::make_unique<finish_message>();
                send(cyclic_finish.operator*()); // Message is not counted, since its only for triggering other members to contribute statistics and shut down
                std::unique_ptr<finish_message> self_finish = std::make_unique<finish_message>();
                process_finish(self_finish.operator*(), get_local_endpoint());
            }
        });
    }
}

void str_dh::process_finish_ack(finish_ack_message _rcvd_finish_ack_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    higher_member_id_assigned_ = true;
    higher_member_id_synching_ = true;
    if (member_id_ != INITIAL_SPONSOR_ID) {
        scatter_timer_.cancel();
        contribute_statistics();
    }
}

void str_dh::process_pending_request() {
    if (!is_sponsor_) {
        return;
    }

    if (!all_predecessors_known()) {
        send_member_info_request_predecessors();
        send_cyclic_member_info_request_predecessors();
        return;
    }

    std::pair<boost::asio::ip::udp::endpoint, blinded_secret_t> unassigned_member = get_unassigned_member();
    boost::asio::ip::udp::endpoint pending_remote_endpoint = unassigned_member.first;
    blinded_secret_t pending_blinded_secret = unassigned_member.second;

    if (pending_remote_endpoint.address().to_string().compare(UNINITIALIZED_ADDRESS) != 0 && pending_blinded_secret.SizeInBytes() != 0) {
        std::unique_ptr<str_key_tree> previous_str_tree = std::move(str_key_tree_map_[service_of_interest_]);
        secret_t group_secret(diffie_hellman_.AgreedValueLength());
        diffie_hellman_.Agree(group_secret, previous_str_tree->root_node_.group_secret_, pending_blinded_secret); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);
        std::unique_ptr<str_key_tree> str_tree = build_str_tree(group_secret,
                                                                DEFAULT_SECRET,
                                                                DEFAULT_SECRET,
                                                                pending_blinded_secret);

        is_sponsor_ = false;
        std::unique_ptr<response_message> response = std::make_unique<response_message>();
        response->blinded_group_secret_ = previous_str_tree->root_node_.blinded_group_secret_;
        response->blinded_sponsor_secret_ = blinded_secret_;
        response->new_sponsor.assigned_id_ = member_id_+1;
        response->new_sponsor.ip_address_ = pending_remote_endpoint.address();
        response->new_sponsor.port_ = pending_remote_endpoint.port();
        response->new_sponsor.blinded_secret_ = pending_blinded_secret;
        response->offered_service_ = service_of_interest_;
        response_message_cache_ = std::make_unique<response_message>(response.operator*());

        str_tree->next_internal_node_ = std::move(previous_str_tree);
        str_key_tree_map_[service_of_interest_] = std::move(str_tree);

        assigned_member_key_map_[service_of_interest_][response->new_sponsor.assigned_id_] = pending_blinded_secret;
        assigned_member_endpoint_map_[service_of_interest_][pending_remote_endpoint] = response->new_sponsor.assigned_id_;
        keys_computed_count_++;

        send(response.operator*()); statistics_recorder_->record_count(count_metric::RESPONSE_MESSAGE_COUNT_);
        send_cyclic_response();
    } else {
        std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
        offer->offered_service_ = service_of_interest_;
        send(offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);
        send_cyclic_offer();
    }
}

void str_dh::check_and_add_next_blinded_key_to_group_secret() {
    if (!is_sponsor_ && is_assigned()) {
        blinded_secret_t next_blinded_key;
        while ((next_blinded_key = get_next_blinded_key()).SizeInBytes() != 0) {
            secret_t old_group_secret = str_key_tree_map_[service_of_interest_]->root_node_.group_secret_;
            secret_t new_group_secret(diffie_hellman_.AgreedValueLength());
            diffie_hellman_.Agree(new_group_secret, old_group_secret, next_blinded_key); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);
            std::unique_ptr<str_key_tree> str_tree = build_str_tree(new_group_secret,
                                                                    DEFAULT_SECRET,
                                                                    DEFAULT_SECRET,
                                                                    next_blinded_key);
            std::unique_ptr<str_key_tree> previous_str_tree = std::move(str_key_tree_map_[service_of_interest_]);
            str_tree->next_internal_node_ = std::move(previous_str_tree);
            str_key_tree_map_[service_of_interest_] = std::move(str_tree);
            keys_computed_count_++;
        }
    }
}

std::pair<boost::asio::ip::udp::endpoint, blinded_secret_t> str_dh::get_unassigned_member() {
    std::pair<boost::asio::ip::udp::endpoint, blinded_secret_t> unassigned_member;
    for (auto member : pending_requests_[service_of_interest_]) {
        boost::asio::ip::udp::endpoint member_endpoint = member.first;
        blinded_secret_t member_blinded_secret = member.second;
        if (!assigned_member_endpoint_map_[service_of_interest_].contains(member_endpoint)) {
            unassigned_member = std::make_pair(member_endpoint, member_blinded_secret);
            break;
        }
    }
    pending_requests_[service_of_interest_].clear();
    return unassigned_member;
}

blinded_secret_t str_dh::get_next_blinded_key() {
    blinded_secret_t blinded_key;
    if (assigned_member_key_map_[service_of_interest_].contains(keys_computed_count_ + member_id_)) {
        blinded_key = assigned_member_key_map_[service_of_interest_][keys_computed_count_ + member_id_];
    }
    return blinded_key;  
}

std::unique_ptr<str_key_tree> str_dh::build_str_tree(secret_t _group_secret, blinded_secret_t _blinded_group_secret,
                                            secret_t _member_secret, blinded_secret_t _blinded_member_secret) {
    std::unique_ptr<str_key_tree> str_tree = std::make_unique<str_key_tree>();
    str_tree->root_node_.group_secret_ = _group_secret;
    str_tree->root_node_.blinded_group_secret_ = _blinded_group_secret;
    str_tree->leaf_node_.member_secret_ = _member_secret;
    str_tree->leaf_node_.blinded_member_secret_ = _blinded_member_secret;
    return std::move(str_tree);
}

void str_dh::send(message& _message) {
    boost::asio::streambuf buffer;
    message_handler_->serialize(_message, buffer);
    multicast_application_impl::send_multicast(buffer);
}

void str_dh::send_cyclic_offer() {
    scatter_timer_.expires_from_now(scatter_delay_);
    scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
        if (!_error && is_sponsor_ && assigned_member_key_map_[service_of_interest_].size() < member_id_) {
            std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
            offer->offered_service_ = service_of_interest_;
            send(offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);
            send_cyclic_offer();
        }
    });
}

void str_dh::send_cyclic_response() {
    scatter_timer_.expires_from_now(scatter_delay_);
    scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
        if (!_error && assigned_member_key_map_[service_of_interest_].size() <= member_id_ && !higher_member_id_assigned_) {
            std::unique_ptr<response_message> response = std::make_unique<response_message>(response_message_cache_.operator*());
            send(response.operator*()); statistics_recorder_->record_count(count_metric::RESPONSE_MESSAGE_COUNT_);
            send_cyclic_response();
        }
    });
}

void str_dh::send_cyclic_member_info_request_predecessors() {
    scatter_timer_.expires_from_now(scatter_delay_);
    scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
        if (!_error && !all_predecessors_known()) {
            send_member_info_request_predecessors();
            send_cyclic_member_info_request_predecessors();
        }
    });
}

void str_dh::send_member_info_request_predecessors() {
    std::unique_ptr<member_info_request_message> member_info_req_msg = std::make_unique<member_info_request_message>();
    member_info_req_msg->required_service_ = service_of_interest_;
    member_info_req_msg->requested_members_ = get_unknown_predecessors();
    send(member_info_req_msg.operator*()); statistics_recorder_->record_count(count_metric::MEMBER_INFO_REQUEST_MESSAGE_COUNT_);
}

void str_dh::send_cyclic_member_info_synch_request_successors() {
    scatter_timer_.expires_from_now(scatter_delay_);
    scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
        if (!_error && !all_successors_known()) {
            send_member_info_synch_request_successors();
            send_cyclic_member_info_synch_request_successors();
        }
    });
}

void str_dh::send_member_info_synch_request_successors() {
    std::unique_ptr<member_info_synch_request_message> member_info_synch_req_msg = std::make_unique<member_info_synch_request_message>();
    member_info_synch_req_msg->required_service_ = service_of_interest_;
    member_info_synch_req_msg->requested_members_ = get_unknown_successors();
    send(member_info_synch_req_msg.operator*()); statistics_recorder_->record_count(count_metric::MEMBER_INFO_REQUEST_MESSAGE_COUNT_);
}

void str_dh::send_cyclic_synch_token() {
    scatter_timer_.expires_from_now(scatter_delay_);
    scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
        if (!_error && !higher_member_id_synching_) {
            send_synch_token_to_next_member();
            send_cyclic_synch_token();
        }
    });
}

void str_dh::send_synch_token_to_next_member() {
    std::unique_ptr<synch_token_message> synch_token_msg = std::make_unique<synch_token_message>();
    synch_token_msg->member_id_ = member_id_ != member_count_ ? member_id_ + 1 : 1;
    send(synch_token_msg.operator*()); statistics_recorder_->record_count(count_metric::SYNCH_TOKEN_MESSAGE_COUNT_);
}

void str_dh::send_cyclic_finish() {
    scatter_timer_.expires_from_now(scatter_delay_);
    scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
        if (!_error) {
            send_finish();
            send_cyclic_finish();
        }
    });
}

void str_dh::send_finish() {
    std::unique_ptr<finish_message> finish = std::make_unique<finish_message>();
    send(finish.operator*()); statistics_recorder_->record_count(count_metric::FINISH_MESSAGE_COUNT_);
}

bool str_dh::is_assigned() {
    return member_id_ != DEFAULT_MEMBER_ID;
}

bool str_dh::is_last_member() {
    return member_id_ == member_count_;
}

bool str_dh::all_predecessors_known() {
    return assigned_member_endpoint_map_[service_of_interest_].size() >= member_id_-is_assigned();
}

bool str_dh::all_successors_known() {
    return assigned_member_endpoint_map_[service_of_interest_].size() == member_count_-is_assigned();
}

std::vector<member_id_t> str_dh::get_unknown_predecessors() {
    std::vector<member_id_t> unknown_predecessors;
    for (member_id_t i = 1; i < member_id_; i++) {
        if (!assigned_member_key_map_[service_of_interest_].contains(i)) {
            unknown_predecessors.push_back(i);
        }
        // if (unknown_predecessors.size() >= 100) { // Lists can get bigger than buffers causing input stream errors! Asking for a x of members at a time limits the maximum for each request
        //     break;
        // }
    }
    return unknown_predecessors;
}

std::vector<member_id_t> str_dh::get_unknown_successors() {
    std::vector<member_id_t> unknown_successors;
    for (member_id_t i = member_id_+1; i <= member_count_; i++) {
        if (!assigned_member_key_map_[service_of_interest_].contains(i)) {
            unknown_successors.push_back(i);
        }
        // if (unknown_successors.size() >= 100) { // Lists can get bigger than buffers causing input stream errors! Asking for a x of members at a time limits the maximum for each request
        //     break;
        // }
    }
    return unknown_successors;
}

void str_dh::check_if_higher_member_id_assigned(boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (is_assigned() && !higher_member_id_assigned_ && 
        (assigned_member_endpoint_map_[service_of_interest_].count(_remote_endpoint) && assigned_member_endpoint_map_[service_of_interest_][_remote_endpoint] > (member_id_ % member_count_)
        || !assigned_member_endpoint_map_[service_of_interest_].count(_remote_endpoint))) {
        higher_member_id_assigned_ = true;
    }   
}

std::string str_dh::short_secret_repr(secret_t _secret) {
    CryptoPP::Integer secret_int;
    secret_int.Decode(_secret.BytePtr(), _secret.SizeInBytes());
    std::stringstream ss;
    ss << secret_int;
    std::string secret_string = ss.str();
    ss.str("");
    ss << secret_string.substr(0,3) << "..." << secret_string.substr(secret_string.length()-4,3);
    return ss.str();
}

void str_dh::contribute_statistics() {
    if((assigned_member_key_map_[service_of_interest_].size() == assigned_member_endpoint_map_[service_of_interest_].size())
        && (assigned_member_endpoint_map_[service_of_interest_].size()+is_assigned() == member_count_) && ( member_count_ - member_id_ + 1 == keys_computed_count_ )) {
            statistics_recorder_->contribute_statistics();
            multicast_application_impl::stop();
    }
}

std::chrono::milliseconds str_dh::compute_scatter_delay(std::uint32_t _scatter_delay_min, std::uint32_t _scatter_delay_max) {
    if (_scatter_delay_min > _scatter_delay_max) {
        const std::uint32_t tmp(_scatter_delay_min);
        _scatter_delay_min = _scatter_delay_max;
        _scatter_delay_max = tmp;
    }
    std::random_device random_device;
    std::mt19937 mersenne_twister(random_device());
    std::uniform_int_distribution<std::uint32_t> distribution(
            _scatter_delay_min, _scatter_delay_max);
    return std::chrono::milliseconds(distribution(mersenne_twister));
}

void str_dh::start() {
    multicast_application_impl::start();
}

void str_dh::process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}