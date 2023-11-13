#ifndef MULTICAST_APPLICATION_IMPL
#define MULTICAST_APPLICATION_IMPL

#include <memory>
#include "multicast_channel.hpp"

class multicast_application_impl : public multicast_application {
    public:
      multicast_application_impl(boost::asio::ip::address _listening_interface_by_ip, boost::asio::ip::address _multicast_ip, std::uint16_t _multicast_port);
      ~multicast_application_impl();
    protected:
      void send_multicast(boost::asio::streambuf& _buffer);
      void send_to(boost::asio::streambuf& _buffer, boost::asio::ip::udp::endpoint _endpoint);
      void start();
      void stop();
      boost::asio::io_service& get_io_service();
      boost::asio::ip::udp::endpoint get_local_endpoint() const;
    private:
      boost::asio::io_service io_service_; // MUST be listed BEFORE unique_ptr
      std::unique_ptr<multicast_channel> multicast_channel_;
};

#endif