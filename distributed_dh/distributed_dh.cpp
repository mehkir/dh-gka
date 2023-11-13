#include "distributed_dh.hpp"
#include "MODP2048_256sg.hpp"

#include <unistd.h>
#include <random>
#include <cryptopp/nbtheory.h>
#include <cryptopp/modes.h>
#include <cryptopp/oids.h>
#include <cryptopp/asn.h>

distributed_dh::distributed_dh(bool _is_sponsor, service_id_t _service_id,  std::uint32_t _member_count, std::uint32_t _scatter_delay_min, std::uint32_t _scatter_delay_max, std::string _listening_interface_by_ip, std::string _multicast_ip, std::uint16_t _port) : is_sponsor_(_is_sponsor), service_of_interest_(_service_id), member_count_(_member_count), multicast_application_impl(_listening_interface_by_ip, _multicast_ip, _port), message_handler_(std::make_unique<message_handler>(this)), statistics_recorder_(statistics_recorder::get_instance()), scatter_timer_(multicast_application_impl::get_io_service()), timeout_timer_(multicast_application_impl::get_io_service()) {
    if (is_sponsor_) {
        statistics_recorder_->record_timestamp(time_metric::DURATION_START_);
    }
#ifdef RETRANSMISSIONS
    scatter_delay_ = compute_scatter_delay(_scatter_delay_min, _scatter_delay_max);
#endif
#ifdef DEFAULT_DH
    diffie_hellman_.AccessGroupParameters().Initialize(P, Q, G);
    LOG_DEBUG("[<distributed_dh>]: Using default DH")
#elif defined(ECC_DH)
    diffie_hellman_.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());
    LOG_DEBUG("[<distributed_dh>]: Using ECDH")
#endif
    secret_.New(diffie_hellman_.PrivateKeyLength());
    blinded_secret_.New(diffie_hellman_.PublicKeyLength());
    diffie_hellman_.GeneratePrivateKey(rnd_, secret_); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);
    diffie_hellman_.GeneratePublicKey(rnd_, secret_, blinded_secret_); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);

    non_acked_responses_.clear();
    endpoints_acks_rcvd_from_.clear();

    statistics_recorder_->record_count(count_metric::MEMBER_COUNT_);

    if (is_sponsor_) {
        group_secret_.New(diffie_hellman_.AgreedValueLength());
        diffie_hellman_.GeneratePrivateKey(rnd_, group_secret_); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);

        LOG_STD("[<distributed_dh>]: pid=" << getpid() << " generated group secret " << short_secret_repr(group_secret_))

        std::unique_ptr<offer_message> initial_offer = std::make_unique<offer_message>();
        initial_offer->offered_service_ = service_of_interest_;
        send_multicast(initial_offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);
#ifdef RETRANSMISSIONS
        send_cyclic_messages();
#endif
    } else {
        // std::unique_ptr<find_message> initial_find = std::make_unique<find_message>();
        // initial_find->required_service_ = service_of_interest_;
        // send_multicast(initial_find.operator*()); statistics_recorder_->record_count(count_metric::FIND_MESSAGE_COUNT_);
    }
}

distributed_dh::~distributed_dh() {

}

void distributed_dh::start() {
    multicast_application_impl::start();
}

void distributed_dh::received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) {
    std::lock_guard<std::mutex> lock_receive(receive_mutex_);
    if (get_local_endpoint().port() != _remote_endpoint.port()) {
        message_handler_->deserialize_and_callback(_data, _bytes_recvd, _remote_endpoint);
    }
}

void distributed_dh::process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (is_sponsor_ && service_of_interest_ == _rcvd_find_message.required_service_) {
        std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
        offer->offered_service_ = service_of_interest_;
        send_multicast(offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);
    }
}

void distributed_dh::process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!group_secret_rcvd() && _rcvd_offer_message.offered_service_ == service_of_interest_) {
        std::unique_ptr<request_message> request = std::make_unique<request_message>();
        request->blinded_secret_ = blinded_secret_;
        request->required_service_ = service_of_interest_;
        send_to(request.operator*(), _remote_endpoint); statistics_recorder_->record_count(count_metric::REQUEST_MESSAGE_COUNT_);
    }
}

