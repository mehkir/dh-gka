#include "member.hpp"
#include "MODP2048_256sg.hpp"

member::member() {
    diffie_hellman_.AccessGroupParameters().Initialize(p, q, g);
    secret_.New(diffie_hellman_.PrivateKeyLength());
    blinded_secret_.New(diffie_hellman_.PublicKeyLength());
    diffie_hellman_.GeneratePrivateKey(rnd_, secret_);
    diffie_hellman_.GeneratePublicKey(rnd_, secret_, blinded_secret_);
    secret_int_.Decode(secret_.BytePtr(), secret_.SizeInBytes());
    blinded_secret_int_.Decode(blinded_secret_.BytePtr(), blinded_secret_.SizeInBytes());
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