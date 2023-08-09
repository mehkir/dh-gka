#ifndef STR_DH_MEMBER
#define STR_DH_MEMBER

#include "multicast_application_impl.hpp"
#include "str_key_tree.hpp"
#include "primitives.hpp"
#include "message.hpp"
#include <cryptopp/dh.h>
#include <cryptopp/osrng.h>
#include <map>
#include <set>
#include <tuple>
#include <mutex>

#define DEFAULT_MEMBER_ID -1
#define DEFAULT_VALUE -1

typedef CryptoPP::Integer blinded_secret_int_t;
typedef CryptoPP::Integer secret_int_t;

struct sorted_member_entry {
    member_id_t member_id_;
    boost::asio::ip::udp::endpoint endpoint_;
    blinded_secret_int_t blinded_secret_;
    
    friend bool operator<(const sorted_member_entry &lhs, const sorted_member_entry &rhs) {
	    return lhs.member_id_ < rhs.member_id_;
    }
};

class member : public multicast_application_impl {
// Variables
private:
    bool is_sponsor_;
    int keys_computed_count_;
    CryptoPP::DH diffie_hellman_;
    CryptoPP::AutoSeededRandomPool rnd_;
    CryptoPP::SecByteBlock secret_;
    CryptoPP::SecByteBlock blinded_secret_;
    CryptoPP::Integer secret_int_;
    CryptoPP::Integer blinded_secret_int_;
    std::map<service_id_t, std::unique_ptr<str_key_tree>> str_key_tree_map_;
    std::map<std::tuple<boost::asio::ip::udp::endpoint, service_id_t>, blinded_secret_int_t> pending_requests_;
    std::map<service_id_t, std::set<sorted_member_entry>> assigned_members_map_;
    std::mutex receive_mutex_;

    //service_id_t required_service_ = -1;
    //service_id_t offered_service_ = -1;
    service_id_t service_of_interest_ = DEFAULT_VALUE;
    member_id_t member_id_ = DEFAULT_MEMBER_ID;
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