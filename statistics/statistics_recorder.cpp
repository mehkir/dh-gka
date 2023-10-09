#include "statistics_recorder.hpp"
#include "logger.hpp"

#include <iostream>
#include <stdexcept>

std::mutex statistics_recorder::mutex_;
statistics_recorder* statistics_recorder::instance_;

statistics_recorder* statistics_recorder::get_instance() {
    std::lock_guard<std::mutex> lock_guard(mutex_);
    if(instance_ == nullptr) {
        instance_ = new statistics_recorder();
    }
    return instance_;
}

statistics_recorder::statistics_recorder() {
}

statistics_recorder::~statistics_recorder() {
}

void statistics_recorder::record_timestamp(time_metric _time_metric) {
    if(time_statistics_.count(_time_metric)) {
        throw std::runtime_error("There is already a timestamp for the key: " + _time_metric);
    }
    time_statistics_[_time_metric] = std::chrono::system_clock::now().time_since_epoch().count();
}

void statistics_recorder::record_count(count_metric _count_metric) {
    count_statistics_[_count_metric]++;
}

void statistics_recorder::compose_statistics() {
    for (bool shared_objects_initialized = false; !shared_objects_initialized;) {
        try {
            boost::interprocess::named_condition condition(boost::interprocess::open_only, STATISTICS_CONDITION);
            boost::interprocess::named_mutex mutex(boost::interprocess::open_only, STATISTICS_MUTEX);
            boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, SEGMENT_NAME);
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);

            while (!(composite_count_statistics_ = segment.find<shared_statistics_map>(COUNT_STATISTICS_MAP_NAME).first) &&
               !(composite_time_statistics_ = segment.find<shared_statistics_map>(TIME_STATISTICS_MAP_NAME).first)) {
                condition.wait(lock);
                LOG_DEBUG("[<statistics_recorder>] (compose_statistics) shared maps not intialized yet")
            }
            LOG_DEBUG("[<statistics_recorder>] (compose_statistics) resume composing")
            for(std::pair<metric_id, metric_value> pair : count_statistics_) {
                if(!(*composite_count_statistics_).count(pair.first)) {
                    (*composite_count_statistics_)[pair.first] = 0;
                }
                (*composite_count_statistics_)[pair.first] += pair.second;
            }
            for(std::pair<metric_id, metric_value> pair : time_statistics_) {
                (*composite_time_statistics_)[pair.first] = pair.second;
            }
            condition.notify_one();
            shared_objects_initialized = true;
        } catch (boost::interprocess::interprocess_exception interprocess_exception) {
            std::cerr << interprocess_exception.what() << std::endl;
            LOG_DEBUG("[<statistics_recorder>] (compose_statistics) shared objects not created yet")
            sleep(1);
        }
    }
}