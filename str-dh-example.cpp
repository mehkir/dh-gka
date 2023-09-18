#include <boost/algorithm/string.hpp>
#include "logger.hpp"

int main(int argc, char* argv[]) {
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: member <is_sponsor> <service_id>\n";
      std::cerr << "  Example: member true 42\n";
      return 1;
    }

    std::string is_sponsor(argv[1]);
    if (!boost::iequals(is_sponsor, "true") && !boost::iequals(is_sponsor, "false")) {
      std::cerr << "is_sponsor must be \"true\" or \"false\" (case-insensitive)\n";
      return 1;
    }

    int service_id = std::stoi(argv[2]);
    if (service_id <= 0) {
      std::cerr << "service_id must be greater than 0\n";
      return 1;
    }

    //member _member(boost::iequals(is_sponsor, "true"), service_id);
    //_member.start();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}