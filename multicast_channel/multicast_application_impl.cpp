#include "multicast_application_impl.hpp"
#include "ip_addresses.hpp"

multicast_application_impl::multicast_application_impl() : multicast_channel_(std::make_unique<multicast_channel>(io_service_,
                                            boost::asio::ip::address::from_string(INTERFACE_ADDRESS),
                                            boost::asio::ip::address::from_string(MULTICAST_ADDRESS),
                                            65000,
                                            *this)){
}

multicast_application_impl::~multicast_application_impl() {

}

void multicast_application_impl::send(std::string _message) {
    multicast_channel_->send(_message);
}

void multicast_application_impl::start() {
  try {
    io_service_.run();
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}