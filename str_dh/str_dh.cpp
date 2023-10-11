#include "str_dh.hpp"
#include "MODP2048_256sg.hpp"

#include <cryptopp/nbtheory.h>
#include <cryptopp/oids.h>
#include <cryptopp/asn.h>

#define UNINITIALIZED_ADDRESS "0.0.0.0"

str_dh::str_dh(bool _is_sponsor, service_id_t _service_id, int _member_count) : is_sponsor_(_is_sponsor), service_of_interest_(_service_id), member_count_(_member_count), message_handler_(std::make_unique<message_handler>(this)), statistics_recorder_(statistics_recorder::get_instance()), timer_(multicast_application_impl::get_io_service()) {
#ifdef DEFAULT_DH
    diffie_hellman_.AccessGroupParameters().Initialize(P, Q, G);
    LOG_DEBUG("[<str_dh>] Using default DH")
#elif defined(ECC_DH)
    diffie_hellman_.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());
    LOG_DEBUG("[<str_dh>] Using ECDH")
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
        member_id_ = 1;
        keys_computed_count_ = 1;
        str_key_tree_map_[service_of_interest_] = build_str_tree(secret_, blinded_secret_, secret_, blinded_secret_);

        std::unique_ptr<offer_message> initial_offer = std::make_unique<offer_message>();
        initial_offer->offered_service_ = service_of_interest_;
        send(initial_offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);

        timer_.expires_from_now(boost::asio::chrono::seconds(CYCLIC_OFFER_SECONDS));
        timer_.async_wait(
            std::bind(&str_dh::send_cyclic_offer,
                    this, std::placeholders::_1));
    } else {
        keys_computed_count_ = 0;
        std::unique_ptr<find_message> initial_find = std::make_unique<find_message>();
        initial_find->required_service_ = service_of_interest_;
        send(initial_find.operator*()); statistics_recorder_->record_count(count_metric::FIND_MESSAGE_COUNT_);
    }
}

str_dh::~str_dh() {

}

void str_dh::send_cyclic_offer(const boost::system::error_code &_error) {
    if (_error) {
        return;
    }
    if (is_sponsor_) {
        std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
        offer->offered_service_ = service_of_interest_;
        send(offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);
        LOG_DEBUG("[<str_dh>]: member_id=" << member_id_ << " sent cyclic offer")
        timer_.expires_from_now(boost::asio::chrono::seconds(CYCLIC_OFFER_SECONDS));
        timer_.async_wait(
            std::bind(&str_dh::send_cyclic_offer,
                    this, std::placeholders::_1));
    } else {
        std::cerr << "[<str_dh>]: member has to be sponsor in order to offer" << std::endl;
    }
}

void str_dh::received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) {
    std::lock_guard<std::mutex> lock_receive(receive_mutex_);
    message_handler_->deserialize_and_callback(_data, _bytes_recvd, _remote_endpoint);
}

void str_dh::process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (is_sponsor_ && service_of_interest_ == _rcvd_find_message.required_service_) {
        std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
        offer->offered_service_ = service_of_interest_;
        send(offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);
    }
}

void str_dh::process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!is_assigned() && _rcvd_offer_message.offered_service_ == service_of_interest_) {
        std::unique_ptr<request_message> request = std::make_unique<request_message>();
        request->blinded_secret_ = blinded_secret_;
        request->required_service_ = service_of_interest_;
        send(request.operator*()); statistics_recorder_->record_count(count_metric::REQUEST_MESSAGE_COUNT_);
    }
}

