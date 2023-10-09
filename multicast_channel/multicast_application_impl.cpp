#include "multicast_application_impl.hpp"
#include "ip_addresses.hpp"
#include "logger.hpp"

multicast_application_impl::multicast_application_impl() : multicast_channel_(std::make_unique<multicast_channel>(io_service_,
                                            boost::asio::ip::address::from_string(INTERFACE_ADDRESS),
                                            boost::asio::ip::address::from_string(MULTICAST_ADDRESS),
                                            65000,
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

boost::asio::ip::udp::endpoint multicast_application_impl::get_local_endpoint() const {
  return multicast_channel_->get_local_endpoint();
}