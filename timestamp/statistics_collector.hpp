#ifndef STATISTICS
#define STATISTICS

#define GROUP_SIZE                         "GROUP_SIZE"
#define FIND_MESSAGE_COUNT                 "FIND_MESSAGE_COUNT"
#define OFFER_MESSAGE_COUNT                "OFFER_MESSAGE_COUNT"
#define REQUEST_MESSAGE_COUNT              "REQUEST_MESSAGE_COUNT"
#define RESPONSE_MESSAGE_COUNT             "RESPONSE_MESSAGE_COUNT"
#define CRYPTO_OPERATIONS_COUNT            "CRYPTO_OPERATIONS_COUNT"
#define DURATION_START                     "DURATION_START"
#define DURATION_END                       "DURATION_END"
#define KEY_AGREEMENT_START                "KEY_AGREEMENT_START"

#include <mutex>
#include <chrono>
#include <unordered_map>

typedef uint64_t metric_value;

class statistics_collector
{

public:
    enum metric {
        GROUP_SIZE_,
        FIND_MESSAGE_COUNT_,
        OFFER_MESSAGE_COUNT_,
        REQUEST_MESSAGE_COUNT_,
        RESPONSE_MESSAGE_COUNT_,
        CRYPTO_OPERATIONS_COUNT_,
        DURATION_START_,
        DURATION_END_,
        KEY_AGREEMENT_START_
    };
    static statistics_collector* get_instance();
    void record_timestamp(metric _metric);
    void record_count(metric _metric);
    void write_statistics();
private:
    static std::mutex mutex_;
    static statistics_collector* instance_;
    std::unordered_map<metric, std::string> metric_names_;
    std::unordered_map<metric, metric_value> statistics_;
    statistics_collector();
    ~statistics_collector();
};

#endif