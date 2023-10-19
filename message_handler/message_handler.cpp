#include "message_handler.hpp"
#include "logger.hpp"

message_handler::message_handler(key_agreement_protocol* _key_agreement_protocol) : key_agreement_protocol_(_key_agreement_protocol) {
}

message_handler::~message_handler() {

}

void message_handler::deserialize_and_callback(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) {
    boost::asio::streambuf buffer;
    write_to_streambuf(buffer, reinterpret_cast<const char*>(_data), _bytes_recvd);
    switch (extract_message_id(buffer))
    {
    case message_type::FIND: {
        process_find(buffer, _remote_endpoint);
    }
        break;
    case message_type::OFFER: {
        process_offer(buffer, _remote_endpoint);
    }
        break;
    case message_type::REQUEST: {
        process_request(buffer, _remote_endpoint);
    }
        break;
    case message_type::RESPONSE: {
        process_response(buffer, _remote_endpoint);
    }
        break;
    case message_type::MEMBER_INFO_REQUEST: {
        process_member_info_request(buffer, _remote_endpoint);
    }
        break;
    case message_type::MEMBER_INFO_RESPONSE: {
        process_member_info_response(buffer, _remote_endpoint);
    }
        break;
    case message_type::SYNCH_TOKEN: {
        process_synch_token(buffer, _remote_endpoint);
    }
        break;
    case message_type::MEMBER_INFO_SYNCH_REQUEST: {
        process_member_info_synch_request(buffer, _remote_endpoint);
    }
        break;
    case message_type::MEMBER_INFO_SYNCH_RESPONSE: {
        process_member_info_synch_response(buffer, _remote_endpoint);
    }
        break;
    case message_type::DISTRIBUTED_RESPONSE: {
        process_distributed_response(buffer, _remote_endpoint);
    }
        break;
    default:
        std::cerr << "Unknown message type received" << std::endl;
        break;
    }
}

message_id_t message_handler::extract_message_id(boost::asio::streambuf& buffer) {
    message_id_t message_id;
    read_from_streambuf(buffer, reinterpret_cast<char*>(&message_id), MESSAGE_ID_SIZE);
    return message_id;
}

void message_handler::process_find(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    find_message rcvd_find_message;
    rcvd_find_message.deserialize_(buffer);
    key_agreement_protocol_->process_find(rcvd_find_message, _remote_endpoint);
}

void message_handler::process_offer(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    offer_message rcvd_offer_message;
    rcvd_offer_message.deserialize_(buffer);
    key_agreement_protocol_->process_offer(rcvd_offer_message, _remote_endpoint);
}

void message_handler::process_request(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    request_message rcvd_request_message;
    rcvd_request_message.deserialize_(buffer);
    key_agreement_protocol_->process_request(rcvd_request_message, _remote_endpoint);
}

void message_handler::process_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    response_message rcvd_response_message;
    rcvd_response_message.deserialize_(buffer);
    key_agreement_protocol_->process_response(rcvd_response_message, _remote_endpoint);
}

void message_handler::process_member_info_request(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    member_info_request_message rcvd_member_info_request_message;
    rcvd_member_info_request_message.deserialize_(buffer);
    key_agreement_protocol_->process_member_info_request(rcvd_member_info_request_message, _remote_endpoint);
}

void message_handler::process_member_info_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    member_info_response_message rcvd_member_info_response_message;
    rcvd_member_info_response_message.deserialize_(buffer);
    key_agreement_protocol_->process_member_info_response(rcvd_member_info_response_message, _remote_endpoint);
}

void message_handler::process_synch_token(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    synch_token_message rcvd_synch_token_message;
    rcvd_synch_token_message.deserialize_(buffer);
    key_agreement_protocol_->process_synch_token(rcvd_synch_token_message, _remote_endpoint);
}

void message_handler::process_member_info_synch_request(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    member_info_synch_request_message rcvd_member_info_synch_request_message;
    rcvd_member_info_synch_request_message.deserialize_(buffer);
    key_agreement_protocol_->process_member_info_synch_request(rcvd_member_info_synch_request_message, _remote_endpoint);
}

void message_handler::process_member_info_synch_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    member_info_synch_response_message rcvd_member_info_synch_response_message;
    rcvd_member_info_synch_response_message.deserialize_(buffer);
    key_agreement_protocol_->process_member_info_synch_response(rcvd_member_info_synch_response_message, _remote_endpoint);
}

void message_handler::process_distributed_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint) {
    distributed_response_message rcvd_distributed_response_message;
    rcvd_distributed_response_message.deserialize_(buffer);
    key_agreement_protocol_->process_distributed_response(rcvd_distributed_response_message, _remote_endpoint);
}

void message_handler::serialize(message& _message, boost::asio::streambuf& _buffer) {
    write_to_streambuf(_buffer, reinterpret_cast<const char*>(&_message.message_type_), MESSAGE_ID_SIZE);
    _message.serialize_(_buffer);
}
