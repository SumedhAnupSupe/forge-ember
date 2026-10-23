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
    char name[64];      // Fixed size
    char email[128];    // Fixed size
    char password_hash[256]; // Fixed size
    uint64_t created_at;
    
    static const size_t RECORD_SIZE = 64 + 128 + 256 + 4 + 8; // ~460 bytes
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
    
    static const size_t RECORD_SIZE = 4 + 4 + 128 + 512 + 8 + 1 + 1 + 1; // ~658 bytes
};

// Recipient record
struct Recipient {
    uint32_t message_id;
    uint32_t user_id;
    char type[4]; // TO, CC, BCC (3 chars + null)
    
    static const size_t RECORD_SIZE = 4 + 4 + 4; // ~12 bytes
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