void str_dh::process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!assigned_member_endpoint_map_[_rcvd_request_message.required_service_].contains(_remote_endpoint)
        && !pending_requests_[_rcvd_request_message.required_service_].contains(_remote_endpoint)) {
        pending_requests_[_rcvd_request_message.required_service_][_remote_endpoint] = _rcvd_request_message.blinded_secret_;
    }
    if(member_id_ != INITIAL_SPONSOR_ID) {
        process_pending_request();
    } else if(member_id_ == INITIAL_SPONSOR_ID && is_sponsor_ && pending_requests_[service_of_interest_].size() == member_count_-1) {
        timer_.cancel();
        statistics_recorder_->record_timestamp(time_metric::KEY_AGREEMENT_START_);
        process_pending_request();
    }

    if(member_id_ == INITIAL_SPONSOR_ID) {
        LOG_DEBUG("[<str_dh>]: pending_requests size=" << pending_requests_[service_of_interest_].size())
    }
    contribute_statistics();
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
    if (become_sponsor && !is_assigned() && _rcvd_response_message.offered_service_ == service_of_interest_) {
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
        LOG_DEBUG("[<str_dh>]: (become_sponsor) Compute group key with blinded group secret from member_id=" << assigned_member_endpoint_map_[service_of_interest_][_remote_endpoint] << ", keys_computed=" << keys_computed_count_)
        LOG_DEBUG("[<str_dh>]: assigned id=" << member_id_ << ", group secret=" << short_secret_repr(str_key_tree_map_[service_of_interest_]->root_node_.group_secret_) << " of service " << service_of_interest_)
    }

    if (!is_sponsor_ && is_assigned()
        && (keys_computed_count_ + member_id_) == _rcvd_response_message.new_sponsor.assigned_id_
        && _rcvd_response_message.offered_service_ == service_of_interest_) {

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

            LOG_DEBUG("[<str_dh>]: (no sponsor and assigned) Compute group key with blinded group secret from member_id=" << keys_computed_count_ + member_id_ << ", keys_computed=" << keys_computed_count_)
            LOG_DEBUG("[<str_dh>]: assigned id=" << member_id_ << ", group secret=" << short_secret_repr(str_key_tree_map_[service_of_interest_]->root_node_.group_secret_) << " of service " << service_of_interest_)

            keys_computed_count_++;
        }
    }
    process_pending_request();
    contribute_statistics();
}

void str_dh::process_pending_request() {
    if (!is_sponsor_) {
        return;
    }

    if (all_predecessors_known()) {
        LOG_DEBUG("Request synch (not implemented yet)")
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

        str_tree->next_internal_node_ = std::move(previous_str_tree);
        str_key_tree_map_[service_of_interest_] = std::move(str_tree);

        assigned_member_key_map_[service_of_interest_][response->new_sponsor.assigned_id_] = pending_blinded_secret;
        assigned_member_endpoint_map_[service_of_interest_][pending_remote_endpoint] = response->new_sponsor.assigned_id_;

        keys_computed_count_++;
        LOG_DEBUG("[<str_dh>]: (process_pending_request) Compute group key with blinded secret from member_id=" << assigned_member_endpoint_map_[service_of_interest_][pending_remote_endpoint] << ", keys_computed=" << keys_computed_count_)
        LOG_DEBUG("[<str_dh>]: assigned id=" << member_id_ << ", group secret=" << short_secret_repr(str_key_tree_map_[service_of_interest_]->root_node_.group_secret_) << " of service " << service_of_interest_)
        send(response.operator*()); statistics_recorder_->record_count(count_metric::RESPONSE_MESSAGE_COUNT_);
    } else {
        LOG_DEBUG("Send offer (not implemented yet)")
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

bool str_dh::is_assigned() {
    return member_id_ != DEFAULT_MEMBER_ID;
}

bool str_dh::all_predecessors_known() {
    return assigned_member_endpoint_map_[service_of_interest_].size() < member_id_-1;
}

std::string str_dh::short_secret_repr(secret_t _secret) {
    CryptoPP::Integer secret_int;
    secret_int.Decode(_secret.BytePtr(), _secret.SizeInBytes());
    std::ostringstream oss;
    oss << secret_int;
    std::string secret_string = oss.str();
    oss.str("");
    oss << secret_string.substr(0,3) << "..." << secret_string.substr(secret_string.length()-4,3);
    return oss.str();
}

void str_dh::contribute_statistics() {
    if((assigned_member_key_map_[service_of_interest_].size() == assigned_member_endpoint_map_[service_of_interest_].size())
        && (assigned_member_endpoint_map_[service_of_interest_].size()+is_assigned() == member_count_)) {
            if(member_id_ == INITIAL_SPONSOR_ID) {
                statistics_recorder_->record_timestamp(time_metric::DURATION_END_);
            }
            statistics_recorder_->contribute_statistics();
            multicast_application_impl::stop();
    }
}

void str_dh::start() {
    multicast_application_impl::start();
}

void str_dh::process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}