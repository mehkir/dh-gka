#ifndef STR_DH_MESSAGE
#define STR_DH_MESSAGE

#include "primitives.hpp"
#include <cryptopp/integer.h>
#include <tuple>
#include <boost/asio.hpp>

enum message_type {
    FIND,
    OFFER,
    REQUEST,
    RESPONSE
};

struct message {
    public:
        message_type message_type_;
};

struct find_message : message {
    public:
        find_message() {
            message_type_ = message_type::FIND;
        }
        service_id_t required_service_;
};

struct offer_message : message {
    public:
        offer_message() {
            message_type_ = message_type::OFFER;
        }
        service_id_t offered_service_;
};

struct request_message : message {
    public:
        request_message() {
            message_type_ = message_type::REQUEST;
        }
        service_id_t required_service_;
        CryptoPP::Integer blinded_secret_int_;
};

struct response_message : message {
    public:
        response_message() {
            message_type_ = message_type::RESPONSE;
        }
        service_id_t offered_service_;
        CryptoPP::Integer blinded_member_secret_int_;
        CryptoPP::Integer blinded_group_secret_int_;
        member_count_t member_count_;
        std::tuple<boost::asio::ip::address, unsigned short, int> new_sponsor_;
};

#endif