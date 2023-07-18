#include "multicast_application_impl.hpp"

class multicast_app : public multicast_application_impl {
public:
    multicast_app() {
    }

    ~multicast_app() {
    }

    void received_data(void* data, std::size_t bytes_recvd) override {
        std::cout.write(reinterpret_cast<const char*>(data), bytes_recvd);
        std::cout << std::endl;
    }

    void send(std::string message) {
      multicast_application_impl::send(message);
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