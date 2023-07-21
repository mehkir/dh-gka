#ifndef STR_DH_MULTICAST_APPLICATION_IMPL
#define STR_DH_MULTICAST_APPLICATION_IMPL

#include <memory>
#include "multicast_channel.hpp"

class multicast_application_impl : public multicast_application {
    public:
      multicast_application_impl();
      ~multicast_application_impl();
    protected:
      void send(boost::asio::streambuf& buffer);
      void start();
    private:
      boost::asio::io_service io_service_; // MUST be listed BEFORE unique_ptr
      std::unique_ptr<multicast_channel> multicast_channel_;
};

#endif