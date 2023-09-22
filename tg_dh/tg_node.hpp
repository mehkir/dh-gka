#ifndef TG_NODE_DH
#define TG_NODE_DH

#include <cryptopp/integer.h>
#include <memory>

struct node {
    std::unique_ptr<node> left_node_;
    std::unique_ptr<node> right_node_;
    int right_height_;
    int left_height_;
};

struct root_node : node {
    CryptoPP::Integer group_secret_;
    CryptoPP::Integer blinded_group_secret_;
};

struct member_node : node {
    CryptoPP::Integer member_secret_;
    CryptoPP::Integer blinded_member_secret_;
};

struct intermediate_node : node {
    CryptoPP::Integer intermediate_secret_;
    CryptoPP::Integer blinded_intermediate_secret_;
};

#endif