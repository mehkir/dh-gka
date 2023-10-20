#include "tg_dh.hpp"
#include "MODP2048_256sg.hpp"

#include <cryptopp/nbtheory.h>

#define UNINITIALIZED_ADDRESS "0.0.0.0"

tg_dh::tg_dh(bool _is_sponsor, service_id_t _service_id) : is_sponsor_(_is_sponsor), service_of_interest_(_service_id), message_handler_(std::make_unique<message_handler>(this)) {
    diffie_hellman_.AccessGroupParameters().Initialize(P, Q, G);
    secret_.New(diffie_hellman_.PrivateKeyLength());
    blinded_secret_.New(diffie_hellman_.PublicKeyLength());
    diffie_hellman_.GeneratePrivateKey(rnd_, secret_);
    diffie_hellman_.GeneratePublicKey(rnd_, secret_, blinded_secret_);
    secret_int_.Decode(secret_.BytePtr(), secret_.SizeInBytes());
    blinded_secret_int_.Decode(blinded_secret_.BytePtr(), blinded_secret_.SizeInBytes());

    tg_key_tree_map_.clear();
    pending_requests_.clear();
    assigned_member_key_map_.clear();
    assigned_member_endpoint_map_.clear();

    if (is_sponsor_) {
        member_id_ = 1;
        keys_computed_count_ = 1;
        std::unique_ptr<member_node> member = std::make_unique<member_node>();
        member->left_height_ = 0;
        member->right_height_ = 0;
        member->member_secret_ = secret_int_;
        member->blinded_member_secret_ = blinded_secret_int_;
        tg_key_tree_map_[service_of_interest_] = std::move(member);

        std::unique_ptr<offer_message> initial_offer = std::make_unique<offer_message>();
        initial_offer->offered_service_ = service_of_interest_;
        send(initial_offer.operator*());
    } else {
        keys_computed_count_ = 0;
        std::unique_ptr<find_message> initial_find = std::make_unique<find_message>();
        initial_find->required_service_ = service_of_interest_;
        send(initial_find.operator*());
    }
}

tg_dh::~tg_dh() {

}

void tg_dh::received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) {
    std::lock_guard<std::mutex> lock_receive(receive_mutex_);
    if (get_local_endpoint().port() != _remote_endpoint.port()) {
        message_handler_->deserialize_and_callback(_data, _bytes_recvd, _remote_endpoint);
    }
}

void tg_dh::process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (is_sponsor_ && service_of_interest_ == _rcvd_find_message.required_service_) {
        std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
        offer->offered_service_ = service_of_interest_;
        send(offer.operator*());
    }
}

void tg_dh::process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!is_assigned() && _rcvd_offer_message.offered_service_ == service_of_interest_) {
        std::unique_ptr<request_message> request = std::make_unique<request_message>();
        request->blinded_secret_ = blinded_secret_;
        request->required_service_ = service_of_interest_;
        send(request.operator*());
    }
}

void tg_dh::process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!assigned_member_endpoint_map_[_rcvd_request_message.required_service_].contains(_remote_endpoint)
        && !pending_requests_[_rcvd_request_message.required_service_].contains(_remote_endpoint)) {
        pending_requests_[_rcvd_request_message.required_service_][_remote_endpoint] = _rcvd_request_message.blinded_secret_;
    }
    process_pending_request();
}

void tg_dh::process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
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

        // TODO

        keys_computed_count_++;
        LOG_DEBUG("[<tg_dh>]: (become_sponsor) Compute group key with blinded group secret from member_id=" << assigned_member_endpoint_map_[service_of_interest_][_remote_endpoint] << ", keys_computed=" << keys_computed_count_)
        LOG_DEBUG("[<tg_dh>]: assigned id=" << member_id_ << ", group secret=" << 111 << " of service " << service_of_interest_)
    }

    if (!is_sponsor_ && is_assigned()
        && (keys_computed_count_ + member_id_) == _rcvd_response_message.new_sponsor.assigned_id_
        && _rcvd_response_message.offered_service_ == service_of_interest_) {

        blinded_secret_t next_blinded_key;
        while ((next_blinded_key = get_next_blinded_key()).SizeInBytes() != 0) {

            // TODO

            keys_computed_count_++;
            LOG_DEBUG("[<tg_dh>]: (no sponsor and assigned) Compute group key with blinded group secret from member_id=" << keys_computed_count_ + member_id_ - 1  << ", keys_computed=" << keys_computed_count_)
            LOG_DEBUG("[<tg_dh>]: assigned id=" << member_id_ << ", group secret=" << 111 << " of service " << service_of_interest_)
        }
    }

    process_pending_request();
}

void tg_dh::process_member_info_request(member_info_request_message _rcvd_member_info_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void tg_dh::process_member_info_response(member_info_response_message _rcvd_member_info_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void tg_dh::process_synch_token(synch_token_message _rcvd_synch_token_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void tg_dh::process_member_info_synch_request(member_info_synch_request_message _rcvd_member_info_synch_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void tg_dh::process_member_info_synch_response(member_info_synch_response_message _rcvd_member_info_synch_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void tg_dh::process_finish(finish_message _rcvd_finish_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void tg_dh::process_finish_ack(finish_ack_message _rcvd_finish_ack_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void tg_dh::process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void tg_dh::process_pending_request() {
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

        is_sponsor_ = false;

        // TODO
        std::unique_ptr<response_message> response = std::make_unique<response_message>();
        assigned_member_key_map_[service_of_interest_][response->new_sponsor.assigned_id_] = pending_blinded_secret;
        assigned_member_endpoint_map_[service_of_interest_][pending_remote_endpoint] = response->new_sponsor.assigned_id_;

        keys_computed_count_++;
        LOG_DEBUG("[<tg_dh>]: (process_pending_request) Compute group key with blinded secret from member_id=" << assigned_member_endpoint_map_[service_of_interest_][pending_remote_endpoint] << ", keys_computed=" << keys_computed_count_)
        LOG_DEBUG("[<tg_dh>]: assigned id=" << member_id_ << ", group secret=" << 111 << " of service " << service_of_interest_)
        send(response.operator*());
    } else {
        LOG_DEBUG("Send offer (not implemented yet)")
    }
}

blinded_secret_t tg_dh::get_next_blinded_key() {
    blinded_secret_t blinded_key;
    if (assigned_member_key_map_[service_of_interest_].contains(keys_computed_count_ + member_id_)) {
        blinded_key = assigned_member_key_map_[service_of_interest_][keys_computed_count_ + member_id_];
    }
    return blinded_key;  
}

void tg_dh::send(message& _message) {
    boost::asio::streambuf buffer;
    message_handler_->serialize(_message, buffer);
    multicast_application_impl::send_multicast(buffer);
}

bool tg_dh::is_assigned() {
    return member_id_ != DEFAULT_MEMBER_ID;
}

bool tg_dh::all_predecessors_known() {
    return assigned_member_endpoint_map_[service_of_interest_].size() < member_id_-1;
}

std::string tg_dh::short_secret_repr(secret_t _secret) {
    CryptoPP::Integer secret_int;
    secret_int.Decode(_secret.BytePtr(), _secret.SizeInBytes());
    std::ostringstream oss;
    oss << secret_int;
    std::string secret_string = oss.str();
    oss.str("");
    oss << secret_string.substr(0,3) << "..." << secret_string.substr(secret_string.length()-4,3);
    return oss.str();
}

void tg_dh::start() {
    multicast_application_impl::start();
}