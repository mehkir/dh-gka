#include <boost/algorithm/string.hpp>
#include "logger.hpp"
#include "str_dh.hpp"
#include "distributed_dh.hpp"

int main(int argc, char* argv[]) {
  try
  {
    if (argc != 9)
    {
      std::cerr << "Usage: multicast-dh-example <is_sponsor> <service_id> <member_count> <scatter_delay_min(ms)> <scatter_delay_max(ms)> <listening_interface_by_ip> <multicast_ip> <multicast_port>\n";
      std::cerr << "  Example: multicast-dh-example true 42 20 10 100 \n";
      return 1;
    }

    std::string is_sponsor(argv[1]);
    std::uint32_t service_id = std::stoi(argv[2]);
    std::uint32_t member_count = std::stoi(argv[3]);
    std::uint32_t scatter_delay_min = std::stoi(argv[4]);
    std::uint32_t scatter_delay_max = std::stoi(argv[5]);

    boost::system::error_code ec;
    boost::asio::ip::address listening_interface_by_ip = boost::asio::ip::address::from_string(argv[6], ec);
    if (ec) {
        std::cerr << ec.what() << std::endl;
        return 1;
    }

    boost::asio::ip::address multicast_ip = boost::asio::ip::address::from_string(argv[7], ec);
    if (ec) {
        std::cerr << ec.what() << std::endl;
        return 1;
    }

    if (!boost::iequals(is_sponsor, "true") && !boost::iequals(is_sponsor, "false")) {
      std::cerr << "is_sponsor must be \"true\" or \"false\" (case-insensitive)\n";
      return 1;
    }

    if (service_id < 1) {
      std::cerr << "service id must be greater than 0\n";
      return 1;      
    }

    if (member_count < 2) {
      std::cerr << "service id must be greater than 1\n";
      return 1;
    }

#ifdef PROTO_STR_DH
    str_dh _member(boost::iequals(is_sponsor, "true"), service_id, member_count, scatter_delay_min, scatter_delay_max, listening_interface_by_ip, multicast_ip, std::stoi(argv[8]));
#elif defined(PROTO_DST_DH)
    distributed_dh _member(boost::iequals(is_sponsor, "true"), service_id, member_count, scatter_delay_min, scatter_delay_max, listening_interface_by_ip, multicast_ip, std::stoi(argv[8]));
#endif
    _member.start();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}