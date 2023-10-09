#ifndef STATISTICS_RECORDER
#define STATISTICS_RECORDER

#include "shared_memory_parameters.hpp"

class statistics_recorder
{
public:
    static statistics_recorder* get_instance();
    void record_timestamp(time_metric _time_metric);
    void record_count(count_metric _count_metric);
    void compose_statistics();
private:
    static std::mutex mutex_;
    static statistics_recorder* instance_;
    std::unordered_map<metric_id, metric_value> count_statistics_;
    std::unordered_map<metric_id, metric_value> time_statistics_;
    shared_statistics_map* composite_count_statistics_;
    shared_statistics_map* composite_time_statistics_;
    statistics_recorder();
    ~statistics_recorder();
};

#endif