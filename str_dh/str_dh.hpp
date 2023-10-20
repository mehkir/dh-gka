#ifndef STR_DH
#define STR_DH

#include "key_agreement_protocol.hpp"
#include "str_key_tree.hpp"
#include "primitives.hpp"
#include "message_handler.hpp"
#include "multicast_application_impl.hpp"
#include "statistics_recorder.hpp"

#include <cryptopp/dh.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/osrng.h>
#include <unordered_map>
#include <set>
#include <tuple>

#define INITIAL_SPONSOR_ID 1

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
        service_id_t service_of_interest_ = DEFAULT_SERVICE_ID;
        member_id_t member_id_ = DEFAULT_MEMBER_ID;
        bool is_sponsor_;
        bool request_scheduled_;
        bool response_scheduled_;
        bool higher_member_id_synching_;
        bool higher_member_id_assigned_;
        bool synch_token_rcvd_;
        bool synch_finished_;
        int keys_computed_count_;
        CryptoPP::AutoSeededRandomPool rng_;
        secret_t secret_;
        blinded_secret_t blinded_secret_;
        std::unordered_map<service_id_t, std::unique_ptr<str_key_tree>> str_key_tree_map_;
        std::unordered_map<service_id_t, std::unordered_map<boost::asio::ip::udp::endpoint, blinded_secret_t>> pending_requests_;
        std::unordered_map<service_id_t, std::unordered_map<member_id_t,blinded_secret_t>> assigned_member_key_map_;
        std::unordered_map<service_id_t, std::unordered_map<boost::asio::ip::udp::endpoint,member_id_t>> assigned_member_endpoint_map_;
        std::unique_ptr<message_handler> message_handler_;
        std::unique_ptr<statistics_recorder> statistics_recorder_;
        std::uint32_t member_count_;
        std::chrono::milliseconds scatter_delay_; 
        boost::asio::steady_timer scatter_timer_;
        std::unique_ptr<response_message> response_message_cache_;
    // Methods
    public:
        str_dh(bool _is_sponsor, service_id_t _service_id, std::uint32_t _member_count, std::uint32_t _scatter_delay_min, std::uint32_t _scatter_delay_max);
        ~str_dh();
        void start();
        virtual void received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_member_info_request(member_info_request_message _rcvd_member_info_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_member_info_response(member_info_response_message _rcvd_member_info_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_synch_token(synch_token_message _rcvd_synch_token_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_member_info_synch_request(member_info_synch_request_message _rcvd_member_info_synch_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_member_info_synch_response(member_info_synch_response_message _rcvd_member_info_synch_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
    protected:
    private:
        void process_pending_request();
        void check_and_add_next_blinded_key_to_group_secret();
        blinded_secret_t get_next_blinded_key();
        std::pair<boost::asio::ip::udp::endpoint, blinded_secret_t> get_unassigned_member();
        std::unique_ptr<str_key_tree> build_str_tree(secret_t _group_secret, blinded_secret_t _blinded_group_secret,
                                                 secret_t _member_secret, blinded_secret_t _blinded_member_secret);
        void check_if_higher_member_id_assigned(boost::asio::ip::udp::endpoint _remote_endpoint);
        void send(message& _message);
        void send_cyclic_offer();
        void send_cyclic_response();
        void send_cyclic_member_info_request_predecessors();
        void send_member_info_request_predecessors();
        void send_cyclic_member_info_synch_request_successors();
        void send_member_info_synch_request_successors();
        void send_cyclic_synch_token();
        void send_synch_token_to_next_member();
        bool is_assigned();
        bool all_predecessors_known();
        bool all_successors_known();
        std::vector<member_id_t> get_unknown_predecessors();
        std::vector<member_id_t> get_unknown_successors();
        std::string short_secret_repr(secret_t _secret);
        void contribute_statistics();
        std::chrono::milliseconds compute_scatter_delay(std::uint32_t _scatter_delay_min, std::uint32_t _scatter_delay_max);
        template <typename T, typename R> void process_member_info_request_(T _rcvd_member_info_request_message, boost::asio::ip::udp::endpoint _remote_endpoint);
        template <typename T> void process_member_info_response_(T _rcvd_member_info_response_message, boost::asio::ip::udp::endpoint _remote_endpoint);
};

#endif