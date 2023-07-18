#ifndef STR_DH_MEMBER
#define STR_DH_MEMBER

#include "multicast_application_impl.hpp"
#include <cryptopp/dh.h>
#include <cryptopp/osrng.h>

class member : public multicast_application_impl {
private:
    CryptoPP::DH diffie_hellman_;
    CryptoPP::AutoSeededRandomPool rnd_;
    CryptoPP::SecByteBlock secret_;
    CryptoPP::SecByteBlock blinded_secret_;
    CryptoPP::Integer secret_int_;
    CryptoPP::Integer blinded_secret_int_;
public:
    member();
    ~member();
    void received_data(void* data, std::size_t bytes_recvd);
    void send(std::string message);
    void member::start();

};

#endif