#ifndef STR_DH_MEMBER
#define STR_DH_MEMBER

#include "multicast_application_impl.hpp"

class member : public multicast_application_impl {
private:

public:
    member();
    ~member();
    void received_data(void* data, std::size_t bytes_recvd);
    void send(std::string message);
    void member::start();

};

#endif