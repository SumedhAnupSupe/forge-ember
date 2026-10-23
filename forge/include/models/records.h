#ifndef RECORDS_H
#define RECORDS_H

#include <string>
#include <vector>
#include <ctime>
#include <cstdint>

namespace forge {

// User record
struct User {
    uint32_t id;
    char name[64];
    char email[128];
    char password_hash[256];
    uint64_t created_at;
    uint8_t is_deleted;
    
    static const size_t ENCODED_RECORD_SIZE = 4 + 64 + 128 + 256 + 8 + 1; // 461 bytes
    static const size_t RECORD_SIZE = ENCODED_RECORD_SIZE;
};

// Message record
struct Message {
    uint32_t id;
    uint32_t sender_id;
    char subject[128];
    char body[512];
    uint64_t created_at;
    uint8_t is_read;
    uint8_t is_starred;
    uint8_t is_deleted;
    
    static const size_t ENCODED_RECORD_SIZE = 4 + 4 + 128 + 512 + 8 + 1 + 1 + 1; // 659 bytes
    static const size_t RECORD_SIZE = ENCODED_RECORD_SIZE;
};

// Recipient record
struct Recipient {
    uint32_t message_id;
    uint32_t user_id;
    char type[4]; // TO, CC, BCC
    uint8_t is_deleted;
    
    static const size_t ENCODED_RECORD_SIZE = 4 + 4 + 4 + 1; // 13 bytes
    static const size_t RECORD_SIZE = ENCODED_RECORD_SIZE;
};

enum class RecipientType {
    TO,
    CC,
    BCC
};

// Helper functions to convert
RecipientType stringToRecipientType(const std::string& type);
std::string recipientTypeToString(RecipientType type);

} // namespace forge

#endif // RECORDS_H
