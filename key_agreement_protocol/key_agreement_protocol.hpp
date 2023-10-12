#ifndef KEY_AGREEMENT_PROTOCOL
#define KEY_AGREEMENT_PROTOCOL

#include "../message_handler/message.hpp"

class key_agreement_protocol {
private:

public:
    virtual ~key_agreement_protocol() {}
    virtual void process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) = 0;
    virtual void process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) = 0;
    virtual void process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) = 0;
    virtual void process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) = 0;
    virtual void process_member_info_request(member_info_request_message _rcvd_member_info_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) = 0;
    virtual void process_member_info_response(member_info_response_message _rcvd_member_info_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) = 0;
    virtual void process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) = 0;
};

#endif