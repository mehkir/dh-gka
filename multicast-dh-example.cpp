#include <boost/algorithm/string.hpp>
#include "logger.hpp"
#include "str_dh.hpp"
#include "distributed_dh.hpp"

int main(int argc, char* argv[]) {
  try
  {
    if (argc != 6)
    {
      std::cerr << "Usage: multicast-dh-example <is_sponsor> <service_id> <member_count> <scatter_delay_min(ms)> <scatter_delay_max(ms)>\n";
      std::cerr << "  Example: multicast-dh-example true 42 20 10 100\n";
      return 1;
    }

    std::string is_sponsor(argv[1]);
    std::uint32_t service_id = std::stoi(argv[2]);
    std::uint32_t member_count = std::stoi(argv[3]);
    std::uint32_t scatter_delay_min = std::stoi(argv[4]);
    std::uint32_t scatter_delay_max = std::stoi(argv[5]);

    if (!boost::iequals(is_sponsor, "true") && !boost::iequals(is_sponsor, "false")) {
      std::cerr << "is_sponsor must be \"true\" or \"false\" (case-insensitive)\n";
      return 1;
    }

    if (member_count < 2) {
      std::cerr << "service id must be greater than 1\n";
      return 1;
    }

#ifdef PROTO_STR_DH
    str_dh _member(boost::iequals(is_sponsor, "true"), service_id, member_count, scatter_delay_min, scatter_delay_max);
#elif defined(PROTO_DST_DH)
    distributed_dh _member(boost::iequals(is_sponsor, "true"), service_id);
#endif
    _member.start();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}