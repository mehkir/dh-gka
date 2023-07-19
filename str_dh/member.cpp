#include "member.hpp"
#include "MODP2048_256sg.hpp"

member::member(bool _is_sponsor, service_id_t _service_id) : is_sponsor_(_is_sponsor) {
    diffie_hellman_.AccessGroupParameters().Initialize(p, q, g);
    secret_.New(diffie_hellman_.PrivateKeyLength());
    blinded_secret_.New(diffie_hellman_.PublicKeyLength());
    diffie_hellman_.GeneratePrivateKey(rnd_, secret_);
    diffie_hellman_.GeneratePublicKey(rnd_, secret_, blinded_secret_);
    secret_int_.Decode(secret_.BytePtr(), secret_.SizeInBytes());
    blinded_secret_int_.Decode(blinded_secret_.BytePtr(), blinded_secret_.SizeInBytes());
    if (is_sponsor_) {
        offered_service_ = _service_id;
        member_id = 1;
        str_key_tree_map_[offered_service_].reset(new str_key_tree());
        str_key_tree_map_[offered_service_]->root_node.group_secret = secret_int_;
        str_key_tree_map_[offered_service_]->root_node.blinded_group_secret = blinded_secret_int_;
    } else {
        required_service_ = _service_id;
    }
}

member::~member() {
}

void member::received_data(void* data, std::size_t bytes_recvd) {
    std::cout.write(reinterpret_cast<const char*>(data), bytes_recvd);
    std::cout << std::endl;
}

void member::send(std::string message) {
    multicast_application_impl::send(message);
}

void member::start() {
    multicast_application_impl::start();
}