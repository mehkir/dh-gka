#ifndef STR_DH_MULTICAST_CHANNEL
#define STR_DH_MULTICAST_CHANNEL

#include <iostream>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

class multicast_application;

class multicast_channel
{

private:
/* Member variables*/
  boost::asio::ip::udp::socket send_socket_;
  boost::asio::ip::udp::socket multicast_socket_;
  boost::asio::ip::udp::endpoint multicast_endpoint_;
  boost::asio::ip::udp::endpoint remote_endpoint_;
  std::string message_;
  enum { max_length = 1024 };
  char data_[max_length];
  multicast_application& mc_app_;
private:
/* Methods */
  void handle_send_to(const boost::system::error_code& error);
  void receive();
  void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd);

public:
  multicast_channel(boost::asio::io_service& io_service,
    const boost::asio::ip::address& listen_interface_by_address,
    const boost::asio::ip::address& multicast_address,
    int multicast_port, multicast_application& mc_app);
  ~multicast_channel();
  void send(std::string message);
};
#endif