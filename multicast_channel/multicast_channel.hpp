#ifndef MULTICAST_CHANNEL
#define MULTICAST_CHANNEL

#include "multicast_application.hpp"

#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

class multicast_channel
{

private:
/* Member variables*/
  boost::asio::ip::udp::socket unicast_socket_;
  boost::asio::ip::udp::socket multicast_socket_;
  boost::asio::ip::udp::endpoint multicast_endpoint_;
  boost::asio::ip::udp::endpoint unicast_remote_endpoint_;
  boost::asio::ip::udp::endpoint multicast_remote_endpoint_;
  std::string message_;
  enum { max_length = 1024 };
  unsigned char unicast_data_[max_length];
  unsigned char multicast_data_[max_length];
  multicast_application& mc_app_;
private:
/* Methods */
  void handle_send_to(const boost::system::error_code& _error);
  void receive_multicast();
  void receive_unicast();
  void handle_multicast_receive_from(const boost::system::error_code& _error, size_t _bytes_recvd);
  void handle_unicast_receive_from(const boost::system::error_code& _error, size_t _bytes_recvd);

public:
  multicast_channel(boost::asio::io_service& _io_service,
    const boost::asio::ip::address& _listen_interface_by_address,
    const boost::asio::ip::address& _multicast_address,
    int _multicast_port, multicast_application& _mc_app);
  ~multicast_channel();
  void send_multicast(boost::asio::streambuf& _buffer);
  void send_to(boost::asio::streambuf& _buffer, boost::asio::ip::udp::endpoint _remote_endpoint);
  boost::asio::ip::udp::endpoint get_local_endpoint() const;
};
#endif