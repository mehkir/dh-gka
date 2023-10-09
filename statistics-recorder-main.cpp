#include "statistics_recorder.hpp"
#include <memory>

int main (int argc, char* argv[]) {
    std::unique_ptr<statistics_recorder> sr(statistics_recorder::get_instance());
    sr->compose_statistics();
    return 0;
}