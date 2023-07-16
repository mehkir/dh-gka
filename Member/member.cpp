#include "member.hpp"

using namespace boost::placeholders;

member::member(boost::asio::io_service& io_service,
      const boost::asio::ip::address& multicast_address,
      short multicast_port, bool ping)
    :
      multicast_endpoint_(multicast_address, multicast_port),
      send_socket_(io_service, multicast_endpoint_.protocol()),
      multicast_port_(multicast_port),
      ping_(ping)
{
    if (ping) {
        boost::system::error_code dummy;
        send(dummy);
    }
}

member::~member()
{
}

void member::send(const boost::system::error_code& error) {
    if (!error)
    {
      std::ostringstream os;
      os << "Ping";
      message_ = os.str();

      send_socket_.async_send_to(
          boost::asio::buffer(message_), multicast_endpoint_,
          boost::bind(&member::handle_send_to, this,
            boost::asio::placeholders::error));
    }
}

  void handle_send_to(const boost::system::error_code& error)
  {
    // ...
  }