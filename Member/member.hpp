#include <iostream>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

class member
{
private:
/* Member variables*/
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint sender_endpoint_;
  boost::asio::ip::udp::endpoint multicast_endpoint_;
  bool ping_;
  std::string message_;
  enum { max_length = 1024 };
  char data_[max_length];
private:
/* Methods */
  void send(std::string message);
  void handle_send_to(const boost::system::error_code& error);
  void receive();
  void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd);

public:
    member(boost::asio::io_service& io_service,
      const boost::asio::ip::address& listen_interface_by_address,
      const boost::asio::ip::address& multicast_address,
      int multicast_port, bool ping);
    ~member();
};