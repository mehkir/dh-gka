#ifndef MESSAGE
#define MESSAGE

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
#include "primitives.hpp"
#include "logger.hpp"

#define LAST_IP_FIELD_IDX 3
#define MESSAGE_ID_SIZE 1 

enum message_type {
    NONE,
    FIND,
    OFFER,
    REQUEST,
    RESPONSE,
    MEMBER_INFO_REQUEST,
    MEMBER_INFO_RESPONSE,
    SYNCH_TOKEN,
    MEMBER_INFO_SYNCH_REQUEST,
    MEMBER_INFO_SYNCH_RESPONSE,
    DISTRIBUTED_RESPONSE
};

static std::vector<unsigned char> get_secbyteblock_as_byte_vector(CryptoPP::SecByteBlock _secbyteblock) {
    std::vector<unsigned char> byte_vector(_secbyteblock.BytePtr(), _secbyteblock.BytePtr()+_secbyteblock.SizeInBytes());
    return byte_vector;
}

static CryptoPP::SecByteBlock get_byte_vector_as_secbyteblock(std::vector<unsigned char> _byte_vector) {
    CryptoPP::SecByteBlock secbyteblock;
    secbyteblock.Assign(_byte_vector.data(), _byte_vector.size());
    return secbyteblock;
}

static std::vector<unsigned char> get_cryptopp_integer_as_byte_vector(CryptoPP::Integer _integer) {
    std::vector<unsigned char> byte_vector;
    for (size_t i = 0; i < _integer.ByteCount(); i++) {
        byte_vector.push_back(_integer.GetByte(i));
    }
    return byte_vector;
}

static CryptoPP::Integer get_byte_vector_as_cryptopp_integer(std::vector<unsigned char> _byte_vector) {
    CryptoPP::Integer integer;
    for (size_t i = 0; i < _byte_vector.size(); i++) {
        integer.SetByte(i, _byte_vector[i]);
    }
    return integer;
}

static std::vector<unsigned char> get_ipv4_address_as_byte_vector(boost::asio::ip::address_v4 _ip_address) {
    std::vector<std::string> ip_fields;
    std::vector<unsigned char> byte_vector;
    boost::split(ip_fields, _ip_address.to_string(), boost::is_any_of("."));
    for (auto byte : ip_fields) {
        byte_vector.push_back(stoi(byte));
    }
    return byte_vector;
}

static void read_from_streambuf(boost::asio::streambuf& _buffer, char* _data, std::streamsize _byte_count){
    std::istream iss(&_buffer);
    iss.read(_data, _byte_count);
}

static void write_to_streambuf(boost::asio::streambuf& _buffer, const char* _data, std::streamsize _byte_count) {
    std::ostream oss(&_buffer);
    oss.write(_data, _byte_count);
}

static boost::asio::ip::address_v4 get_byte_vector_as_ipv4_address(std::vector<unsigned char> _byte_vector) {
    std::stringstream ss;
    for (size_t i = 0; i < _byte_vector.size(); i++) {
        ss << std::to_string(_byte_vector[i]);
        ss << (i < LAST_IP_FIELD_IDX ? "." : ""); 
    }
    return boost::asio::ip::address_v4::from_string(ss.str());
}

struct message {
    public:
        message_id_t message_type_;
        message() {
            message_type_ = message_type::NONE;
        }

        virtual void serialize_(boost::asio::streambuf& _buffer) {
            make_members_serializable();
            std::ostream oss(&_buffer);
            boost::archive::binary_oarchive oarchive(oss);
            write_to_archive(oarchive);
        }

        virtual void deserialize_(boost::asio::streambuf& _buffer) {
            std::istream iss(&_buffer);
            boost::archive::binary_iarchive iarchive(iss);
            read_from_archive(iarchive);
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

        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) {
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) {
            _iarchive >> *this;
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
    protected:
        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            _iarchive >> *this;
        }
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
    protected:
        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            _iarchive >> *this;
        }
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
        blinded_secret_t blinded_secret_;
    protected:
        virtual void make_members_serializable() override {
            blinded_secret_bytes_ = get_secbyteblock_as_byte_vector(blinded_secret_);
        }
        
        virtual void deserialize_members() override {
            blinded_secret_ = get_byte_vector_as_secbyteblock(blinded_secret_bytes_);
        }

        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            _iarchive >> *this;
        }
    private:
        // Serializable members
        std::vector<unsigned char> blinded_secret_bytes_;
        // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<find_message>(*this);
            ar & blinded_secret_bytes_;
        }
};

