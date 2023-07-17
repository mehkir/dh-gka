#include "multicast_application.hpp"

multicast_application::multicast_application(multicast_application& mc_app) {
    multicast_channel_.reset(new multicast_channel(io_service_,
                                            boost::asio::ip::address::from_string("0.0.0.0"),
                                            boost::asio::ip::address::from_string("239.255.0.1"),
                                            65000,
                                            mc_app));

}

multicast_application::~multicast_application() {

}

void multicast_application::send(std::string message) {
    multicast_channel_->send(message);
}

void multicast_application::start() {
  try {
    io_service_.run();
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}