void distributed_dh::process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (non_acked_responses_.size() + endpoints_acks_rcvd_from_.size() == 0) { statistics_recorder_->record_timestamp(time_metric::KEY_AGREEMENT_START_); }
    if (!non_acked_responses_.count(_remote_endpoint) && _rcvd_request_message.required_service_ == service_of_interest_) {
        secret_t shared_secret(diffie_hellman_.AgreedValueLength());
        diffie_hellman_.Agree(shared_secret, secret_, _rcvd_request_message.blinded_secret_); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);

        // Calculate a SHA-256 hash over the Diffie-Hellman session key
        CryptoPP::SecByteBlock key(CryptoPP::SHA256::DIGESTSIZE);
        CryptoPP::SHA256().CalculateDigest(key, shared_secret, shared_secret.SizeInBytes()); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);
        // Generate a random IV
        CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
        rnd_.GenerateBlock(iv, CryptoPP::AES::BLOCKSIZE);
        std::vector<CryptoPP::byte> iv_vector(iv, iv + CryptoPP::AES::BLOCKSIZE);

        CryptoPP::SecByteBlock encrypted_group_key(group_secret_.SizeInBytes());

        // Encrypt
        CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption cfbEncryption(key, CryptoPP::SHA256::DIGESTSIZE, iv);
        cfbEncryption.ProcessData(encrypted_group_key.BytePtr(), group_secret_.BytePtr(), group_secret_.SizeInBytes()); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);

        std::unique_ptr<distributed_response_message> distributed_response = std::make_unique<distributed_response_message>();
        distributed_response->offered_service_ = service_of_interest_;
        distributed_response->blinded_sponsor_secret_ = blinded_secret_;
        distributed_response->encrypted_group_secret_ = encrypted_group_key;
        distributed_response->initialization_vector_ = iv_vector;

        non_acked_responses_[_remote_endpoint] = std::make_unique<distributed_response_message>(distributed_response.operator*());

        send_to(distributed_response.operator*(), _remote_endpoint); statistics_recorder_->record_count(count_metric::DISTRIBUTED_RESPONSE_MESSAGE_COUNT_);
    }
}

void distributed_dh::process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!group_secret_rcvd() && _rcvd_distributed_response_message.offered_service_ == service_of_interest_) {
        CryptoPP::SecByteBlock encrypted_group_secret_ = _rcvd_distributed_response_message.encrypted_group_secret_;

        secret_t shared_secret(diffie_hellman_.AgreedValueLength());
        diffie_hellman_.Agree(shared_secret, secret_, _rcvd_distributed_response_message.blinded_sponsor_secret_); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);

        // Calculate a SHA-256 hash over the Diffie-Hellman session key
        CryptoPP::SecByteBlock key(CryptoPP::SHA256::DIGESTSIZE);
        CryptoPP::SHA256().CalculateDigest(key, shared_secret, shared_secret.size()); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);

        CryptoPP::SecByteBlock decrypted_group_key(diffie_hellman_.AgreedValueLength());

        // Decrypt
        CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption cfbDecryption(key, CryptoPP::SHA256::DIGESTSIZE, _rcvd_distributed_response_message.initialization_vector_.data());
        cfbDecryption.ProcessData(decrypted_group_key.BytePtr(), encrypted_group_secret_.BytePtr(), encrypted_group_secret_.SizeInBytes()); statistics_recorder_->record_count(count_metric::CRYPTO_OPERATIONS_COUNT_);

        group_secret_.New(diffie_hellman_.AgreedValueLength());
        group_secret_ = decrypted_group_key;

        LOG_DEBUG("[<distributed_dh>]: pid=" << getpid() << " received group secret " << short_secret_repr(group_secret_))
    }
    if (group_secret_rcvd() && _rcvd_distributed_response_message.offered_service_ == service_of_interest_) {
        std::unique_ptr<finish_ack_message> finish_ack = std::make_unique<finish_ack_message>();
        send_to(finish_ack.operator*(), _remote_endpoint); statistics_recorder_->record_count(count_metric::FINISH_ACK_MESSAGE_COUNT_);
#ifndef RETRANSMISSIONS
        contribute_statistics();
#endif
    }
}

void distributed_dh::send_cyclic_messages() {
    scatter_timer_.expires_from_now(scatter_delay_);
    scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
        if (!_error && (non_acked_responses_.size() + endpoints_acks_rcvd_from_.size() != member_count_-1)) {
            std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
            offer->offered_service_ = service_of_interest_;
            send_multicast(offer.operator*()); statistics_recorder_->record_count(count_metric::OFFER_MESSAGE_COUNT_);
        }
        if (!_error) {
            for (std::unordered_map<boost::asio::ip::udp::endpoint, std::unique_ptr<distributed_response_message>>::iterator itr = non_acked_responses_.begin(); itr != non_acked_responses_.end(); itr++) {
                std::unique_ptr<distributed_response_message> distributed_response = std::make_unique<distributed_response_message>(itr->second.operator*());
                send_to(distributed_response.operator*(), itr->first); statistics_recorder_->record_count(count_metric::DISTRIBUTED_RESPONSE_MESSAGE_COUNT_);
            }
        }
        if (!_error && (endpoints_acks_rcvd_from_.size() != member_count_-1)) {
            send_cyclic_messages();
        }
    });
}

