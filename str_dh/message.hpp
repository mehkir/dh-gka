#ifndef STR_DH_MESSAGE
#define STR_DH_MESSAGE

#include "primitives.hpp"
#include <vector>
#include <sstream>
#include <boost/asio.hpp>
#include <cryptopp/integer.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/algorithm/string.hpp>
#include "logger.hpp"

#define LAST_IP_FIELD_IDX 3
#define MESSAGE_ID_SIZE 1 

enum message_type {
    NONE,
    FIND,
    OFFER,
    REQUEST,
    RESPONSE
};

static std::vector<unsigned char> get_cryptopp_integer_as_byte_vector(CryptoPP::Integer _integer) {
    LOG_DEBUG("[<message.hpp>]: get_cryptopp_integer_as_byte_vector")
    std::vector<unsigned char> byte_vector;
    for (size_t i = 0; i < _integer.ByteCount(); i++) {
        byte_vector.push_back(_integer.GetByte(i));
    }
    return byte_vector;
}

static CryptoPP::Integer get_byte_vector_as_cryptopp_integer(std::vector<unsigned char> _byte_vector) {
    LOG_DEBUG("[<message.hpp>]: get_byte_vector_as_cryptopp_integer")
    CryptoPP::Integer integer;
    for (size_t i = 0; i < _byte_vector.size(); i++) {
        integer.SetByte(i, _byte_vector[i]);
    }
    return integer;
}

static std::vector<unsigned char> get_ipv4_address_as_byte_vector(boost::asio::ip::address_v4 _ip_address) {
    LOG_DEBUG("[<message.hpp>]: get_ipv4_address_as_byte_vector")
    std::vector<std::string> ip_fields;
    std::vector<unsigned char> byte_vector;
    boost::split(ip_fields, _ip_address.to_string(), boost::is_any_of("."));
    for (auto byte : ip_fields) {
        byte_vector.push_back(stoi(byte));
    }
    return byte_vector;
}

static void read_from_streambuf(boost::asio::streambuf& _buffer, char* _data, std::streamsize _byte_count){
    LOG_DEBUG("[<message.hpp>]: read_from_streambuf")
    std::istream iss(&_buffer);
    iss.read(_data, _byte_count);
}

static void write_to_streambuf(boost::asio::streambuf& _buffer, const char* _data, std::streamsize _byte_count) {
    LOG_DEBUG("[<message.hpp>]: write_to_streambuf")
    std::ostream oss(&_buffer);
    oss.write(_data, _byte_count);
}

static boost::asio::ip::address_v4 get_byte_vector_as_ipv4_address(std::vector<unsigned char> _byte_vector) {
    LOG_DEBUG("[<message.hpp>]: get_byte_vector_as_ipv4_address")
    std::stringstream ss;
    for (size_t i = 0; i < _byte_vector.size(); i++) {
        ss << std::to_string(_byte_vector[i]);
        ss << (i < LAST_IP_FIELD_IDX ? "." : ""); 
    }
    return boost::asio::ip::address_v4::from_string(ss.str());
}

struct message {
    public:
        message() {
            message_type_ = message_type::NONE;
            LOG_DEBUG("[<message>]: initialization complete")
        }
        message_id_t message_type_;
        virtual void serialize_(boost::asio::streambuf& _buffer) {
            LOG_DEBUG("[<message>]: serialize_")
            make_members_serializable();
            std::ostream oss(&_buffer);
            boost::archive::binary_oarchive oarchive(oss);
            write_to_archive(oarchive);
        }

        virtual void deserialize_(boost::asio::streambuf& _buffer) {
            LOG_DEBUG("[<message>]: deserialize_")
            std::istream iss(&_buffer);
            boost::archive::binary_iarchive iarchive(iss);
            read_from_archive(iarchive);
            deserialize_members();
        }
    protected:
        virtual void make_members_serializable() {
            LOG_DEBUG("[<message>]: make_members_serializable")
            // Override in messages who need to make
            // non-serializable members serializable   
        }

        virtual void deserialize_members() {
            LOG_DEBUG("[<message>]: deserialize_members")
            // Override in messages who needed to make
            // non-serializable members serializable   
        }

        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) {
            LOG_DEBUG("[<message>]: write_to_archive")
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) {
            LOG_DEBUG("[<message>]: read_from_archive")
            _iarchive >> *this;
        }
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            LOG_DEBUG("[<message>]: serialize")
            ar & message_type_;
        }
};

