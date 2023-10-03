#ifndef STR_DH_KEY_TREE
#define STR_DH_KEY_TREE

#include <cryptopp/integer.h>
#include <memory>

struct str_IN {
    CryptoPP::SecByteBlock group_secret_;
    CryptoPP::SecByteBlock blinded_group_secret_;
};

struct str_LN {
    CryptoPP::SecByteBlock member_secret_;
    CryptoPP::SecByteBlock blinded_member_secret_;
};

struct str_key_tree {
    str_IN root_node_;
    str_LN leaf_node_;
    std::unique_ptr<str_key_tree> next_internal_node_;
};

#endif