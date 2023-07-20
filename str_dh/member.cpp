#include "member.hpp"
#include "MODP2048_256sg.hpp"
#include "message.hpp"

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
        rmsg.blinded_member_secret_int_ = 16;
        rmsg.member_count_ = 32;
        rmsg.offered_service_ = 64;
        send(&rmsg, sizeof(response_message));

        find_message fmsg;
        fmsg.required_service_ = 1000;
        send(&fmsg, sizeof(find_message));
    } else {
        required_service_ = _service_id;
    }
}

member::~member() {
}

void member::received_data(void* _data, std::size_t _bytes_recvd) {
    message* rcvd_message = reinterpret_cast<message*>(_data);
    switch (rcvd_message->message_type_)
    {
    case message_type::FIND: {
        find_message rcvd_find_message = *reinterpret_cast<find_message*>(rcvd_message);
        std::cout << rcvd_find_message.required_service_ << std::endl;
    }
        break;
    case message_type::OFFER:
        /* code */
        break;
    case message_type::REQUEST:
        /* code */
        break;
    case message_type::RESPONSE: {
        response_message* rcvd_response_message = reinterpret_cast<response_message*>(rcvd_message);
        std::cout << rcvd_response_message->blinded_group_secret_int_ << std::endl;
        std::cout << rcvd_response_message->blinded_member_secret_int_ << std::endl;
        std::cout << rcvd_response_message->member_count_ << std::endl;
        std::cout << rcvd_response_message->offered_service_ << std::endl;
    }
        break;
    default:
        break;
    }
}

void member::send(void* _data, std::size_t _data_byte_size) {
    multicast_application_impl::send(_data, _data_byte_size);
}

void member::start() {
    multicast_application_impl::start();
}