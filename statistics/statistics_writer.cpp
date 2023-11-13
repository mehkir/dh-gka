#include "statistics_writer.hpp"
#include "logger.hpp"

#include <fstream>
#include <sstream>
#include <sys/stat.h>

std::mutex statistics_writer::mutex_;
statistics_writer* statistics_writer::instance_;
int statistics_writer::member_count_;
std::string statistics_writer::absolute_results_directory_path_;
std::string statistics_writer::result_filename_;

statistics_writer* statistics_writer::get_instance(int _member_count, std::string _absolute_results_directory_path, std::string _result_filename) {
    std::lock_guard<std::mutex> lock_guard(mutex_);
    if(instance_ == nullptr) {
        instance_ = new statistics_writer();
        member_count_ = _member_count;
        absolute_results_directory_path_ = _absolute_results_directory_path;
        result_filename_ = _result_filename;
    }
    return instance_;
}

statistics_writer::statistics_writer() {
    count_metric_names_[count_metric::MEMBER_COUNT_] = MEMBER_COUNT;
    count_metric_names_[count_metric::FIND_MESSAGE_COUNT_] = FIND_MESSAGE_COUNT;
    count_metric_names_[count_metric::OFFER_MESSAGE_COUNT_] = OFFER_MESSAGE_COUNT;
    count_metric_names_[count_metric::REQUEST_MESSAGE_COUNT_] = REQUEST_MESSAGE_COUNT;
    count_metric_names_[count_metric::RESPONSE_MESSAGE_COUNT_] = RESPONSE_MESSAGE_COUNT;
    count_metric_names_[count_metric::MEMBER_INFO_REQUEST_MESSAGE_COUNT_] = MEMBER_INFO_REQUEST_MESSAGE_COUNT;
    count_metric_names_[count_metric::MEMBER_INFO_RESPONSE_MESSAGE_COUNT_] = MEMBER_INFO_RESPONSE_MESSAGE_COUNT;
    count_metric_names_[count_metric::SYNCH_TOKEN_MESSAGE_COUNT_] = SYNCH_TOKEN_MESSAGE_COUNT;
    count_metric_names_[count_metric::FINISH_MESSAGE_COUNT_] = FINISH_MESSAGE_COUNT;
    count_metric_names_[count_metric::FINISH_ACK_MESSAGE_COUNT_] = FINISH_ACK_MESSAGE_COUNT;
    count_metric_names_[count_metric::DISTRIBUTED_RESPONSE_MESSAGE_COUNT_] = DISTRIBUTED_RESPONSE_MESSAGE_COUNT;
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
}

statistics_writer::~statistics_writer() {
    boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, SEGMENT_NAME);
    segment.destroy<shared_statistics_map>(COUNT_STATISTICS_MAP_NAME);
    segment.destroy<shared_statistics_map>(TIME_STATISTICS_MAP_NAME);
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
        LOG_STD("[<statistics_writer>] " << current_member_count << "/" << member_count_ << " have added statistics")
        condition.notify_one();
        condition.wait(lock);
    }
    LOG_STD("[<statistics_writer>] " << current_member_count << "/" << member_count_ << " have added statistics")
    std::ofstream statistics_file;
    int filecount = 0;
    std::stringstream absolute_result_file_path;
    absolute_result_file_path << absolute_results_directory_path_ << result_filename_ << "-#" << filecount << ".csv";
    struct stat buffer;
    //Choose unused/non-existing absolute_result_file_path
    for(filecount = 1; (stat(absolute_result_file_path.str().c_str(), &buffer) == 0); filecount++) {
        absolute_result_file_path.str("");
        absolute_result_file_path << absolute_results_directory_path_ << result_filename_ << "-#" << filecount << ".csv";
    }
    statistics_file.open(absolute_result_file_path.str());
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
        } else {
            statistics_file << 0;
        }
        statistics_file << ",";
    }
    for(metric_id m_id = 0; m_id < time_metric::TIME_SIZE; m_id++) {
        if((*composite_time_statistics_).count(m_id)) {
            statistics_file << (*composite_time_statistics_)[m_id];
        } else {
            statistics_file << 0;
        }
        if(m_id != time_metric::TIME_SIZE-1) {
            statistics_file << ",";
        } else {
            statistics_file << "\n";
        }
    }
    statistics_file.close();
}