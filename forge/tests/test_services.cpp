#include <iostream>
#include <cstring>
#include "storage/storage.h"
#include "services/user_service.h"
#include "services/message_service.h"
#include "index/indexes.h"
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <set>

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
            const std::vector<std::pair<uint32_t, RecipientType>> duplicateRecipients = {
                {u2.id, RecipientType::TO}, {u2.id, RecipientType::CC}
            };
            Message rejected{};
            if (!messages.createMessage(u1.id, "Duplicate", "Should fail", duplicateRecipients, rejected))
                std::cout << "  Duplicate message recipients rejected OK" << std::endl;
            else { std::cout << "  Duplicate message recipients accepted FAILED" << std::endl; failures++; }

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

                MailboxState bobState{};
                if (storage->readMailboxState(sent.id, u2.id, bobState)) {
                    bobState.is_read = 1;
                    if (storage->updateMailboxState(bobState)) {
                        const auto senderAfterRead = messages.getMessagesForUser(u1.id);
                        const auto recipientAfterRead = messages.getMessagesForUser(u2.id);
                        const bool senderStillUnread = senderAfterRead.size() == 1 && !senderAfterRead[0].is_read;
                        const bool recipientRead = recipientAfterRead.size() == 1 && recipientAfterRead[0].is_read;
                        if (senderStillUnread && recipientRead)
                            std::cout << "  Independent per-user read state OK" << std::endl;
                        else { std::cout << "  Independent per-user read state FAILED" << std::endl; failures++; }
                        bobState.is_deleted = 1;
                        if (!storage->updateMailboxState(bobState)) {
                            std::cout << "  Per-user trash update FAILED" << std::endl;
                            failures++;
                        } else {
                            const auto senderAfterTrash = messages.getMessagesForUser(u1.id);
                            const auto bobAfterTrash = messages.getMessagesForUser(u2.id);
                            if (senderAfterTrash.size() == 1 && bobAfterTrash.size() == 1 &&
                                !senderAfterTrash[0].is_deleted && bobAfterTrash[0].is_deleted)
                                std::cout << "  Independent per-user trash state OK" << std::endl;
                            else { std::cout << "  Independent per-user trash state FAILED" << std::endl; failures++; }
                            if (messages.removeMailboxCopy(sent.id, u2.id)) {
                                const auto senderAfterPurge = messages.getMessagesForUser(u1.id);
                                const auto bobAfterPurge = messages.getMessagesForUser(u2.id);
                                if (senderAfterPurge.size() == 1 && bobAfterPurge.empty())
                                    std::cout << "  Per-user permanent mailbox removal OK" << std::endl;
                                else { std::cout << "  Per-user permanent mailbox removal FAILED" << std::endl; failures++; }
                            } else {
                                std::cout << "  Per-user permanent mailbox removal FAILED" << std::endl;
                                failures++;
                            }

                            storage->close();
                            const bool reopened = storage->open();
                            if (reopened) im->buildIndexes();
                            if (reopened && messages.getMessagesForUser(u1.id).size() == 1 &&
                                messages.getMessagesForUser(u2.id).empty())
                                std::cout << "  Restart does not resurrect removed mailbox copy OK" << std::endl;
                            else { std::cout << "  Removed mailbox copy resurrected after restart" << std::endl; failures++; }

                            if (messages.removeMailboxCopy(sent.id, u1.id)) {
                                Message purgedMessage{};
                                std::vector<Recipient> purgedRecipients;
                                std::vector<uint32_t> staleRecipients;
                                const bool bodyPurged = !storage->readMessage(sent.id, purgedMessage);
                                const bool recipientsPurged =
                                    !storage->readRecipientsByMessage(sent.id, purgedRecipients, true) ||
                                    purgedRecipients.empty();
                                const bool indexesCleared = !im->hasMessageId(sent.id) &&
                                    !im->findRecipientsByMessage(sent.id, staleRecipients);
                                if (bodyPurged && recipientsPurged && indexesCleared &&
                                    messages.getMessagesForUser(u1.id).empty() &&
                                    messages.getMessagesForUser(u2.id).empty())
                                    std::cout << "  Last participant removal physically purges shared records and indexes OK" << std::endl;
                                else { std::cout << "  Last participant purge FAILED" << std::endl; failures++; }
                            } else {
                                std::cout << "  Last participant purge failed" << std::endl;
                                failures++;
                            }
                        }
                    } else {
                        std::cout << "  Mailbox state update FAILED" << std::endl;
                        failures++;
                    }
                } else {
                    std::cout << "  Mailbox state creation FAILED" << std::endl;
                    failures++;
                }
            }
        }
    } else { failures++; }
    storage->close();

    // Concurrent message creation exercises ID allocation, journal commits,
    // mailbox-state creation, and index updates under competing callers.
    const std::string concurrentDir = tmpdir + "_concurrent";
    auto concurrentStorage = std::make_shared<Storage>(concurrentDir);
    if (!concurrentStorage->open()) {
        std::cout << "  Concurrent storage startup FAILED" << std::endl;
        failures++;
    } else {
        auto concurrentIndexes = std::make_shared<IndexManager>(concurrentStorage);
        concurrentIndexes->buildIndexes();
        auto concurrentUsers = std::make_shared<UserService>(concurrentStorage, concurrentIndexes);
        User sender{}, recipient{};
        if (!concurrentUsers->createUser("Concurrent Sender", "sender-concurrent@example.test", "hash", sender) ||
            !concurrentUsers->createUser("Concurrent Recipient", "recipient-concurrent@example.test", "hash", recipient)) {
            std::cout << "  Concurrent user setup FAILED" << std::endl;
            failures++;
        } else {
            MessageService concurrentMessages(concurrentStorage, concurrentIndexes);
            std::atomic<int> createFailures{0};
            std::mutex idsMutex;
            std::vector<uint32_t> createdIds;
            std::vector<std::thread> workers;
            constexpr int threadCount = 8;
            constexpr int messagesPerThread = 10;
            for (int threadId = 0; threadId < threadCount; ++threadId) {
                workers.emplace_back([&, threadId]() {
                    for (int sequence = 0; sequence < messagesPerThread; ++sequence) {
                        Message created{};
                        const std::string subject = "parallel-" + std::to_string(threadId) +
                                                    "-" + std::to_string(sequence);
                        if (!concurrentMessages.createMessage(sender.id, subject, "concurrent body",
                                {{recipient.id, RecipientType::TO}}, created)) {
                            ++createFailures;
                            continue;
                        }
                        std::lock_guard<std::mutex> lock(idsMutex);
                        createdIds.push_back(created.id);
                    }
                });
            }
            for (auto& worker : workers) worker.join();
            const std::set<uint32_t> uniqueIds(createdIds.begin(), createdIds.end());
            const auto allMessages = concurrentStorage->getAllMessages();
            if (createFailures.load() == 0 &&
                createdIds.size() == threadCount * messagesPerThread &&
                uniqueIds.size() == createdIds.size() &&
                allMessages.size() == createdIds.size())
                std::cout << "  Concurrent message creation and unique IDs OK" << std::endl;
            else { std::cout << "  Concurrent message creation FAILED" << std::endl; failures++; }
        }
        concurrentStorage->close();
    }
    std::filesystem::remove_all(concurrentDir);
    std::filesystem::remove_all(tmpdir);
    return failures;
}
