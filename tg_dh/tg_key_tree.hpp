#ifndef TG_DH_KEY_TREE
#define TG_DH_KEY_TREE

#include <cryptopp/integer.h>
#include <memory>

struct tg_key_node {
    CryptoPP::Integer member_secret_;
    CryptoPP::Integer blinded_member_secret_;
    std::unique_ptr<tg_key_node> left_node_;
    std::unique_ptr<tg_key_node> right_node_;
    int right_height_;
    int left_height_;
};

struct tg_key_tree {
    tg_key_node tg_key_node;
};

#endif