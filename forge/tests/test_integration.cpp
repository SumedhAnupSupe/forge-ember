#include <iostream>
#include <filesystem>
#include "storage/storage.h"
#include "services/user_service.h"
#include "index/indexes.h"

using namespace forge;

int test_integration() {
    int failures = 0;
    std::cout << "Testing Integration..." << std::endl;
    std::string tmpdir = "/tmp/forge_test_i_" + std::to_string(time(nullptr));
    std::filesystem::create_directories(tmpdir);
    {
        auto storage = std::make_shared<Storage>(tmpdir);
        if (!storage->open()) { failures++; return failures; }
        auto im = std::make_shared<IndexManager>(storage);
        im->buildIndexes();
        auto us = std::make_shared<UserService>(storage, im);
        User u1; us->createUser("Alice", "alice@example.com", "$argon2id$dummy", u1);
        storage->close();
    }
    {
        auto storage = std::make_shared<Storage>(tmpdir);
        if (!storage->open()) { failures++; return failures; }
        auto users = storage->getAllUsers(false);
        if (!users.empty()) std::cout << "  Integration restart OK" << std::endl;
        else { failures++; }
        storage->close();
    }
    std::filesystem::remove_all(tmpdir);
    return failures;
}