void distributed_dh::send_multicast(message& _message) {
    boost::asio::streambuf buffer;
    message_handler_->serialize(_message, buffer);
    multicast_application_impl::send_multicast(buffer);
}

void distributed_dh::send_to(message& _message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    boost::asio::streambuf buffer;
    message_handler_->serialize(_message, buffer);
    multicast_application_impl::send_to(buffer, _remote_endpoint);
}

std::string distributed_dh::short_secret_repr(secret_t _secret) {
    CryptoPP::Integer secret_int;
    secret_int.Decode(_secret.BytePtr(), _secret.SizeInBytes());
    std::ostringstream oss;
    oss << secret_int;
    std::string secret_string = oss.str();
    oss.str("");
    oss << secret_string.substr(0,3) << "..." << secret_string.substr(secret_string.length()-4,3);
    return oss.str();
}

bool distributed_dh::group_secret_rcvd() {
    return group_secret_.SizeInBytes() != 0;
}

void distributed_dh::process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void distributed_dh::process_member_info_request(member_info_request_message _rcvd_member_info_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void distributed_dh::process_member_info_response(member_info_response_message _rcvd_member_info_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void distributed_dh::process_synch_token(synch_token_message _rcvd_synch_token_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void distributed_dh::process_member_info_synch_request(member_info_synch_request_message _rcvd_member_info_synch_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void distributed_dh::process_member_info_synch_response(member_info_synch_response_message _rcvd_member_info_synch_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}

void distributed_dh::process_finish(finish_message _rcvd_finish_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (_remote_endpoint != get_local_endpoint()) {
        contribute_statistics();
    }
    if (_remote_endpoint == get_local_endpoint()) {
        scatter_timer_.expires_from_now(scatter_delay_);
        scatter_timer_.async_wait([this](const boost::system::error_code& _error) {
            if (!_error) {
                std::unique_ptr<finish_message> finish = std::make_unique<finish_message>();
                send_multicast(finish.operator*()); // Message is not counted, since its only for triggering other members to contribute statistics and shut down
                std::unique_ptr<finish_message> self_msg = std::make_unique<finish_message>();
                process_finish(self_msg.operator*(), get_local_endpoint());
            }
        });
    }
}

void distributed_dh::process_finish_ack(finish_ack_message _rcvd_finish_ack_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    endpoints_acks_rcvd_from_.insert(_remote_endpoint);
    non_acked_responses_.erase(_remote_endpoint);
    if (is_sponsor_ && endpoints_acks_rcvd_from_.size() == member_count_-1) {
#ifdef RETRANSMISSIONS
        is_sponsor_ = false;
        statistics_recorder_->record_timestamp(time_metric::DURATION_END_);
        timeout_timer_.expires_from_now(std::chrono::seconds(TIMEOUT));
        timeout_timer_.async_wait([this](const boost::system::error_code& _error) {
            if (!_error) {
                scatter_timer_.cancel();
                contribute_statistics();
            }
        });
        std::unique_ptr<finish_message> finish = std::make_unique<finish_message>();
        send_multicast(finish.operator*()); // Message is not counted, since its only for triggering other members to contribute statistics and shut down
        std::unique_ptr<finish_message> self_msg = std::make_unique<finish_message>();
        process_finish(self_msg.operator*(), get_local_endpoint());
#else
    statistics_recorder_->record_timestamp(time_metric::DURATION_END_);
    contribute_statistics();
#endif
    }
}

void distributed_dh::contribute_statistics() {
    if (group_secret_rcvd()) {
        statistics_recorder_->contribute_statistics();
        multicast_application_impl::stop();
    }
}

std::chrono::milliseconds distributed_dh::compute_scatter_delay(std::uint32_t _scatter_delay_min, std::uint32_t _scatter_delay_max) {
    if (_scatter_delay_min > _scatter_delay_max) {
        const std::uint32_t tmp(_scatter_delay_min);
        _scatter_delay_min = _scatter_delay_max;
        _scatter_delay_max = tmp;
    }
    std::random_device random_device;
    std::mt19937 mersenne_twister(random_device());
    std::uniform_int_distribution<std::uint32_t> distribution(
            _scatter_delay_min, _scatter_delay_max);
    return std::chrono::milliseconds(distribution(mersenne_twister));
}