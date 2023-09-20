#include "str_dh.hpp"
#include "MODP2048_256sg.hpp"

#include <cryptopp/nbtheory.h>

#define UNINITIALIZED_ADDRESS "0.0.0.0"

str_dh::str_dh(bool _is_sponsor, service_id_t _service_id) : is_sponsor_(_is_sponsor), service_of_interest_(_service_id), message_handler_(std::make_unique<message_handler>(this)) {
    diffie_hellman_.AccessGroupParameters().Initialize(P, Q, G);
    secret_.New(diffie_hellman_.PrivateKeyLength());
    blinded_secret_.New(diffie_hellman_.PublicKeyLength());
    diffie_hellman_.GeneratePrivateKey(rnd_, secret_);
    diffie_hellman_.GeneratePublicKey(rnd_, secret_, blinded_secret_);
    secret_int_.Decode(secret_.BytePtr(), secret_.SizeInBytes());
    blinded_secret_int_.Decode(blinded_secret_.BytePtr(), blinded_secret_.SizeInBytes());

    str_key_tree_map_.clear();
    pending_requests_.clear();
    assigned_member_key_map_.clear();
    assigned_member_endpoint_map_.clear();

    if (is_sponsor_) {
        member_id_ = 1;
        keys_computed_count_ = 1;
        str_key_tree_map_[service_of_interest_] = build_str_tree(secret_int_, blinded_secret_int_, secret_int_, blinded_secret_int_);

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

str_dh::~str_dh() {

}

void str_dh::received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) {
    std::lock_guard<std::mutex> lock_receive(receive_mutex_);
    message_handler_->deserialize_and_callback(_data, _bytes_recvd, _remote_endpoint);
}

void str_dh::process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (is_sponsor_ && service_of_interest_ == _rcvd_find_message.required_service_) {
        std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
        offer->offered_service_ = service_of_interest_;
        send(offer.operator*());
    }
}

void str_dh::process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!is_assigned() && _rcvd_offer_message.offered_service_ == service_of_interest_) {
        std::unique_ptr<request_message> request = std::make_unique<request_message>();
        request->blinded_secret_int_ = blinded_secret_int_;
        request->required_service_ = service_of_interest_;
        send(request.operator*());
    }
}

void str_dh::process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!assigned_member_endpoint_map_[_rcvd_request_message.required_service_].contains(_remote_endpoint)
        && !pending_requests_[_rcvd_request_message.required_service_].contains(_remote_endpoint)) {
        pending_requests_[_rcvd_request_message.required_service_][_remote_endpoint] = _rcvd_request_message.blinded_secret_int_;
    }
    process_pending_request();
    
    if (is_assigned()) {
        LOG_DEBUG("[<member>]: assigned id=" << member_id_ << ", group secret=" << str_key_tree_map_[service_of_interest_]->root_node_.group_secret_ << " of service " << service_of_interest_ << ", keys_computed=" << keys_computed_count_)
    }
}

void str_dh::process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    boost::asio::ip::udp::endpoint new_sponsor_endpoint(_rcvd_response_message.new_sponsor.ip_address_, _rcvd_response_message.new_sponsor.port_);
    // Add new assigned sponsor
    if (new_sponsor_endpoint != get_local_endpoint() && !assigned_member_endpoint_map_[_rcvd_response_message.offered_service_].contains(new_sponsor_endpoint)) {
        assigned_member_key_map_[_rcvd_response_message.offered_service_][_rcvd_response_message.new_sponsor.assigned_id_] = _rcvd_response_message.new_sponsor.blinded_secret_int_;
        assigned_member_endpoint_map_[_rcvd_response_message.offered_service_][new_sponsor_endpoint] = _rcvd_response_message.new_sponsor.assigned_id_;
    }
    // Add old assigned sponsor
    if (!assigned_member_endpoint_map_[_rcvd_response_message.offered_service_].contains(_remote_endpoint)) {
        assigned_member_key_map_[_rcvd_response_message.offered_service_][_rcvd_response_message.new_sponsor.assigned_id_-1] = _rcvd_response_message.blinded_sponsor_secret_int_;
        assigned_member_endpoint_map_[_rcvd_response_message.offered_service_][_remote_endpoint] = _rcvd_response_message.new_sponsor.assigned_id_-1;
    }
    pending_requests_[_rcvd_response_message.offered_service_].erase(new_sponsor_endpoint);
    pending_requests_[_rcvd_response_message.offered_service_].erase(_remote_endpoint);

    bool become_sponsor = get_local_endpoint() == new_sponsor_endpoint;
    if (become_sponsor && !is_assigned() && _rcvd_response_message.offered_service_ == service_of_interest_) {
        is_sponsor_ = true;
        member_id_ = _rcvd_response_message.new_sponsor.assigned_id_;
        secret_int_t group_secret = CryptoPP::ModularExponentiation(_rcvd_response_message.blinded_group_secret_int_, secret_int_, P);
        std::unique_ptr<str_key_tree> str_tree = build_str_tree(group_secret,
                                                                CryptoPP::ModularExponentiation(G, group_secret, P),
                                                                secret_int_,
                                                                blinded_secret_int_);
        std::unique_ptr<str_key_tree> previous_str_tree = build_str_tree(DEFAULT_VALUE,
                                                                        _rcvd_response_message.blinded_sponsor_secret_int_,
                                                                        DEFAULT_VALUE,
                                                                        _rcvd_response_message.blinded_group_secret_int_);
        str_tree->next_internal_node_ = std::move(previous_str_tree);
        str_key_tree_map_[service_of_interest_] = std::move(str_tree);

        keys_computed_count_++;
        LOG_DEBUG("(become_sponsor) Compute group key with blinded group secret from IP=" << _remote_endpoint.address().to_string() << ", Port=" << _remote_endpoint.port() << ", keys_computed=" << keys_computed_count_)
    }

    if (!is_sponsor_ && is_assigned()
        && (keys_computed_count_ + member_id_) == _rcvd_response_message.new_sponsor.assigned_id_
        && _rcvd_response_message.offered_service_ == service_of_interest_) {

        blinded_secret_int_t next_blinded_key;
        while ((next_blinded_key = get_next_blinded_key()) != 0) {
            secret_int_t group_secret = str_key_tree_map_[service_of_interest_]->root_node_.group_secret_;
            std::unique_ptr<str_key_tree> str_tree = build_str_tree(CryptoPP::ModularExponentiation(next_blinded_key, group_secret, P),
                                                                    DEFAULT_VALUE,
                                                                    DEFAULT_VALUE,
                                                                    next_blinded_key);
            std::unique_ptr<str_key_tree> previous_str_tree = std::move(str_key_tree_map_[service_of_interest_]);
            str_tree->next_internal_node_ = std::move(previous_str_tree);
            str_key_tree_map_[service_of_interest_] = std::move(str_tree);

            keys_computed_count_++;
            LOG_DEBUG("(assigned) Compute group key with blinded group secret from member_id=" << keys_computed_count_ + member_id_ + 1  << ", keys_computed=" << keys_computed_count_)
        }
    }

    process_pending_request();

    if (is_assigned()) {
        LOG_DEBUG("[<member>]: assigned id=" << member_id_ << ", group secret=" << str_key_tree_map_[service_of_interest_]->root_node_.group_secret_ << " of service " << service_of_interest_ << ", keys_computed=" << keys_computed_count_)
    }
}

