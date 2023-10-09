#include "statistics_writer.hpp"
#include "logger.hpp"
#include <memory>

int main (int argc, char* argv[]) {
    if(argc != 2) {
      std::cerr << "Usage: member <is_sponsor> <service_id>\n";
      std::cerr << "  Example: member true 42\n";
      return 1;
    }
    std::unique_ptr<statistics_writer> sw(statistics_writer::get_instance(std::stoi(argv[1])));
    sw->write_statistics();
    return 0;
}