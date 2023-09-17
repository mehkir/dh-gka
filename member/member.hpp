#ifndef MEMBER
#define MEMBER

#include "multicast_application_impl.hpp"
#include "str_key_tree.hpp"
#include "primitives.hpp"
#include "key_agreement_protocol.hpp"
#include <cryptopp/dh.h>
#include <cryptopp/osrng.h>

#include <set>
#include <tuple>
#include <mutex>

class member : public multicast_application_impl {
// Variables
private:
    std::mutex receive_mutex_;
    std::unique_ptr<key_agreement_protocol> key_agreement_protocol_;
// Methods
public:
    member(std::unique_ptr<key_agreement_protocol> _key_agreement_protocol);
    ~member();
    virtual void received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) override;
    void send(message& _message);
    void start();
private:
    message_id_t extract_message_id(boost::asio::streambuf& buffer);
    void process_find(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_offer(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_request(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
    void process_response(boost::asio::streambuf& buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
};

#endif