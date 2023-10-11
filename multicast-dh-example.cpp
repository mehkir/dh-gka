#include <boost/algorithm/string.hpp>
#include "logger.hpp"
#include "str_dh.hpp"
#include "distributed_dh.hpp"

int main(int argc, char* argv[]) {
  try
  {
    if (argc != 4)
    {
      std::cerr << "Usage: multicast-dh-example <is_sponsor> <service_id> <member_count>\n";
      std::cerr << "  Example: multicast-dh-example true 42 20\n";
      return 1;
    }

    std::string is_sponsor(argv[1]);
    if (!boost::iequals(is_sponsor, "true") && !boost::iequals(is_sponsor, "false")) {
      std::cerr << "is_sponsor must be \"true\" or \"false\" (case-insensitive)\n";
      return 1;
    }

    int service_id = std::stoi(argv[2]);
    int member_count = std::stoi(argv[3]);
    if (service_id <= 0) {
      std::cerr << "service_id must be greater than 0\n";
      return 1;
    }
    if (member_count <= 0) {
      std::cerr << "member_count must be greater than 0\n";
      return 1;
    }

#ifdef PROTO_STR_DH
    str_dh _member(boost::iequals(is_sponsor, "true"), service_id, member_count);
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