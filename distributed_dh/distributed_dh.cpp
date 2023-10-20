#include "distributed_dh.hpp"
#include "MODP2048_256sg.hpp"

#include <cryptopp/nbtheory.h>
#include <cryptopp/modes.h>
#include <cryptopp/oids.h>
#include <cryptopp/asn.h>

distributed_dh::distributed_dh(bool _is_sponsor, service_id_t _service_id) : is_sponsor_(_is_sponsor), service_of_interest_(_service_id), message_handler_(std::make_unique<message_handler>(this)) {
#ifdef DEFAULT_DH
    diffie_hellman_.AccessGroupParameters().Initialize(P, Q, G);
    LOG_DEBUG("[<distributed_dh>] Using default DH")
#elif defined(ECC_DH)
    diffie_hellman_.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());
    LOG_DEBUG("[<distributed_dh>] Using ECDH")
#endif
    secret_.New(diffie_hellman_.PrivateKeyLength());
    blinded_secret_.New(diffie_hellman_.PublicKeyLength());
    diffie_hellman_.GeneratePrivateKey(rnd_, secret_);
    diffie_hellman_.GeneratePublicKey(rnd_, secret_, blinded_secret_);
    group_secret_.New(diffie_hellman_.AgreedValueLength());

    if (is_sponsor_) {
        member_id_ = 1;
        diffie_hellman_.GeneratePrivateKey(rnd_, group_secret_);

        LOG_DEBUG("[<distributed_dh>]: generated group secret " << short_secret_repr(group_secret_))

        std::unique_ptr<offer_message> initial_offer = std::make_unique<offer_message>();
        initial_offer->offered_service_ = service_of_interest_;
        send_multicast(initial_offer.operator*());
    } else {
        std::unique_ptr<find_message> initial_find = std::make_unique<find_message>();
        initial_find->required_service_ = service_of_interest_;
        send_multicast(initial_find.operator*());
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
        send_multicast(offer.operator*());
    }
}

void distributed_dh::process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    if (!is_assigned() && _rcvd_offer_message.offered_service_ == service_of_interest_) {
        std::unique_ptr<request_message> request = std::make_unique<request_message>();
        request->blinded_secret_ = blinded_secret_;
        request->required_service_ = service_of_interest_;
        send_to(request.operator*(), _remote_endpoint);
    }
}

void distributed_dh::process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    secret_t shared_secret(diffie_hellman_.AgreedValueLength());
    diffie_hellman_.Agree(shared_secret, secret_, _rcvd_request_message.blinded_secret_);

    // Calculate a SHA-256 hash over the Diffie-Hellman session key
    CryptoPP::SecByteBlock key(CryptoPP::SHA256::DIGESTSIZE);
    CryptoPP::SHA256().CalculateDigest(key, shared_secret, shared_secret.SizeInBytes()); 
    // Generate a random IV
    CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
    rnd_.GenerateBlock(iv, CryptoPP::AES::BLOCKSIZE);
    std::vector<unsigned char> iv_vector(iv, iv + CryptoPP::AES::BLOCKSIZE);

    CryptoPP::SecByteBlock encrypted_group_key(group_secret_.SizeInBytes());

    // Encrypt
    CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption cfbEncryption(key, CryptoPP::SHA256::DIGESTSIZE, iv);
    cfbEncryption.ProcessData(encrypted_group_key.BytePtr(), group_secret_.BytePtr(), group_secret_.SizeInBytes());

    std::unique_ptr<distributed_response_message> distributed_response = std::make_unique<distributed_response_message>();
    distributed_response->offered_service_ = service_of_interest_;
    distributed_response->blinded_sponsor_secret_ = blinded_secret_;
    distributed_response->encrypted_group_secret_ = encrypted_group_key;
    distributed_response->initialization_vector_ = iv_vector;
    send_to(distributed_response.operator*(), _remote_endpoint);
}

void distributed_dh::process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    CryptoPP::SecByteBlock encrypted_group_secret_ = _rcvd_distributed_response_message.encrypted_group_secret_;

    secret_t shared_secret(diffie_hellman_.AgreedValueLength());
    diffie_hellman_.Agree(shared_secret, secret_, _rcvd_distributed_response_message.blinded_sponsor_secret_);

    // Calculate a SHA-256 hash over the Diffie-Hellman session key
    CryptoPP::SecByteBlock key(CryptoPP::SHA256::DIGESTSIZE);
    CryptoPP::SHA256().CalculateDigest(key, shared_secret, shared_secret.size()); 

    CryptoPP::SecByteBlock decrypted_group_key(diffie_hellman_.AgreedValueLength());

    // Decrypt
    CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption cfbDecryption(key, CryptoPP::SHA256::DIGESTSIZE, _rcvd_distributed_response_message.initialization_vector_.data());
    cfbDecryption.ProcessData(decrypted_group_key.BytePtr(), encrypted_group_secret_.BytePtr(), encrypted_group_secret_.SizeInBytes());

    group_secret_ = decrypted_group_key;

    LOG_DEBUG("[<distributed_dh>]: received group secret " << short_secret_repr(group_secret_))
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

bool distributed_dh::is_assigned() {
    return member_id_ != DEFAULT_MEMBER_ID;
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
    // Unused, just here to comply with key_agreement_protocol
}

void distributed_dh::process_finish_ack(finish_ack_message _rcvd_finish_ack_message, boost::asio::ip::udp::endpoint _remote_endpoint) {
    // Unused, just here to comply with key_agreement_protocol
}