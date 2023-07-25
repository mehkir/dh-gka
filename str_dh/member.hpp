#ifndef STR_DH_MEMBER
#define STR_DH_MEMBER

#include "multicast_application_impl.hpp"
#include "str_key_tree.hpp"
#include "primitives.hpp"
#include "message.hpp"
#include <cryptopp/dh.h>
#include <cryptopp/osrng.h>
#include <map>
#include <tuple>
#include <mutex>

typedef CryptoPP::Integer blinded_secret_int_t;

class member : public multicast_application_impl {
// Variables
private:
    CryptoPP::DH diffie_hellman_;
    CryptoPP::AutoSeededRandomPool rnd_;
    CryptoPP::SecByteBlock secret_;
    CryptoPP::SecByteBlock blinded_secret_;
    CryptoPP::Integer secret_int_;
    CryptoPP::Integer blinded_secret_int_;
    bool is_sponsor_;
    std::map<service_id_t, std::unique_ptr<str_key_tree>> str_key_tree_map_;
    std::map<std::tuple<boost::asio::ip::udp::endpoint, service_id_t>, blinded_secret_int_t> member_blinded_secrets_cache_;
    std::map<service_id_t, member_count_t> member_count_;
    std::mutex receive_mutex_;

    //service_id_t required_service_ = -1;
    //service_id_t offered_service_ = -1;
    service_id_t service_of_interest_ = -1;
    member_id_t member_id_ = -1;
// Methods
public:
    member(bool _is_sponsor, service_id_t _service_id);
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