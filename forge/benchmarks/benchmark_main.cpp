#include "storage/storage.h"
#include "index/hash_table.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace forge;
using Clock = std::chrono::steady_clock;

template <typename Fn>
double measureMilliseconds(Fn&& fn) {
    const auto start = Clock::now();
    fn();
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

int main(int argc, char** argv) {
    size_t messageCount = 100;
    if (argc == 3 && std::string(argv[1]) == "--messages") {
        try {
            messageCount = static_cast<size_t>(std::stoul(argv[2]));
        } catch (...) {
            std::cerr << "Usage: forge-benchmarks [--messages N]\n";
            return 2;
        }
    }
    if (messageCount == 0 || messageCount > 1000) {
        std::cerr << "Message count must be between 1 and 1000\n";
        return 2;
    }

    const auto dataDir = std::filesystem::temp_directory_path() /
        ("forge_benchmark_" + std::to_string(
            std::chrono::duration_cast<std::chrono::microseconds>(
                Clock::now().time_since_epoch()).count()));
    std::filesystem::remove_all(dataDir);

    Storage storage(dataDir.string());
    if (!storage.open()) {
        std::cerr << "Could not open benchmark storage\n";
        return 1;
    }

    User user{};
    user.id = 1;
    std::strcpy(user.name, "Benchmark User");
    std::strcpy(user.email, "benchmark@example.test");
    std::strcpy(user.password_hash, "benchmark-hash");
    if (!storage.createUser(user)) {
        std::cerr << "Could not create benchmark user\n";
        storage.close();
        std::filesystem::remove_all(dataDir);
        return 1;
    }

    const double writeMs = measureMilliseconds([&]() {
        for (size_t i = 0; i < messageCount; ++i) {
            Message message{};
            message.id = storage.getNextMessageId();
            message.sender_id = user.id;
            const std::string subject = "Benchmark message " + std::to_string(i);
            std::strncpy(message.subject, subject.c_str(), sizeof(message.subject) - 1);
            std::strncpy(message.body, "Fixed-size record write benchmark",
                         sizeof(message.body) - 1);

            Recipient recipient{};
            recipient.message_id = message.id;
            recipient.user_id = user.id;
            std::strcpy(recipient.type, "TO");
            if (!storage.createMessageWithRecipients(message, {recipient})) {
                std::cerr << "Message transaction failed at row " << i << "\n";
                storage.close();
                std::filesystem::remove_all(dataDir);
                return 1;
            }
        }
    });

    std::vector<Message> messages;
    std::vector<Recipient> recipients;
    std::vector<MailboxState> mailboxStates;
    const double messageScanMs = measureMilliseconds([&]() {
        messages = storage.getAllMessages(false);
    });
    const double recipientScanMs = measureMilliseconds([&]() {
        recipients = storage.getAllRecipients(false);
    });
    const double mailboxScanMs = measureMilliseconds([&]() {
        mailboxStates = storage.getAllMailboxStates();
    });

    HashTable index(2048);
    const double indexInsertMs = measureMilliseconds([&]() {
        for (uint32_t i = 1; i <= messageCount; ++i)
            index.insert("message-" + std::to_string(i), i);
    });
    size_t foundCount = 0;
    const double indexLookupMs = measureMilliseconds([&]() {
        for (uint32_t i = 1; i <= messageCount; ++i) {
            std::vector<uint32_t> values;
            if (index.find("message-" + std::to_string(i), values) &&
                values.size() == 1 && values.front() == i) ++foundCount;
        }
    });

    const bool correct = messages.size() == messageCount &&
        recipients.size() == messageCount &&
        mailboxStates.size() == messageCount &&
        foundCount == messageCount;

    std::cout << "FORGE benchmark (temporary local storage)\n";
    std::cout << "messages=" << messageCount << "\n";
    std::cout << "message_create_total_ms=" << writeMs << "\n";
    std::cout << "message_create_ops_per_sec="
              << (writeMs > 0 ? messageCount * 1000.0 / writeMs : 0) << "\n";
    std::cout << "message_scan_ms=" << messageScanMs << "\n";
    std::cout << "recipient_scan_ms=" << recipientScanMs << "\n";
    std::cout << "mailbox_state_scan_ms=" << mailboxScanMs << "\n";
    std::cout << "hash_insert_ms=" << indexInsertMs << "\n";
    std::cout << "hash_lookup_ms=" << indexLookupMs << "\n";
    std::cout << "verified_records=" << (correct ? "true" : "false") << "\n";

    storage.close();
    std::filesystem::remove_all(dataDir);
    return correct ? 0 : 1;
}
