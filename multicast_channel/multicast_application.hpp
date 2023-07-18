#ifndef STR_DH_MULTICAST_APPLICATION
#define STR_DH_MULTICAST_APPLICATION

#include <cstddef>

class multicast_application
{
private:

public:
    virtual ~multicast_application() {}
    virtual void received_data(void* data, std::size_t bytes_recvd) = 0;
};

#endif