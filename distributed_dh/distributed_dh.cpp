#include "distributed_dh.hpp"
#include "MODP2048_256sg.hpp"

#include <cryptopp/nbtheory.h>

distributed_dh::distributed_dh(bool _is_sponsor, service_id_t _service_id) : is_sponsor_(_is_sponsor), service_of_interest_(_service_id), message_handler_(std::make_unique<message_handler>(this)) {
    diffie_hellman_.AccessGroupParameters().Initialize(P, Q, G);
    group_secret_.New(diffie_hellman_.PrivateKeyLength());    
    secret_.New(diffie_hellman_.PrivateKeyLength());
    blinded_secret_.New(diffie_hellman_.PublicKeyLength());
    diffie_hellman_.GeneratePrivateKey(rnd_, group_secret_);
    diffie_hellman_.GeneratePrivateKey(rnd_, secret_);
    diffie_hellman_.GeneratePublicKey(rnd_, secret_, blinded_secret_);
    group_secret_int_.Decode(group_secret_.BytePtr(), group_secret_.SizeInBytes());
    secret_int_.Decode(secret_.BytePtr(), secret_.SizeInBytes());
    blinded_secret_int_.Decode(blinded_secret_.BytePtr(), blinded_secret_.SizeInBytes());

    if (is_sponsor_) {
        member_id_ = 1;

        std::unique_ptr<offer_message> initial_offer = std::make_unique<offer_message>();
        initial_offer->offered_service_ = service_of_interest_;
        send_multicast(initial_offer.operator*());
    } else {
        std::unique_ptr<find_message> initial_find = std::make_unique<find_message>();
        initial_find->required_service_ = service_of_interest_;
        send_multicast(initial_find.operator*());
    }
}

distributed_dh::~distributed_dh() {

}

void distributed_dh::start() {
    multicast_application_impl::start();
}

void distributed_dh::received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) {
    std::lock_guard<std::mutex> lock_receive(receive_mutex_);
    message_handler_->deserialize_and_callback(_data, _bytes_recvd, _remote_endpoint);
}

void distributed_dh::process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (is_sponsor_ && service_of_interest_ == _rcvd_find_message.required_service_) {
        std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
        offer->offered_service_ = service_of_interest_;
        send_multicast(offer.operator*());
    }
}

void distributed_dh::process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!is_assigned() && _rcvd_offer_message.offered_service_ == service_of_interest_) {
        std::unique_ptr<request_message> request = std::make_unique<request_message>();
        request->blinded_secret_int_ = blinded_secret_int_;
        request->required_service_ = service_of_interest_;
        send_to(request.operator*(), _remote_endpoint);
    }
}

void distributed_dh::process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    secret_int_t shared_secret_int =  CryptoPP::ModularExponentiation(_rcvd_request_message.blinded_secret_int_, secret_int_, P);
    CryptoPP::SecByteBlock shared_secret

    shared_secret_int.Encode()

}

void distributed_dh::process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {

}

void distributed_dh::send_multicast(message& _message) {
    boost::asio::streambuf buffer;
    message_handler_->serialize(_message, buffer);
    multicast_application_impl::send_multicast(buffer);
}

void distributed_dh::send_to(message& _message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    boost::asio::streambuf buffer;
    message_handler_->serialize(_message, buffer);
    multicast_application_impl::send_to(buffer, _remote_endpoint);
}

std::string distributed_dh::short_secret_repr(secret_int_t _secret) {
    std::ostringstream oss;
    oss << _secret;
    std::string secret_string = oss.str();
    oss.str("");
    oss << secret_string.substr(0,3) << "..." << secret_string.substr(secret_string.length()-4,3);
    return oss.str();
}

bool distributed_dh::is_assigned() {
    return member_id_ != DEFAULT_MEMBER_ID;
}

void distributed_dh::process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}