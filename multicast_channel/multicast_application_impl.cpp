#include "multicast_application_impl.hpp"
#include "logger.hpp"

multicast_application_impl::multicast_application_impl(std::string _listening_interface_by_ip, std::string _multicast_ip, std::uint16_t _port) : multicast_channel_(std::make_unique<multicast_channel>(io_service_,
                                            boost::asio::ip::address::from_string(_listening_interface_by_ip),
                                            boost::asio::ip::address::from_string(_multicast_ip),
                                            _port,
                                            *this)){
}

multicast_application_impl::~multicast_application_impl() {

}

void multicast_application_impl::send_multicast(boost::asio::streambuf& _buffer) {
  multicast_channel_->send_multicast(_buffer);
}

void multicast_application_impl::send_to(boost::asio::streambuf& _buffer, boost::asio::ip::udp::endpoint _endpoint) {
  multicast_channel_->send_to(_buffer, _endpoint);
}

void multicast_application_impl::start() {
  try {
    io_service_.run();
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}

void multicast_application_impl::stop() {
  io_service_.stop();
}

boost::asio::io_service& multicast_application_impl::get_io_service() {
  return io_service_;
}

boost::asio::ip::udp::endpoint multicast_application_impl::get_local_endpoint() const {
  return multicast_channel_->get_local_endpoint();
}