struct find_message : message {
    public:
        find_message() {
            message_type_ = message_type::FIND;
            LOG_DEBUG("[<find_message>]: initialization complete")
        }
        service_id_t required_service_;
    protected:
        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            LOG_DEBUG("[<find_message>]: write_to_archive")
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            LOG_DEBUG("[<find_message>]: read_from_archive")
            _iarchive >> *this;
        }
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            LOG_DEBUG("[<find_message>]: serialize")
            ar & boost::serialization::base_object<message>(*this);
            ar & required_service_;
        }
};

struct offer_message : message {
    public:
        offer_message() {
            message_type_ = message_type::OFFER;
            LOG_DEBUG("[<offer_message>]: initialization complete")
        }
        service_id_t offered_service_;
    protected:
        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            LOG_DEBUG("[<offer_message>]: write_to_archive")
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            LOG_DEBUG("[<offer_message>]: read_from_archive")
            _iarchive >> *this;
        }
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            LOG_DEBUG("[<offer_message>]: serialize")
            ar & boost::serialization::base_object<message>(*this);
            ar & offered_service_;
        }
};

struct request_message : find_message {
    public:
        request_message() {
            message_type_ = message_type::REQUEST;
            LOG_DEBUG("[<request_message>]: initialization complete")
        }
        CryptoPP::Integer blinded_secret_int_;
    protected:
        virtual void make_members_serializable() override {
            LOG_DEBUG("[<request_message>]: make_members_serializable")
            blinded_secret_int_bytes_ = get_cryptopp_integer_as_byte_vector(blinded_secret_int_);
        }
        
        virtual void deserialize_members() override {
            LOG_DEBUG("[<request_message>]: deserialize_members")
            blinded_secret_int_ = get_byte_vector_as_cryptopp_integer(blinded_secret_int_bytes_);
        }

        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            LOG_DEBUG("[<request_message>]: write_to_archive")
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            LOG_DEBUG("[<request_message>]: read_from_archive")
            _iarchive >> *this;
        }
    private:
        // Serializable members
        std::vector<unsigned char> blinded_secret_int_bytes_;
        // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            LOG_DEBUG("[<request_message>]: serialize")
            ar & boost::serialization::base_object<find_message>(*this);
            ar & blinded_secret_int_bytes_;
        }
};

struct response_message : offer_message {
    public:
        response_message() {
            message_type_ = message_type::RESPONSE;
            LOG_DEBUG("[<response_message>]: initialization complete")
        }
        CryptoPP::Integer blinded_sponsor_secret_int_;
        CryptoPP::Integer blinded_group_secret_int_;
        member_count_t member_count_;
        struct new_sponsor {
            public:
                boost::asio::ip::address ip_address_;
                unsigned short port_;
                member_id_t assigned_id_;
            private:
                friend class response_message;
                std::vector<unsigned char> ip_address_bytes_;
        } new_sponsor;
    protected:
        virtual void make_members_serializable() override {
            LOG_DEBUG("[<response_message>]: make_members_serializable")
            blinded_sponsor_secret_int_bytes_ = get_cryptopp_integer_as_byte_vector(blinded_sponsor_secret_int_);
            blinded_group_secret_int_bytes_ = get_cryptopp_integer_as_byte_vector(blinded_group_secret_int_);
            new_sponsor.ip_address_bytes_ = get_ipv4_address_as_byte_vector(new_sponsor.ip_address_.to_v4());
        }

        virtual void deserialize_members() override {
            LOG_DEBUG("[<response_message>]: deserialize_members")
            blinded_sponsor_secret_int_ = get_byte_vector_as_cryptopp_integer(blinded_sponsor_secret_int_bytes_);
            blinded_group_secret_int_ = get_byte_vector_as_cryptopp_integer(blinded_group_secret_int_bytes_);
            new_sponsor.ip_address_ = get_byte_vector_as_ipv4_address(new_sponsor.ip_address_bytes_);
        }
        
        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            LOG_DEBUG("[<response_message>]: write_to_archive")
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            LOG_DEBUG("[<response_message>]: read_from_archive")
            _iarchive >> *this;
        }
    private:
        // Serializable members
        std::vector<unsigned char> blinded_sponsor_secret_int_bytes_;
        std::vector<unsigned char> blinded_group_secret_int_bytes_;
        // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            LOG_DEBUG("[<response_message>]: serialize")
            ar & boost::serialization::base_object<offer_message>(*this);
            ar & blinded_sponsor_secret_int_bytes_;
            ar & blinded_group_secret_int_bytes_;
            ar & member_count_;
            ar & new_sponsor.ip_address_bytes_;
            ar & new_sponsor.port_;
            ar & new_sponsor.assigned_id_;
        }
};

#endif