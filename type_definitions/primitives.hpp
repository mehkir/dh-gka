#ifndef PRIMITIVES
#define PRIMITIVES

#include <cstdint>

#define DEFAULT_MEMBER_ID 0
#define DEFAULT_SERVICE_ID 0
#define DEFAULT_SECRET CryptoPP::SecByteBlock()

typedef int service_id_t;
typedef uint16_t member_id_t;
typedef uint8_t message_id_t;
typedef CryptoPP::SecByteBlock blinded_secret_t;
typedef CryptoPP::SecByteBlock secret_t;

#endif