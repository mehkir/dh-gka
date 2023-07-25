#include "multicast_application_impl.hpp"
#include "ip_addresses.hpp"
#include "logger.hpp"

multicast_application_impl::multicast_application_impl() : multicast_channel_(std::make_unique<multicast_channel>(io_service_,
                                            boost::asio::ip::address::from_string(INTERFACE_ADDRESS),
                                            boost::asio::ip::address::from_string(MULTICAST_ADDRESS),
                                            65000,
                                            *this)){
  LOG_DEBUG("[<multicast_application_impl>]: initialization complete")
}

multicast_application_impl::~multicast_application_impl() {
  LOG_DEBUG("[<multicast_application_impl>]: destruction complete")
}

void multicast_application_impl::send(boost::asio::streambuf& buffer) {
  LOG_DEBUG("[<multicast_application_impl>]: send")
  multicast_channel_->send(buffer);
}

void multicast_application_impl::start() {
  try {
    io_service_.run();
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  LOG_DEBUG("[<multicast_application_impl>]: io service started")
}

boost::asio::ip::udp::endpoint multicast_application_impl::get_local_endpoint() const {
  LOG_DEBUG("[<multicast_application_impl>]: get_local_endpoint")
  return multicast_channel_->get_local_endpoint();
}