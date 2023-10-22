#ifndef DISTRIBUTED_DH
#define DISTRIBUTED_DH

#include "key_agreement_protocol.hpp"
#include "primitives.hpp"
#include "message_handler.hpp"
#include "multicast_application_impl.hpp"
#include "statistics_recorder.hpp"

#include <cryptopp/dh.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/osrng.h>
#include <unordered_map>
#include <unordered_set>

class distributed_dh : public key_agreement_protocol, public multicast_application_impl {
    // Variables
    public:
    protected:
    private:
#ifdef DEFAULT_DH
        CryptoPP::DH diffie_hellman_;
#elif defined(ECC_DH)
        CryptoPP::ECDH<CryptoPP::ECP>::Domain diffie_hellman_;
#endif
        std::mutex receive_mutex_;
        service_id_t service_of_interest_ = DEFAULT_SERVICE_ID;
        bool is_sponsor_;
        CryptoPP::AutoSeededRandomPool rnd_;
        secret_t group_secret_;
        secret_t secret_;
        blinded_secret_t blinded_secret_;
        std::unique_ptr<message_handler> message_handler_;
        std::uint32_t member_count_;
        std::unique_ptr<statistics_recorder> statistics_recorder_;
        std::chrono::milliseconds scatter_delay_;
        boost::asio::steady_timer scatter_timer_;
        boost::asio::steady_timer non_acked_responses_timer_;
        std::unordered_map<boost::asio::ip::udp::endpoint, std::unique_ptr<distributed_response_message>> non_acked_responses_;
        std::unordered_set<boost::asio::ip::udp::endpoint> endpoints_acks_rcvd_from_;
    // Methods
    public:
        distributed_dh(bool _is_sponsor, service_id_t _service_id, std::uint32_t _member_count, std::uint32_t _scatter_delay_min, std::uint32_t _scatter_delay_max);
        ~distributed_dh();
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
        virtual void process_finish(finish_message _rcvd_finish_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
        virtual void process_finish_ack(finish_ack_message _rcvd_finish_ack_message, boost::asio::ip::udp::endpoint _remote_endpoint) override;
    protected:
    private:
        bool group_secret_rcvd();
        void send_cyclic_offer();
        void send_cyclic_non_acked_responses();
        void send_multicast(message& _message);
        void send_to(message& _message, boost::asio::ip::udp::endpoint _remote_endpoint);
        std::string short_secret_repr(secret_t _secret);
        std::chrono::milliseconds compute_scatter_delay(std::uint32_t _scatter_delay_min, std::uint32_t _scatter_delay_max);
};

#endif