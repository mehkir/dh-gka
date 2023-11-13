#include "multicast_application_impl.hpp"
#include "logger.hpp"

class multicast_app : public multicast_application_impl {
public:
    multicast_app() : multicast_application_impl(boost::asio::ip::address::from_string("127.0.0.1"), boost::asio::ip::address::from_string("239.255.0.1"), 65000) {
      LOG_STD("[<multicast_app>]: endpoint(" << get_local_endpoint().address() << "," << get_local_endpoint().port() << ")")
    }

    ~multicast_app() {
    }

    void received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) override {
      std::lock_guard<std::mutex> receive_guard(mutex_);
      LOG_DEBUG(_data)
    }

    void send(boost::asio::streambuf& buffer) {
      multicast_application_impl::send_multicast(buffer);
    }

    void start() {
      multicast_application_impl::start();
    }
private:
    std::mutex mutex_;
};

int main(int argc, char* argv[]) {
  multicast_app _multicast_app;
  _multicast_app.start();
  return 0;
}