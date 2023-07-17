#include "member.hpp"

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 5)
    {
      std::cerr << "Usage: member <listen_interface_by_address> <multicast_address> <multicast_port> <ping_bool> \n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    0.0.0.0 239.255.0.1\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    0::0 ff31::8000:1234\n";
      return 1;
    }

    boost::asio::io_service io_service;
    member member_(io_service,
        boost::asio::ip::address::from_string(argv[1]),
        boost::asio::ip::address::from_string(argv[2]),
        std::stoi(argv[3]),
        bool(std::stoi(argv[4])));
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}