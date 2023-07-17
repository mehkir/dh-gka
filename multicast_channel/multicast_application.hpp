#ifndef STR_DH_MULTICAST_APPLICATION
#define STR_DH_MULTICAST_APPLICATION

#include <cstddef>
#include <memory>
#include "multicast_channel.hpp"

class multicast_application {
    public:
      multicast_application();
      ~multicast_application();
      virtual void received_data(void* data, std::size_t bytes_recvd) = 0;
    protected:
      void send(std::string message);
      void start();
    private:
      std::unique_ptr<multicast_channel> multicast_channel_;
      boost::asio::io_service io_service_;
};

#endif