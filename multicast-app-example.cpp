#include "multicast_application_impl.hpp"
#include "logger.hpp"

class multicast_app : public multicast_application_impl {
public:
    multicast_app() {
    }

    ~multicast_app() {
    }

    void received_data(unsigned char* _data, size_t _bytes_recvd) override {
        LOG_DEBUG(_data)
    }

    void send(boost::asio::streambuf& buffer) {
      multicast_application_impl::send(buffer);
    }

    void start() {
      multicast_application_impl::start();
    }
};

int main(int argc, char* argv[]) {
  multicast_app _multicast_app;
  _multicast_app.start();
  return 0;
}