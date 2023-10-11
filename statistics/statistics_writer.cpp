#include "statistics_writer.hpp"
#include "logger.hpp"

#include <fstream>
#include <sstream>
#include <sys/stat.h>

std::mutex statistics_writer::mutex_;
statistics_writer* statistics_writer::instance_;
int statistics_writer::member_count_;

statistics_writer* statistics_writer::get_instance(int _member_count) {
    std::lock_guard<std::mutex> lock_guard(mutex_);
    if(instance_ == nullptr) {
        instance_ = new statistics_writer();
        member_count_ = _member_count;
    }
    return instance_;
}

statistics_writer::statistics_writer() {
    count_metric_names_[count_metric::MEMBER_COUNT_] = MEMBER_COUNT;
    count_metric_names_[count_metric::FIND_MESSAGE_COUNT_] = FIND_MESSAGE_COUNT;
    count_metric_names_[count_metric::OFFER_MESSAGE_COUNT_] = OFFER_MESSAGE_COUNT;
    count_metric_names_[count_metric::REQUEST_MESSAGE_COUNT_] = REQUEST_MESSAGE_COUNT;
    count_metric_names_[count_metric::RESPONSE_MESSAGE_COUNT_] = RESPONSE_MESSAGE_COUNT;
    count_metric_names_[count_metric::CRYPTO_OPERATIONS_COUNT_] = CRYPTO_OPERATIONS_COUNT;
    time_metric_names_[time_metric::DURATION_START_] = DURATION_START;
    time_metric_names_[time_metric::DURATION_END_] = DURATION_END;
    time_metric_names_[time_metric::KEY_AGREEMENT_START_] = KEY_AGREEMENT_START;

    boost::interprocess::managed_shared_memory segment(boost::interprocess::create_only, SEGMENT_NAME, SEGMENT_SIZE_BYTES);
    shmem_allocator allocator(segment.get_segment_manager());
    composite_count_statistics_ = segment.construct<shared_statistics_map>(COUNT_STATISTICS_MAP_NAME)(std::less<int>(), allocator);
    composite_time_statistics_ = segment.construct<shared_statistics_map>(TIME_STATISTICS_MAP_NAME)(std::less<int>(), allocator);
    boost::interprocess::named_condition condition(boost::interprocess::create_only, STATISTICS_CONDITION);
    boost::interprocess::named_mutex mutex(boost::interprocess::create_only, STATISTICS_MUTEX);
    LOG_DEBUG("[<statistics_writer>] Constructor is called")
}

statistics_writer::~statistics_writer() {
    boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, SEGMENT_NAME);
    segment.destroy<shared_statistics_map>(COUNT_STATISTICS_MAP_NAME);
    segment.destroy<shared_statistics_map>(TIME_STATISTICS_MAP_NAME);
    LOG_DEBUG("[<statistics_writer>] Destructor is called")
}

void statistics_writer::write_statistics() {
    boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, SEGMENT_NAME);
    composite_count_statistics_ = segment.find<shared_statistics_map>(COUNT_STATISTICS_MAP_NAME).first;
    composite_time_statistics_ = segment.find<shared_statistics_map>(TIME_STATISTICS_MAP_NAME).first;
    boost::interprocess::named_condition condition(boost::interprocess::open_only, STATISTICS_CONDITION);
    boost::interprocess::named_mutex mutex(boost::interprocess::open_only, STATISTICS_MUTEX);
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
    int current_member_count = 0;
    while(!(*composite_count_statistics_).count(count_metric::MEMBER_COUNT_) || (current_member_count = (*composite_count_statistics_)[count_metric::MEMBER_COUNT_]) != member_count_) {
        std::cout << "[<statistics_writer>] " << current_member_count << "/" << member_count_ << " have added statistics" << std::endl;
        condition.notify_one();
        condition.wait(lock);
    }
    std::cout << "[<statistics_writer>] " << current_member_count << "/" << member_count_ << " have added statistics" << std::endl;
    std::ofstream statistics_file;
    int filecount = 0;
    std::stringstream filename;
    filename << "statistic_results/statistic-#" << filecount << ".csv";
    struct stat buffer;
    //Choose unused/non-existing filename
    for(filecount = 1; (stat(filename.str().c_str(), &buffer) == 0); filecount++) {
        filename.str("");
        filename << "statistic_results/statistic-#" << filecount << ".csv";
    }
    statistics_file.open(filename.str());
    //Write header
    for(metric_id m_id = 0; m_id < count_metric::COUNT_SIZE; m_id++) {
        statistics_file << count_metric_names_[m_id];
        statistics_file << ",";
    }
    for(metric_id m_id = 0; m_id < time_metric::TIME_SIZE; m_id++) {
        statistics_file << time_metric_names_[m_id];
        if(m_id != time_metric::TIME_SIZE-1) {
            statistics_file << ",";
        } else {
            statistics_file << "\n";
        }
    }
    //Write values (keep metric order like above so that header and values comply)
    for(metric_id m_id = 0; m_id < count_metric::COUNT_SIZE; m_id++) {
        if((*composite_count_statistics_).count(m_id)) {
            statistics_file << (*composite_count_statistics_)[m_id];
        }
        statistics_file << ",";
    }
    for(metric_id m_id = 0; m_id < time_metric::TIME_SIZE; m_id++) {
        if((*composite_time_statistics_).count(m_id)) {
            statistics_file << (*composite_time_statistics_)[m_id];
        }
        if(m_id != time_metric::TIME_SIZE-1) {
            statistics_file << ",";
        } else {
            statistics_file << "\n";
        }
    }
    statistics_file.close();
}