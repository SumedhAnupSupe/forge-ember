#include <iostream>
#include <cstring>
#include <vector>
#include "utils/codec.h"

using namespace forge;

int test_codec() {
    int failures = 0;
    std::cout << "Testing Codec..." << std::endl;
    User user;
    memset(&user, 0, sizeof(user));
    user.id = 1;
    strcpy(user.name, "Test User");
    strcpy(user.email, "test@example.com");
    strcpy(user.password_hash, "hash123");
    user.created_at = 123456789;
    user.is_deleted = 0;
    
    try {
        auto serialized = BinaryCodec::serializeUser(user);
        if (serialized.size() == User::ENCODED_RECORD_SIZE) {
            std::cout << "  User serialization size OK" << std::endl;
        } else {
            std::cout << "  User serialization size FAILED" << std::endl;
            failures++;
        }
        User user2;
        memset(&user2, 0, sizeof(user2));
        if (BinaryCodec::deserializeUser(serialized, 0, user2)) {
            if (user2.id == user.id && strcmp(user2.email, user.email) == 0 && user2.is_deleted == 0) {
                std::cout << "  User round-trip OK" << std::endl;
            } else {
                std::cout << "  User round-trip FAILED" << std::endl;
                failures++;
            }
        } else {
            failures++;
        }
    } catch (...) {
        std::cout << "  User codec FAILED" << std::endl;
        failures++;
    }
    
    // Test truncated
    std::vector<uint8_t> bad(User::ENCODED_RECORD_SIZE - 1);
    User user3;
    if (BinaryCodec::deserializeUser(bad, 0, user3)) {
        std::cout << "  Truncated detection FAILED" << std::endl;
        failures++;
    } else {
        std::cout << "  Truncated detection OK" << std::endl;
    }
    
    return failures;
}
