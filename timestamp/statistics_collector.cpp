#include "statistics_collector.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <vector>
#include <stdexcept>

std::mutex statistics_collector::mutex_;
statistics_collector* statistics_collector::instance_;

statistics_collector* statistics_collector::get_instance() {
    std::lock_guard<std::mutex> lock_guard(mutex_);
    if(instance_ == nullptr) {
        instance_ = new statistics_collector();
    }
    return instance_;
}

statistics_collector::statistics_collector() {
    metric_names_[metric::GROUP_SIZE_] = GROUP_SIZE;
    metric_names_[metric::FIND_MESSAGE_COUNT_] = FIND_MESSAGE_COUNT;
    metric_names_[metric::OFFER_MESSAGE_COUNT_] = OFFER_MESSAGE_COUNT;
    metric_names_[metric::REQUEST_MESSAGE_COUNT_] = REQUEST_MESSAGE_COUNT;
    metric_names_[metric::RESPONSE_MESSAGE_COUNT_] = RESPONSE_MESSAGE_COUNT;
    metric_names_[metric::CRYPTO_OPERATIONS_COUNT_] = CRYPTO_OPERATIONS_COUNT;
    metric_names_[metric::DURATION_START_] = DURATION_START;
    metric_names_[metric::DURATION_END_] = DURATION_END;
    metric_names_[metric::KEY_AGREEMENT_START_] = KEY_AGREEMENT_START;
}

statistics_collector::~statistics_collector() {
}

void statistics_collector::record_timestamp(metric _metric) {
    if(statistics_.count(_metric)) {
        throw std::runtime_error("There is already a timestamp for the key: " + metric_names_[_metric]);
    }
    statistics_[_metric] = std::chrono::system_clock::now().time_since_epoch().count();
}

void statistics_collector::record_count(metric _metric) {
    statistics_[_metric]++;
}

void statistics_collector::write_statistics() {
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
    for(int metric_idx = 0; metric_idx < metric_names_.size(); metric_idx++) {
        statistics_file << metric_names_[metric(metric_idx)];
        if(metric_idx != metric_names_.size()-1) {
            statistics_file << ",";
        } else {
            statistics_file << "\n";
        }
    }
    //Write values
    for(int metric_idx = 0; metric_idx < metric_names_.size(); metric_idx++) {
        statistics_file << statistics_[metric(metric_idx)];
        if(metric_idx != metric_names_.size()-1) {
            statistics_file << ",";
        } else {
            statistics_file << "\n";
        }
    }
    statistics_file.close();
}