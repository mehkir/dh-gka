#include "member.hpp"
#include "MODP2048_256sg.hpp"
#include "logger.hpp"

member::member(bool _is_sponsor, service_id_t _service_id) : is_sponsor_(_is_sponsor) {
    diffie_hellman_.AccessGroupParameters().Initialize(p, q, g);
    secret_.New(diffie_hellman_.PrivateKeyLength());
    blinded_secret_.New(diffie_hellman_.PublicKeyLength());
    diffie_hellman_.GeneratePrivateKey(rnd_, secret_);
    diffie_hellman_.GeneratePublicKey(rnd_, secret_, blinded_secret_);
    secret_int_.Decode(secret_.BytePtr(), secret_.SizeInBytes());
    blinded_secret_int_.Decode(blinded_secret_.BytePtr(), blinded_secret_.SizeInBytes());
    if (is_sponsor_) {
        offered_service_ = _service_id;
        member_id = 1;
        str_key_tree_map_[offered_service_] = std::make_unique<str_key_tree>();
        str_key_tree_map_[offered_service_]->root_node.group_secret = secret_int_;
        str_key_tree_map_[offered_service_]->root_node.blinded_group_secret = blinded_secret_int_;

        std::unique_ptr<offer_message> initial_offer = std::make_unique<offer_message>();
        initial_offer->offered_service_ = offered_service_;
        send(initial_offer.operator*());
    } else {
        required_service_ = _service_id;
        std::unique_ptr<find_message> initial_find = std::make_unique<find_message>();
        initial_find->required_service_ = required_service_;
        send(initial_find.operator*());
    }
    LOG_DEBUG("[<member>]: initialization complete")
}

member::~member() {
    LOG_DEBUG("[<member>]: destruction complete")
}

void member::received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) {
    LOG_DEBUG("[<member>]: received data")
    boost::asio::streambuf buffer;
    write_to_streambuf(buffer, reinterpret_cast<const char*>(_data), _bytes_recvd);
    switch (extract_message_id(buffer))
    {
    case message_type::FIND: {
        LOG_DEBUG("[<member>]: received data, FIND case")
        process_find(buffer, _remote_endpoint);
    }
        break;
    case message_type::OFFER: {
        LOG_DEBUG("[<member>]: received data, OFFER case")
        process_offer(buffer, _remote_endpoint);
    }
        break;
    case message_type::REQUEST: {
        LOG_DEBUG("[<member>]: received data, REQUEST case")
        process_request(buffer, _remote_endpoint);
    }
        break;
    case message_type::RESPONSE: {
        LOG_DEBUG("[<member>]: received data, RESPONSE case")
        process_response(buffer, _remote_endpoint);
    }
        break;
    default:
        std::cerr << "Unknown message type received" << std::endl;
        break;
    }
}

message_id_t member::extract_message_id(boost::asio::streambuf& buffer) {
    message_id_t message_id;
    read_from_streambuf(buffer, reinterpret_cast<char*>(&message_id), MESSAGE_ID_SIZE);
    return message_id;
}

void member::process_find(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    find_message rcvd_find_message;
    rcvd_find_message.deserialize_(buffer);
    LOG_DEBUG("required service: " << rcvd_find_message.required_service_)
}

void member::process_offer(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    offer_message rcvd_offer_message;
    rcvd_offer_message.deserialize_(buffer);
    LOG_DEBUG("offered service: " << rcvd_offer_message.offered_service_)

    std::unique_ptr<request_message> request = std::make_unique<request_message>();
    request->blinded_secret_int_ = blinded_secret_int_;
    request->required_service_ = required_service_;
    send(request.operator*());
}

void member::process_request(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    request_message rcvd_request_message;
    rcvd_request_message.deserialize_(buffer);
    LOG_DEBUG("blinded secret: " << rcvd_request_message.blinded_secret_int_)
    LOG_DEBUG("required service: " << rcvd_request_message.required_service_)

    std::unique_ptr<response_message> response = std::make_unique<response_message>();
    response->blinded_group_secret_int_ = 0;
    response->blinded_sponsor_secret_int_ = blinded_secret_int_;
    response->member_count_ = 2;
    response->new_sponsor.assigned_id_ = 1;
    response->new_sponsor.ip_address_ = boost::asio::ip::address::from_string("0.0.0.0");
    response->new_sponsor.port_ = 1111;
    response->offered_service_ = offered_service_;
    send(response.operator*());
}

void member::process_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    response_message rcvd_response_message;
    rcvd_response_message.deserialize_(buffer);
    LOG_DEBUG("blinded group secret: " << rcvd_response_message.blinded_group_secret_int_)
    LOG_DEBUG("blinded sponsor secret: " << rcvd_response_message.blinded_sponsor_secret_int_)
    LOG_DEBUG("member count: " << rcvd_response_message.member_count_)
    LOG_DEBUG("offered service: " << rcvd_response_message.offered_service_)
    LOG_DEBUG("assigned id: " << rcvd_response_message.new_sponsor.assigned_id_)
    LOG_DEBUG("ip address: " << rcvd_response_message.new_sponsor.ip_address_)
    LOG_DEBUG("port: " << rcvd_response_message.new_sponsor.port_)
}

void member::send(message& _message) {
    LOG_DEBUG("[<member>]: send")
    boost::asio::streambuf buffer;
    write_to_streambuf(buffer, reinterpret_cast<const char*>(&_message.message_type_), MESSAGE_ID_SIZE);
    _message.serialize_(buffer);
    multicast_application_impl::send(buffer);
}

void member::start() {
    LOG_DEBUG("[<member>]: start")
    multicast_application_impl::start();
}