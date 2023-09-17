#ifndef STR_DH
#define STR_DH

#include "key_agreement_protocol.hpp"
#include "str_key_tree.hpp"
#include "primitives.hpp"
#include <cryptopp/dh.h>
#include <cryptopp/osrng.h>
#include <unordered_map>
#include <set>
#include <tuple>

#define DEFAULT_MEMBER_ID -1
#define DEFAULT_VALUE -1

typedef CryptoPP::Integer blinded_secret_int_t;
typedef CryptoPP::Integer secret_int_t;

class str_dh : public key_agreement_protocol {
    // Variables
    public:
    protected:
    private:
        service_id_t service_of_interest_ = DEFAULT_VALUE;
        member_id_t member_id_ = DEFAULT_MEMBER_ID;
        bool is_sponsor_;
        int keys_computed_count_;
        CryptoPP::DH diffie_hellman_;
        CryptoPP::AutoSeededRandomPool rnd_;
        CryptoPP::SecByteBlock secret_;
        CryptoPP::SecByteBlock blinded_secret_;
        CryptoPP::Integer secret_int_;
        CryptoPP::Integer blinded_secret_int_;
        std::unordered_map<service_id_t, std::unique_ptr<str_key_tree>> str_key_tree_map_;
        std::unordered_map<service_id_t, std::unordered_map<boost::asio::ip::udp::endpoint, blinded_secret_int_t>> pending_requests_;
        std::unordered_map<service_id_t, std::unordered_map<member_id_t,blinded_secret_int_t>> assigned_member_key_map_;
        std::unordered_map<service_id_t, std::unordered_map<boost::asio::ip::udp::endpoint,member_id_t>> assigned_member_endpoint_map_;
        boost::function<void(message)> send_callback_;
        boost::asio::ip::udp::endpoint local_endpoint_;
    // Methods
    public:
        str_dh(bool _is_sponsor, service_id_t _service_id);
        ~str_dh();
        virtual void process_find(find_message _find_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_offer(offer_message _offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_request(request_message _request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_response(response_message _response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
    protected:
    private:
        void process_pending_request();
        blinded_secret_int_t get_next_blinded_key();
        std::pair<boost::asio::ip::udp::endpoint, blinded_secret_int_t> get_unassigned_member();
        std::unique_ptr<str_key_tree> build_str_tree(CryptoPP::Integer _group_secret, CryptoPP::Integer _blinded_group_secret,
                                                 CryptoPP::Integer _member_secret, CryptoPP::Integer _blinded_member_secret);
}

#endif