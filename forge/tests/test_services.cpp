#include <iostream>
#include "storage/storage.h"
#include "services/user_service.h"
#include "services/message_service.h"
#include "index/indexes.h"
#include <filesystem>

using namespace forge;

int test_services() {
    int failures = 0;
    std::cout << "Testing Services..." << std::endl;
    std::string tmpdir = "/tmp/forge_test_s_" + std::to_string(time(nullptr));
    std::filesystem::create_directories(tmpdir);
    auto storage = std::make_shared<Storage>(tmpdir);
    if (!storage->open()) { failures++; return failures; }
    auto im = std::make_shared<IndexManager>(storage);
    im->buildIndexes();
    auto us = std::make_shared<UserService>(storage, im);
    User u1; if (us->createUser("Alice", "alice@example.com", "$argon2id$dummy", u1)) {
        User u2; if (!us->createUser("Bob", "alice@example.com", "$argon2id$dummy", u2)) {
            std::cout << "  Duplicate email rejected OK" << std::endl;
        } else { std::cout << "  Duplicate email FAILED" << std::endl; failures++; }
    } else { failures++; }
    storage->close();
    std::filesystem::remove_all(tmpdir);
    return failures;
}
