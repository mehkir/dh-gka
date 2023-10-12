#ifndef DISTRIBUTED_DH
#define DISTRIBUTED_DH

#include "key_agreement_protocol.hpp"
#include "primitives.hpp"
#include "message_handler.hpp"
#include "multicast_application_impl.hpp"

#include <cryptopp/dh.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/osrng.h>
#include <unordered_map>
#include <set>
#include <tuple>

#define DEFAULT_MEMBER_ID -1
#define DEFAULT_VALUE -1

typedef CryptoPP::SecByteBlock blinded_secret_t;
typedef CryptoPP::SecByteBlock secret_t;

class distributed_dh : public key_agreement_protocol, public multicast_application_impl {
    // Variables
    public:
    protected:
    private:
#ifndef ECC_DH
        CryptoPP::DH diffie_hellman_;
#else
        CryptoPP::ECDH<CryptoPP::ECP>::Domain diffie_hellman_;
#endif
        std::mutex receive_mutex_;
        service_id_t service_of_interest_ = DEFAULT_VALUE;
        member_id_t member_id_ = DEFAULT_MEMBER_ID;
        bool is_sponsor_;
        CryptoPP::AutoSeededRandomPool rnd_;
        secret_t group_secret_;
        secret_t secret_;
        blinded_secret_t blinded_secret_;
        std::unique_ptr<message_handler> message_handler_;
    // Methods
    public:
        distributed_dh(bool _is_sponsor, service_id_t _service_id);
        ~distributed_dh();
        void start();
        virtual void received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_member_info_request(member_info_request_message _rcvd_member_info_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_member_info_response(member_info_response_message _rcvd_member_info_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
    protected:
    private:
        bool is_assigned();
        void send_multicast(message& _message);
        void send_to(message& _message, boost::asio::ip::udp::endpoint _remote_endpoint);
        std::string short_secret_repr(secret_t _secret);
};

#endif