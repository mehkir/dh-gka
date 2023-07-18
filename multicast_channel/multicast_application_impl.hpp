#ifndef STR_DH_MULTICAST_APPLICATION_IMPL
#define STR_DH_MULTICAST_APPLICATION_IMPL

#include <memory>
#include "multicast_channel.hpp"

class multicast_application_impl : public multicast_application {
    public:
      multicast_application_impl();
      ~multicast_application_impl();
    protected:
      void send(std::string message);
      void start();
    private:
      std::unique_ptr<multicast_channel> multicast_channel_;
      boost::asio::io_service io_service_;
};

#endif