void str_dh::process_pending_request() {
    if (!is_sponsor_) {
        return;
    }

    if (assigned_member_endpoint_map_[service_of_interest_].size() < member_id_-1) { // all_predecessors_known?
        LOG_DEBUG("Request synch (not implemented yet)")
        return;
    }

    std::pair<boost::asio::ip::udp::endpoint, blinded_secret_int_t> unassigned_member = get_unassigned_member();
    boost::asio::ip::udp::endpoint pending_remote_endpoint = unassigned_member.first;
    blinded_secret_int_t pending_blinded_secret = unassigned_member.second;

    if (pending_remote_endpoint.address().to_string().compare(UNINITIALIZED_ADDRESS) != 0 && pending_blinded_secret != 0) {

        std::unique_ptr<str_key_tree> previous_str_tree = std::move(str_key_tree_map_[service_of_interest_]);
        std::unique_ptr<str_key_tree> str_tree = build_str_tree(CryptoPP::ModularExponentiation(pending_blinded_secret, previous_str_tree->root_node_.group_secret_, P),
                                                                DEFAULT_VALUE,
                                                                DEFAULT_VALUE,
                                                                pending_blinded_secret);

        is_sponsor_ = false;
        std::unique_ptr<response_message> response = std::make_unique<response_message>();
        response->blinded_group_secret_int_ = previous_str_tree->root_node_.blinded_group_secret_;
        response->blinded_sponsor_secret_int_ = blinded_secret_int_;
        response->new_sponsor.assigned_id_ = member_id_+1;
        response->new_sponsor.ip_address_ = pending_remote_endpoint.address();
        response->new_sponsor.port_ = pending_remote_endpoint.port();
        response->new_sponsor.blinded_secret_int_ = pending_blinded_secret;
        response->offered_service_ = service_of_interest_;

        str_tree->next_internal_node_ = std::move(previous_str_tree);
        str_key_tree_map_[service_of_interest_] = std::move(str_tree);

        assigned_member_key_map_[service_of_interest_][response->new_sponsor.assigned_id_] = pending_blinded_secret;
        assigned_member_endpoint_map_[service_of_interest_][pending_remote_endpoint] = response->new_sponsor.assigned_id_;
        //pending_requests_[service_of_interest_].erase(pending_remote_endpoint);

        keys_computed_count_++;
        LOG_DEBUG("(process_pending_request) Compute group key with blinded secret from IP=" << pending_remote_endpoint.address().to_string() << ", Port=" << pending_remote_endpoint.port() << ", keys_computed=" << keys_computed_count_)
        send(response.operator*());
    } else {
        LOG_DEBUG("Send offer (not implemented yet)")
    }
}


std::pair<boost::asio::ip::udp::endpoint, blinded_secret_int_t> str_dh::get_unassigned_member() {
    std::pair<boost::asio::ip::udp::endpoint, blinded_secret_int_t> unassigned_member;
    for (auto member : pending_requests_[service_of_interest_]) {
        boost::asio::ip::udp::endpoint member_endpoint = member.first;
        blinded_secret_int_t member_blinded_secret = member.second;
        if (!assigned_member_endpoint_map_[service_of_interest_].contains(member_endpoint)) {
            unassigned_member = std::make_pair(member_endpoint, member_blinded_secret);
            break;
        }
    }
    pending_requests_[service_of_interest_].clear();
    return unassigned_member;
}

blinded_secret_int_t str_dh::get_next_blinded_key() {
    return assigned_member_key_map_[service_of_interest_][keys_computed_count_ + member_id_];  
}

std::unique_ptr<str_key_tree> str_dh::build_str_tree(CryptoPP::Integer _group_secret, CryptoPP::Integer _blinded_group_secret,
                                            CryptoPP::Integer _member_secret, CryptoPP::Integer _blinded_member_secret) {
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
    multicast_application_impl::send(buffer);
}

bool str_dh::is_assigned() {
    return member_id_ != DEFAULT_MEMBER_ID;
}

void str_dh::start() {
    multicast_application_impl::start();
}