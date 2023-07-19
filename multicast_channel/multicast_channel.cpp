#include "multicast_channel.hpp"

using namespace boost::placeholders;

multicast_channel::multicast_channel(boost::asio::io_service& _io_service,
      const boost::asio::ip::address& _listen_interface_by_address,
      const boost::asio::ip::address& _multicast_address,
      int _multicast_port, multicast_application& _mc_app)
    :
      multicast_endpoint_(_multicast_address, _multicast_port),
      multicast_socket_(_io_service),
      unicast_socket_(_io_service),
      mc_app_(_mc_app) {
    // Create the multicast socket and bind it to the multicast address and port
    boost::asio::ip::udp::endpoint listen_endpoint(
        _listen_interface_by_address, _multicast_port);
    multicast_socket_.open(boost::asio::ip::udp::v4());
    multicast_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    multicast_socket_.bind(listen_endpoint);
    // Join the multicast group.
    multicast_socket_.set_option(boost::asio::ip::multicast::join_group(_multicast_address));
    // Create the unicast socket and bind it to an open port
    unicast_socket_.open(boost::asio::ip::udp::v4());
    unicast_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    unicast_socket_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
    receive_multicast();
    receive_unicast();
}

multicast_channel::~multicast_channel() {
}

void multicast_channel::send(std::string _message) {
  std::ostringstream os;
  os << _message;
  message_ = os.str();

  unicast_socket_.async_send_to(
      boost::asio::buffer(message_), multicast_endpoint_,
      boost::bind(&multicast_channel::handle_send_to, this,
        boost::asio::placeholders::error));
}

void multicast_channel::handle_send_to(const boost::system::error_code& _error) {
  if (!_error) {
    // ...
  }
}

void multicast_channel::receive_multicast() {
  multicast_socket_.async_receive_from(
  boost::asio::buffer(multicast_data_, max_length), remote_endpoint_,
  boost::bind(&multicast_channel::handle_multicast_receive_from, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void multicast_channel::receive_unicast() {
  unicast_socket_.async_receive_from(
  boost::asio::buffer(unicast_data_, max_length), remote_endpoint_,
  boost::bind(&multicast_channel::handle_unicast_receive_from, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void multicast_channel::handle_multicast_receive_from(const boost::system::error_code& _error, size_t _bytes_recvd) {
  if (!_error)
  {
    if (unicast_socket_.local_endpoint().port() != remote_endpoint_.port()) {
      mc_app_.received_data(multicast_data_, _bytes_recvd);
      receive_multicast();
    }
  }
}

void multicast_channel::handle_unicast_receive_from(const boost::system::error_code& error, size_t bytes_recvd) {
  if (!error)
  {
    if (unicast_socket_.local_endpoint().port() != remote_endpoint_.port()) {
      mc_app_.received_data(unicast_data_, bytes_recvd);
      receive_unicast();
    }
  }
}