struct response_message : offer_message {
    public:
        response_message() {
            message_type_ = message_type::RESPONSE;
        }
        blinded_secret_t blinded_sponsor_secret_;
        blinded_secret_t blinded_group_secret_;
        struct new_sponsor {
            public:
                boost::asio::ip::address ip_address_;
                unsigned short port_;
                member_id_t assigned_id_;
                blinded_secret_t blinded_secret_;
            private:
                // Serializable members
                friend class response_message;
                std::vector<unsigned char> ip_address_bytes_;
                std::vector<unsigned char> blinded_secret_bytes_;
        } new_sponsor;
    protected:
        virtual void make_members_serializable() override {
            blinded_sponsor_secret_bytes_ = get_secbyteblock_as_byte_vector(blinded_sponsor_secret_);
            blinded_group_secret_bytes_ = get_secbyteblock_as_byte_vector(blinded_group_secret_);
            new_sponsor.ip_address_bytes_ = get_ipv4_address_as_byte_vector(new_sponsor.ip_address_.to_v4());
            new_sponsor.blinded_secret_bytes_ = get_secbyteblock_as_byte_vector(new_sponsor.blinded_secret_);
        }

        virtual void deserialize_members() override {
            blinded_sponsor_secret_ = get_byte_vector_as_secbyteblock(blinded_sponsor_secret_bytes_);
            blinded_group_secret_ = get_byte_vector_as_secbyteblock(blinded_group_secret_bytes_);
            new_sponsor.ip_address_ = get_byte_vector_as_ipv4_address(new_sponsor.ip_address_bytes_);
            new_sponsor.blinded_secret_ = get_byte_vector_as_secbyteblock(new_sponsor.blinded_secret_bytes_);
        }
        
        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            _iarchive >> *this;
        }
    private:
        // Serializable members
        std::vector<unsigned char> blinded_sponsor_secret_bytes_;
        std::vector<unsigned char> blinded_group_secret_bytes_;
        // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<offer_message>(*this);
            ar & blinded_sponsor_secret_bytes_;
            ar & blinded_group_secret_bytes_;
            ar & new_sponsor.ip_address_bytes_;
            ar & new_sponsor.port_;
            ar & new_sponsor.assigned_id_;
            ar & new_sponsor.blinded_secret_bytes_;
        }
};

struct member_info_request_message : find_message {
    public:
        member_info_request_message() {
            message_type_ = message_type::MEMBER_INFO_REQUEST;
        }
        std::vector<member_id_t> requested_members_;
    protected:
        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            _iarchive >> *this;
        }
    private:
        // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<find_message>(*this);
            ar & requested_members_;
        }
};

struct member_info_response_message : offer_message {
    public:
        member_info_response_message() {
            message_type_ = message_type::MEMBER_INFO_RESPONSE;
        }
        member_id_t member_id_;
        blinded_secret_t blinded_secret_;
    protected:
        virtual void make_members_serializable() override {
            blinded_secret_bytes_ = get_secbyteblock_as_byte_vector(blinded_secret_);
        }

        virtual void deserialize_members() override {
            blinded_secret_ = get_byte_vector_as_secbyteblock(blinded_secret_bytes_);
        }

        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            _iarchive >> *this;
        }
    private:
        // Serializable members
        std::vector<unsigned char> blinded_secret_bytes_;
        // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<offer_message>(*this);
            ar & member_id_;
            ar & blinded_secret_bytes_;
        }
};

struct synch_token_message : message {
    public:
        synch_token_message() {
            message_type_ = message_type::SYNCH_TOKEN;
        }
        member_id_t member_id_;
    protected:
        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            _iarchive >> *this;
        }
    private:
        // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<message>(*this);
            ar & member_id_;
        }
};

struct member_info_synch_request_message : member_info_request_message {
    public:
        member_info_synch_request_message() {
            message_type_ = message_type::MEMBER_INFO_SYNCH_REQUEST;
        }
};

struct member_info_synch_response_message : member_info_response_message {
    public:
        member_info_synch_response_message() {
            message_type_ = message_type::MEMBER_INFO_SYNCH_RESPONSE;
        }
};

struct distributed_response_message : offer_message {
    public:
        distributed_response_message() {
            message_type_ = message_type::DISTRIBUTED_RESPONSE;
        }
        blinded_secret_t blinded_sponsor_secret_;
        secret_t encrypted_group_secret_;
        std::vector<unsigned char> initialization_vector_;
    protected:
        virtual void make_members_serializable() override {
            blinded_sponsor_secret_bytes_ = get_secbyteblock_as_byte_vector(blinded_sponsor_secret_);
            encrypted_group_secret_bytes_ = get_secbyteblock_as_byte_vector(encrypted_group_secret_);
        }

        virtual void deserialize_members() override {
            blinded_sponsor_secret_ = get_byte_vector_as_secbyteblock(blinded_sponsor_secret_bytes_);
            encrypted_group_secret_ = get_byte_vector_as_secbyteblock(encrypted_group_secret_bytes_);
        }
        
        virtual void write_to_archive(boost::archive::binary_oarchive& _oarchive) override {
            _oarchive << *this;
        }

        virtual void read_from_archive(boost::archive::binary_iarchive& _iarchive) override {
            _iarchive >> *this;
        }
    private:
        // Serializable members
        std::vector<unsigned char> blinded_sponsor_secret_bytes_;
        std::vector<unsigned char> encrypted_group_secret_bytes_;
        // Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version) {
            ar & boost::serialization::base_object<offer_message>(*this);
            ar & blinded_sponsor_secret_bytes_;
            ar & encrypted_group_secret_bytes_;
            ar & initialization_vector_;
        }
};

#endif