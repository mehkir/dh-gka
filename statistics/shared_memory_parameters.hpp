#ifndef SHARED_MEMORY_PARAMETERS
#define SHARED_MEMORY_PARAMETERS

#define MEMBER_COUNT                            "MEMBER_COUNT"
#define FIND_MESSAGE_COUNT                      "FIND_MESSAGE_COUNT"
#define OFFER_MESSAGE_COUNT                     "OFFER_MESSAGE_COUNT"
#define REQUEST_MESSAGE_COUNT                   "REQUEST_MESSAGE_COUNT"
#define RESPONSE_MESSAGE_COUNT                  "RESPONSE_MESSAGE_COUNT"
#define MEMBER_INFO_REQUEST_MESSAGE_COUNT       "MEMBER_INFO_REQUEST_MESSAGE_COUNT"
#define MEMBER_INFO_RESPONSE_MESSAGE_COUNT      "MEMBER_INFO_RESPONSE_MESSAGE_COUNT"
#define CRYPTO_OPERATIONS_COUNT                 "CRYPTO_OPERATIONS_COUNT"
#define DURATION_START                          "DURATION_START"
#define DURATION_END                            "DURATION_END"
#define KEY_AGREEMENT_START                     "KEY_AGREEMENT_START"

#define SEGMENT_NAME                    "statistics_shared_memory"
#define COUNT_STATISTICS_MAP_NAME       "count_statistics_shared_map"
#define TIME_STATISTICS_MAP_NAME        "time_statistics_shared_map"
#define STATISTICS_MUTEX                "statistics_mutex"
#define STATISTICS_CONDITION            "statistics_condition"
#define SEGMENT_SIZE_BYTES              65536

#include <mutex>
#include <chrono>
#include <unordered_map>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <functional>
#include <utility>

typedef int      metric_id;
typedef uint64_t metric_value;

typedef int       key_type;
typedef uint64_t  mapped_type;
typedef std::pair<const int, uint64_t> ValueType;
typedef boost::interprocess::allocator<ValueType, boost::interprocess::managed_shared_memory::segment_manager> shmem_allocator;
typedef boost::interprocess::map<key_type, mapped_type, std::less<key_type>, shmem_allocator> shared_statistics_map;

enum count_metric {
    MEMBER_COUNT_,
    FIND_MESSAGE_COUNT_,
    OFFER_MESSAGE_COUNT_,
    REQUEST_MESSAGE_COUNT_,
    RESPONSE_MESSAGE_COUNT_,
    MEMBER_INFO_REQUEST_MESSAGE_COUNT_,
    MEMBER_INFO_RESPONSE_MESSAGE_COUNT_,
    CRYPTO_OPERATIONS_COUNT_,
    COUNT_SIZE = CRYPTO_OPERATIONS_COUNT_+1
};
enum time_metric {
    DURATION_START_,
    DURATION_END_,
    KEY_AGREEMENT_START_,
    TIME_SIZE = KEY_AGREEMENT_START_+1
};
enum shm_flags {
    MEMBER_WRITES
};

#endif