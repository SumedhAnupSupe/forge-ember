#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
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
            if (!users.empty()) {
                std::cout << "  Restart persistence OK" << std::endl;
                const uint32_t nextId = s.getNextUserId();
                const uint32_t maxStoredId = std::max_element(users.begin(), users.end(),
                    [](const User& a, const User& b) { return a.id < b.id; })->id;
                if (nextId > maxStoredId)
                    std::cout << "  User ID counter restored after restart OK" << std::endl;
                else { std::cout << "  User ID counter restoration FAILED" << std::endl; failures++; }
            } else { std::cout << "  Restart persistence FAILED" << std::endl; failures++; }
            s.close();
        }
    }
    std::filesystem::remove_all(tmpdir);

    const auto corruptDir = std::filesystem::temp_directory_path() /
        ("forge_corrupt_storage_test_" + std::to_string(time(nullptr)));
    std::filesystem::create_directories(corruptDir);
    const auto corruptUsers = corruptDir / "users.dat";
    {
        std::ofstream file(corruptUsers, std::ios::binary);
        file.put('X');
    }
    {
        Storage corruptStorage(corruptDir.string());
        if (!corruptStorage.open()) {
            const auto size = std::filesystem::file_size(corruptUsers);
            if (size == 1)
                std::cout << "  Corrupt file rejected without truncation OK" << std::endl;
            else { std::cout << "  Corrupt file was modified FAILED" << std::endl; failures++; }
        } else {
            std::cout << "  Corrupt file unexpectedly opened FAILED" << std::endl;
            failures++;
            corruptStorage.close();
        }
    }
    std::filesystem::remove_all(corruptDir);

    // A legacy directory with valid empty record files is upgraded by adding
    // metadata; the existing record files remain untouched.
    const auto legacyDir = std::filesystem::temp_directory_path() /
        ("forge_legacy_storage_test_" + std::to_string(time(nullptr)));
    std::filesystem::create_directories(legacyDir);
    for (const char* name : {"users.dat", "messages.dat", "recipients.dat"}) {
        std::ofstream file(legacyDir / name, std::ios::binary);
    }
    {
        Storage legacyStorage(legacyDir.string());
        if (legacyStorage.open()) {
            legacyStorage.close();
            const auto manifest = legacyDir / "format.meta";
            std::ifstream input(manifest);
            std::string magic;
            unsigned int version = 0;
            size_t userSize = 0, messageSize = 0, recipientSize = 0;
            if (input >> magic >> version >> userSize >> messageSize >> recipientSize &&
                magic == "FORGE_STORAGE" && version == 1 &&
                userSize == User::ENCODED_RECORD_SIZE &&
                messageSize == Message::ENCODED_RECORD_SIZE &&
                recipientSize == Recipient::ENCODED_RECORD_SIZE) {
                std::cout << "  Legacy storage metadata migration OK" << std::endl;
            } else {
                std::cout << "  Legacy storage metadata migration FAILED" << std::endl;
                failures++;
            }
        } else {
            std::cout << "  Legacy storage metadata migration FAILED" << std::endl;
            failures++;
        }
    }
    std::filesystem::remove_all(legacyDir);

    const auto invalidManifestDir = std::filesystem::temp_directory_path() /
        ("forge_invalid_manifest_test_" + std::to_string(time(nullptr)));
    std::filesystem::create_directories(invalidManifestDir);
    {
        std::ofstream manifest(invalidManifestDir / "format.meta");
        manifest << "NOT_FORGE 99 1 1 1\\n";
    }
    {
        Storage invalidManifestStorage(invalidManifestDir.string());
        if (!invalidManifestStorage.open()) {
            std::cout << "  Unsupported storage manifest rejected OK" << std::endl;
        } else {
            std::cout << "  Unsupported storage manifest accepted FAILED" << std::endl;
            failures++;
            invalidManifestStorage.close();
        }
    }
    std::filesystem::remove_all(invalidManifestDir);
    return failures;
}
