#ifndef MESSAGE_HANDLER
#define MESSAGE_HANDLER

#include "key_agreement_protocol.hpp"

#include <mutex>

class message_handler {
// Variables
private:
    key_agreement_protocol* key_agreement_protocol_;
// Methods
public:
    message_handler(key_agreement_protocol* _key_agreement_protocol);
    ~message_handler();
    void deserialize_and_callback(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint);
    void serialize(message& _message, boost::asio::streambuf& _buffer);
private:
    message_id_t extract_message_id(boost::asio::streambuf& buffer);
    void process_find(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_offer(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_request(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_member_info_request(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_member_info_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_synch_token(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_member_info_synch_request(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_member_info_synch_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_distributed_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
};

#endif