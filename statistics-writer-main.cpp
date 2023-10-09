#include "statistics_writer.hpp"
#include <memory>

int main (int argc, char* argv[]) {
    std::unique_ptr<statistics_writer> sw(statistics_writer::get_instance());
    sw->write_statistics();
    return 0;
}