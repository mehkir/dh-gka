#include "multicast_channel.hpp"
#include "multicast_application.hpp"

using namespace boost::placeholders;

multicast_channel::multicast_channel(boost::asio::io_service& io_service,
      const boost::asio::ip::address& listen_interface_by_address,
      const boost::asio::ip::address& multicast_address,
      int multicast_port, multicast_application& mc_app)
    :
      multicast_endpoint_(multicast_address, multicast_port),
      multicast_socket_(io_service),
      send_socket_(io_service, multicast_endpoint_.protocol()),
      mc_app_(mc_app) {
    // Create the socket and bind it to the multicast address and port
    boost::asio::ip::udp::endpoint listen_endpoint(
        listen_interface_by_address, multicast_port);
    multicast_socket_.open(listen_endpoint.protocol());
    multicast_socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    multicast_socket_.bind(listen_endpoint);

    // Join the multicast group.
    multicast_socket_.set_option(
        boost::asio::ip::multicast::join_group(multicast_address));
    receive();
}

multicast_channel::~multicast_channel() {
}

void multicast_channel::send(std::string message) {
  std::ostringstream os;
  os << message;
  message_ = os.str();

  send_socket_.async_send_to(
      boost::asio::buffer(message_), multicast_endpoint_,
      boost::bind(&multicast_channel::handle_send_to, this,
        boost::asio::placeholders::error));
}

void multicast_channel::handle_send_to(const boost::system::error_code& error) {
  if (!error) {
    receive();
  }
}

void multicast_channel::receive() {
  multicast_socket_.async_receive_from(
  boost::asio::buffer(data_, max_length), remote_endpoint_,
  boost::bind(&multicast_channel::handle_receive_from, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void multicast_channel::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd) {
  if (!error)
  {
    boost::asio::ip::udp::endpoint local = send_socket_.local_endpoint();
    boost::asio::ip::udp::endpoint sender = remote_endpoint_;
    if (local.port() != sender.port()) {
      mc_app_.received_data(data_, bytes_recvd);
      receive();
    }
  }
}