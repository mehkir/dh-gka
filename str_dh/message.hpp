#ifndef STR_DH_MESSAGE
#define STR_DH_MESSAGE

#define LAST_IP_FIELD_IDX 3

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

enum message_type {
    FIND,
    OFFER,
    REQUEST,
    RESPONSE
};

struct message {
    public:
        message_type message_type_;
        void serialize_(boost::asio::streambuf& buffer) {
            make_members_serializable();
            std::ostream oss(&buffer);
            boost::archive::binary_oarchive oarchive(oss);
            oarchive << *this;
        }

        void deserialize_(boost::asio::streambuf& buffer) {
            std::istream iss(&buffer);
            boost::archive::binary_iarchive iarchive(iss);
            iarchive >> *this;
            deserialize_members();
        }
    protected:
        virtual void make_members_serializable() {
            // Override in messages who need to make
            // non-serializable members serializable   
        }

        virtual void deserialize_members() {
            // Override in messages who needed to make
            // non-serializable members serializable   
        }
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & message_type_;
        }
};

struct find_message : message {
    public:
        find_message() {
            message_type_ = message_type::FIND;
        }
        service_id_t required_service_;
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<message>(*this);
            ar & required_service_;
        }
};

struct offer_message : message {
    public:
        offer_message() {
            message_type_ = message_type::OFFER;
        }
        service_id_t offered_service_;
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<message>(*this);
            ar & offered_service_;
        }
};

struct request_message : find_message {
    public:
        request_message() {
            message_type_ = message_type::REQUEST;
        }
        CryptoPP::Integer blinded_secret_int_;
        std::vector<unsigned char> blinded_secret_int_bytes_;
    protected:
        virtual void make_members_serializable() override {
            blinded_secret_int_bytes_ = get_cryptopp_integer_as_byte_vector(blinded_secret_int_);
        }
        
        virtual void deserialize_members() override {
            blinded_secret_int_ = get_byte_vector_as_cryptopp_integer(blinded_secret_int_bytes_);
        }
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<find_message>(*this);
            ar & blinded_secret_int_bytes_;
        }
};

struct response_message : offer_message {
    public:
        response_message() {
            message_type_ = message_type::RESPONSE;
        }
        CryptoPP::Integer blinded_sponsor_secret_int_;
        CryptoPP::Integer blinded_group_secret_int_;
        member_count_t member_count_;
        std::vector<unsigned char> blinded_sponsor_secret_int_bytes_;
        std::vector<unsigned char> blinded_group_secret_int_bytes_;
        struct new_sponsor {
            public:
                boost::asio::ip::address ip_address_;
                unsigned short port_;
                member_id_t assigned_id_;
                std::vector<unsigned char> ip_address_bytes_;
        } new_sponsor;
    protected:
        virtual void make_members_serializable() override {
            blinded_sponsor_secret_int_bytes_ = get_cryptopp_integer_as_byte_vector(blinded_sponsor_secret_int_);
            blinded_group_secret_int_bytes_ = get_cryptopp_integer_as_byte_vector(blinded_group_secret_int_);
            new_sponsor.ip_address_bytes_ = get_ipv4_address_as_byte_vector(new_sponsor.ip_address_.to_v4());
        }

        virtual void deserialize_members() override {
            blinded_sponsor_secret_int_ = get_byte_vector_as_cryptopp_integer(blinded_sponsor_secret_int_bytes_);
            blinded_group_secret_int_ = get_byte_vector_as_cryptopp_integer(blinded_group_secret_int_bytes_);
            new_sponsor.ip_address_ = get_byte_vector_as_ipv4_address(new_sponsor.ip_address_bytes_);
        }
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<offer_message>(*this);
            ar & blinded_sponsor_secret_int_bytes_;
            ar & blinded_group_secret_int_bytes_;
            ar & member_count_;
            ar & new_sponsor.ip_address_bytes_;
            ar & new_sponsor.port_;
            ar & new_sponsor.assigned_id_;
        }
};

std::vector<unsigned char> get_cryptopp_integer_as_byte_vector(CryptoPP::Integer _integer) {
    std::vector<unsigned char> byte_vector;
    for (size_t i = 0; i < _integer.ByteCount(); i++) {
        byte_vector.push_back(_integer.GetByte(i));
    }
    return byte_vector;
}

CryptoPP::Integer get_byte_vector_as_cryptopp_integer(std::vector<unsigned char> _byte_vector) {
    CryptoPP::Integer integer;
    for (size_t i = 0; i < _byte_vector.size(); i++) {
        integer.SetByte(i, _byte_vector[i]);
    }
    return integer;
}

std::vector<unsigned char> get_ipv4_address_as_byte_vector(boost::asio::ip::address_v4 _ip_address) {
    std::vector<std::string> ip_fields;
    std::vector<unsigned char> byte_vector;
    boost::split(ip_fields, _ip_address.to_string(), boost::is_any_of("."));
    for (auto byte : ip_fields) {
        byte_vector.push_back(stoi(byte));
    }
    return byte_vector;
}

boost::asio::ip::address_v4 get_byte_vector_as_ipv4_address(std::vector<unsigned char> _byte_vector) {
    std::stringstream ss;
    for (size_t i = 0; i < _byte_vector.size(); i++) {
        ss << std::to_string(_byte_vector[i]);
        ss << (i < LAST_IP_FIELD_IDX ? "." : ""); 
    }
    return boost::asio::ip::address_v4::from_string(ss.str());
}


#endif