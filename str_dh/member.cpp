#include "member.hpp"

member::member() {
}

member::~member() {
}

void member::received_data(void* data, std::size_t bytes_recvd) {
    std::cout.write(reinterpret_cast<const char*>(data), bytes_recvd);
    std::cout << std::endl;
}

void member::send(std::string message) {
    multicast_application_impl::send(message);
}

void member::start() {
    multicast_application_impl::start();
}