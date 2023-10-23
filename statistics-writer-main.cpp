#include "statistics_writer.hpp"
#include "logger.hpp"
#include <memory>

int main (int argc, char* argv[]) {
    if(argc != 3) {
      std::cerr << "Usage: statistics-writer <member_count> <filename_description>\n";
      std::cerr << "  Example: statistics-writer 20 ECC_DH-PROTO_STR_DH-20\n";
      return 1;
    }
    int member_count = std::stoi(argv[1]);
    std::string filename_description(argv[2]);
    if (member_count <= 0) {
      std::cerr << "member_count must be greater than 0\n";
      return 1;
    }
    std::unique_ptr<statistics_writer> sw(statistics_writer::get_instance(member_count, filename_description));
    sw->write_statistics();
    return 0;
}