#include <iostream>
#include <filesystem>
#include <cstring>
#include "storage/storage.h"

using namespace forge;

int test_storage() {
    int failures = 0;
    std::cout << "Testing Storage..." << std::endl;
    std::string tmpdir = "/tmp/forge_test_" + std::to_string(time(nullptr));
    std::filesystem::create_directories(tmpdir);
    {
        Storage s(tmpdir);
        if (!s.open()) { std::cout << "  Open failed" << std::endl; failures++; return failures; }
        User u; memset(&u, 0, sizeof(u));
        u.id = s.getNextUserId();
        strcpy(u.name, "Alice");
        strcpy(u.email, "alice@example.com");
        strcpy(u.password_hash, "$argon2id$dummy");
        u.created_at = 123;
        u.is_deleted = 0;
        if (s.createUser(u)) {
            User u2;
            if (s.readUser(u.id, u2)) std::cout << "  Create/read user OK" << std::endl;
            else { std::cout << "  Read user FAILED" << std::endl; failures++; }
        } else {
            std::cout << "  Create user FAILED" << std::endl; failures++;
        }
        s.setNextUserId(UINT32_MAX);
        const uint32_t lastUserId = s.getNextUserId();
        const uint32_t exhaustedUserId = s.getNextUserId();
        if (lastUserId == UINT32_MAX && exhaustedUserId == 0)
            std::cout << "  User ID exhaustion handled without wraparound OK" << std::endl;
        else { std::cout << "  User ID exhaustion FAILED" << std::endl; failures++; }

        s.setNextMessageId(UINT32_MAX);
        const uint32_t lastMessageId = s.getNextMessageId();
        const uint32_t exhaustedMessageId = s.getNextMessageId();
        if (lastMessageId == UINT32_MAX && exhaustedMessageId == 0)
            std::cout << "  Message ID exhaustion handled without wraparound OK" << std::endl;
        else { std::cout << "  Message ID exhaustion FAILED" << std::endl; failures++; }

        s.close();
    }
    // reopen
    {
        Storage s(tmpdir);
        if (!s.open()) { std::cout << "  Reopen failed" << std::endl; failures++; }
        else {
            auto users = s.getAllUsers(false);
            if (!users.empty()) std::cout << "  Restart persistence OK" << std::endl;
            else { std::cout << "  Restart persistence FAILED" << std::endl; failures++; }
            s.close();
        }
    }
    std::filesystem::remove_all(tmpdir);
    return failures;
}
