#include "multicast_application.hpp"

class MyApp : public multicast_application {
public:
    MyApp() : multicast_application() {
    }

    ~MyApp() {
    }

    void received_data(void* data, std::size_t bytes_recvd) override {
        std::cout.write(reinterpret_cast<const char*>(data), bytes_recvd);
        std::cout << std::endl;
    }

    void send(std::string message) {
      multicast_application::send(message);
    }

    void start() {
      multicast_application::start();
    }
};

int main(int argc, char* argv[])
{
  MyApp myapp;
  myapp.start();
  return 0;
}