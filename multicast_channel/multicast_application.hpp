#ifndef MULTICAST_APPLICATION
#define MULTICAST_APPLICATION

#include <cstddef>
#include <boost/asio.hpp>

class multicast_application {
private:

public:
    virtual ~multicast_application() {}
    virtual void received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) = 0;
};

#endif