#ifndef MULTICAST_APPLICATION_IMPL
#define MULTICAST_APPLICATION_IMPL

#include <memory>
#include "multicast_channel.hpp"

class multicast_application_impl : public multicast_application {
    public:
      multicast_application_impl();
      ~multicast_application_impl();
    protected:
      void send_multicast(boost::asio::streambuf& _buffer);
      void send_to(boost::asio::streambuf& _buffer, boost::asio::ip::udp::endpoint _endpoint);
      void start();
      boost::asio::ip::udp::endpoint get_local_endpoint() const;
    private:
      boost::asio::io_service io_service_; // MUST be listed BEFORE unique_ptr
      std::unique_ptr<multicast_channel> multicast_channel_;
};

#endif