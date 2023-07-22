#include "member.hpp"
#include "MODP2048_256sg.hpp"
#include "message.hpp"
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
        str_key_tree_map_[offered_service_].reset(new str_key_tree());
        str_key_tree_map_[offered_service_]->root_node.group_secret = secret_int_;
        str_key_tree_map_[offered_service_]->root_node.blinded_group_secret = blinded_secret_int_;

        response_message rmsg;
        rmsg.blinded_group_secret_int_ = 8;
        rmsg.blinded_sponsor_secret_int_ = 16;
        rmsg.member_count_ = 32;
        rmsg.offered_service_ = 64;
        rmsg.new_sponsor.ip_address_ = boost::asio::ip::address::from_string("255.144.255.144");
        rmsg.new_sponsor.port_ = 128;
        rmsg.new_sponsor.assigned_id_ = 256;
        boost::asio::streambuf buffer;
        write_to_streambuf(buffer, reinterpret_cast<const char*>(&rmsg.message_type_), MESSAGE_ID_SIZE);
        rmsg.serialize_(buffer);
        send(buffer);

        find_message fmsg;
        fmsg.required_service_ = 1000;
        boost::asio::streambuf fbuffer;
        write_to_streambuf(fbuffer, reinterpret_cast<const char*>(&fmsg.message_type_), MESSAGE_ID_SIZE);
        fmsg.serialize_(fbuffer);
        send(fbuffer);
    } else {
        required_service_ = _service_id;
    }
    LOG_DEBUG("[<member>]: initialization complete")
}

member::~member() {
    LOG_DEBUG("[<member>]: destruction complete")
}

void member::received_data(unsigned char* _data, size_t _bytes_recvd) {
    LOG_DEBUG("[<member>]: received data")
    boost::asio::streambuf buffer;
    write_to_streambuf(buffer, reinterpret_cast<const char*>(_data), _bytes_recvd);
    message_id_t message_id;
    read_from_streambuf(buffer, reinterpret_cast<char*>(&message_id), MESSAGE_ID_SIZE);
    switch (message_id)
    {
    case message_type::FIND: {
        LOG_DEBUG("[<member>]: received data, FIND case")
        find_message rcvd_find_message;
        rcvd_find_message.deserialize_(buffer);
        LOG_DEBUG("required service: " << rcvd_find_message.required_service_)
    }
        break;
    case message_type::OFFER:
        /* code */
        break;
    case message_type::REQUEST:
        /* code */
        break;
    case message_type::RESPONSE: {
        LOG_DEBUG("[<member>]: received data, RESPONSE case")
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
        break;
    default:
        std::cerr << "Unknown message type received" << std::endl;
        break;
    }
}

void member::send(boost::asio::streambuf& buffer) {
    LOG_DEBUG("[<member>]: send")
    multicast_application_impl::send(buffer);
}

void member::start() {
    LOG_DEBUG("[<member>]: start")
    multicast_application_impl::start();
}