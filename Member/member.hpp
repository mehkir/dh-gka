#include <iostream>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include "boost/date_time/posix_time/posix_time_types.hpp"

class member
{
private:
/* Member variables*/
  boost::asio::ip::udp::socket send_socket_;
  boost::asio::ip::udp::socket receive_socket_;
  boost::asio::ip::udp::endpoint sender_endpoint_;
  boost::asio::ip::udp::endpoint multicast_endpoint_;
  short multicast_port_;
  bool ping_;
  std::string message_;
  enum { max_length = 1024 };
  char data_[max_length];
private:
/* Methods */
  void send(const boost::system::error_code& error);
    void handle_send_to(const boost::system::error_code& error);
public:
    member(boost::asio::io_service& io_service,
      const boost::asio::ip::address& multicast_address,
      short multicast_port, bool ping);
    ~member();
};