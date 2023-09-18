#include "message_handler.hpp"
#include "logger.hpp"

using namespace boost::placeholders;

message_handler::message_handler(std::unique_ptr<key_agreement_protocol> _key_agreement_protocol) : key_agreement_protocol_(_key_agreement_protocol) {

    boost::function<void(message)> send_callback_ = boost::bind(&message_handler::send, this, _1);
    LOG_DEBUG("[<message_handler>]: initialization complete, IP=" << get_local_endpoint().address().to_string() << " Port=" << get_local_endpoint().port())
}

message_handler::~message_handler() {
    LOG_DEBUG("[<message_handler>]: destruction complete")
}

void message_handler::received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) {
    std::lock_guard<std::mutex> lock_receive(receive_mutex_);
    boost::asio::streambuf buffer;
    write_to_streambuf(buffer, reinterpret_cast<const char*>(_data), _bytes_recvd);
    switch (extract_message_id(buffer))
    {
    case message_type::FIND: {
        LOG_DEBUG("[<message_handler>]: received data, FIND case")
        process_find(buffer, _remote_endpoint);
    }
        break;
    case message_type::OFFER: {
        LOG_DEBUG("[<message_handler>]: received data, OFFER case")
        process_offer(buffer, _remote_endpoint);
    }
        break;
    case message_type::REQUEST: {
        LOG_DEBUG("[<message_handler>]: received data, REQUEST case")
        process_request(buffer, _remote_endpoint);
    }
        break;
    case message_type::RESPONSE: {
        LOG_DEBUG("[<message_handler>]: received data, RESPONSE case")
        process_response(buffer, _remote_endpoint);
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
    key_agreement_protocol_->(rcvd_response_message, _remote_endpoint);
}

void message_handler::send(message& _message) {
    boost::asio::streambuf buffer;
    write_to_streambuf(buffer, reinterpret_cast<const char*>(&_message.message_type_), MESSAGE_ID_SIZE);
    _message.serialize_(buffer);
    multicast_application_impl::send(buffer);
}

void message_handler::start() {
    multicast_application_impl::start();
}
