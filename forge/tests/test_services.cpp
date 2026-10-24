#include <iostream>
#include <cstring>
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
    User u1;
    User u2;
    if (us->createUser("Alice", "alice@example.com", "$argon2id$dummy", u1)) {
        if (!us->createUser("Bob", "ALICE@example.com", "$argon2id$dummy", u2)) {
            std::cout << "  Case-insensitive duplicate email rejected OK" << std::endl;
        } else { std::cout << "  Duplicate email FAILED" << std::endl; failures++; }

        if (!us->createUser("Bob", "bob@example.com", "$argon2id$dummy", u2)) {
            std::cout << "  Second user creation FAILED" << std::endl;
            failures++;
        } else {
            User duplicateUpdate = u2;
            std::strncpy(duplicateUpdate.email, "alice@example.com", sizeof(duplicateUpdate.email) - 1);
            if (!us->updateUser(duplicateUpdate))
                std::cout << "  Duplicate email rejected on update OK" << std::endl;
            else { std::cout << "  Duplicate email update FAILED" << std::endl; failures++; }

            User renamedEmail = u2;
            std::strncpy(renamedEmail.email, "bob.renamed@example.com", sizeof(renamedEmail.email) - 1);
            if (us->updateUser(renamedEmail)) {
                User lookup{};
                const bool oldEmailGone = !us->getUserByEmail("bob@example.com", lookup);
                const bool newEmailWorks = us->getUserByEmail("BOB.RENAMED@example.com", lookup) &&
                                           lookup.id == u2.id;
                if (oldEmailGone && newEmailWorks)
                    std::cout << "  Email update index consistency OK" << std::endl;
                else { std::cout << "  Email update index consistency FAILED" << std::endl; failures++; }
                u2 = renamedEmail;
            } else { std::cout << "  Email update FAILED" << std::endl; failures++; }

            MessageService messages(storage, im);
            Message sent{};
            const std::vector<std::pair<uint32_t, RecipientType>> recipients = {
                {u2.id, RecipientType::TO}
            };
            if (!messages.createMessage(u1.id, "Hello", "Test message", recipients, sent)) {
                std::cout << "  Message creation FAILED" << std::endl;
                failures++;
            } else {
                const auto senderMessages = messages.getMessagesForUser(u1.id);
                const auto recipientMessages = messages.getMessagesForUser(u2.id);
                const bool senderCanSeeSent = senderMessages.size() == 1 && senderMessages[0].id == sent.id;
                const bool recipientCanSeeReceived = recipientMessages.size() == 1 && recipientMessages[0].id == sent.id;
                if (senderCanSeeSent && recipientCanSeeReceived)
                    std::cout << "  Sent and received message listing OK" << std::endl;
                else {
                    std::cout << "  Sent/received message listing FAILED" << std::endl;
                    failures++;
                }
            }
        }
    } else { failures++; }
    storage->close();
    std::filesystem::remove_all(tmpdir);
    return failures;
}
