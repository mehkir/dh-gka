#ifndef STR_DH
#define STR_DH

#include "key_agreement_protocol.hpp"
#include "str_key_tree.hpp"
#include "primitives.hpp"
#include "message_handler.hpp"
#include "multicast_application_impl.hpp"

#include <cryptopp/dh.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/osrng.h>
#include <unordered_map>
#include <set>
#include <tuple>

#define ECC_DH
#define DEFAULT_MEMBER_ID -1
#define DEFAULT_VALUE -1
#define DEFAULT_SECRET CryptoPP::SecByteBlock()

typedef CryptoPP::SecByteBlock blinded_secret_t;
typedef CryptoPP::SecByteBlock secret_t;

class str_dh : public key_agreement_protocol, public multicast_application_impl {
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
        int keys_computed_count_;
        CryptoPP::AutoSeededRandomPool rng_;
        secret_t secret_;
        blinded_secret_t blinded_secret_;
        std::unordered_map<service_id_t, std::unique_ptr<str_key_tree>> str_key_tree_map_;
        std::unordered_map<service_id_t, std::unordered_map<boost::asio::ip::udp::endpoint, blinded_secret_t>> pending_requests_;
        std::unordered_map<service_id_t, std::unordered_map<member_id_t,blinded_secret_t>> assigned_member_key_map_;
        std::unordered_map<service_id_t, std::unordered_map<boost::asio::ip::udp::endpoint,member_id_t>> assigned_member_endpoint_map_;
        std::unique_ptr<message_handler> message_handler_;
    // Methods
    public:
        str_dh(bool _is_sponsor, service_id_t _service_id);
        ~str_dh();
        void start();
        virtual void received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
    protected:
    private:
        void process_pending_request();
        blinded_secret_t get_next_blinded_key();
        std::pair<boost::asio::ip::udp::endpoint, blinded_secret_t> get_unassigned_member();
        std::unique_ptr<str_key_tree> build_str_tree(secret_t _group_secret, blinded_secret_t _blinded_group_secret,
                                                 secret_t _member_secret, blinded_secret_t _blinded_member_secret);
        void send(message& _message);
        bool is_assigned();
        bool all_predecessors_known();
        std::string short_secret_repr(secret_t _secret);
};

#endif