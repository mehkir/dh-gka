#include "multicast_application_impl.hpp"

multicast_application_impl::multicast_application_impl() {
    multicast_channel_.reset(new multicast_channel(io_service_,
                                            boost::asio::ip::address::from_string("0.0.0.0"),
                                            boost::asio::ip::address::from_string("239.255.0.1"),
                                            65000,
                                            *this));

}

multicast_application_impl::~multicast_application_impl() {

}

void multicast_application_impl::send(std::string message) {
    multicast_channel_->send(message);
}

void multicast_application_impl::start() {
  try {
    io_service_.run();
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}