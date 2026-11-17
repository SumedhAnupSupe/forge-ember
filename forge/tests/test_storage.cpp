#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <vector>
#include <iterator>
#include "storage/storage.h"
#include "utils/codec.h"

using namespace forge;

void appendJournalU32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xff));
}

uint32_t journalTestChecksum(const std::vector<uint8_t>& bytes) {
    uint32_t hash = 2166136261u;
    for (uint8_t byte : bytes) { hash ^= byte; hash *= 16777619u; }
    return hash;
}

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
            size_t userSize = 0, messageSize = 0, recipientSize = 0, stateSize = 0;
            if (input >> magic >> version >> userSize >> messageSize >> recipientSize >> stateSize &&
                magic == "FORGE_STORAGE" && version == 2 &&
                userSize == User::ENCODED_RECORD_SIZE &&
                messageSize == Message::ENCODED_RECORD_SIZE &&
                recipientSize == Recipient::ENCODED_RECORD_SIZE &&
                stateSize == MailboxState::ENCODED_RECORD_SIZE) {
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

    const auto versionOneDir = std::filesystem::temp_directory_path() /
        ("forge_v1_manifest_test_" + std::to_string(time(nullptr)));
    std::filesystem::create_directories(versionOneDir);
    for (const char* name : {"users.dat", "messages.dat", "recipients.dat"}) {
        std::ofstream file(versionOneDir / name, std::ios::binary);
    }
    {
        std::ofstream manifest(versionOneDir / "format.meta");
        manifest << "FORGE_STORAGE\n1\n"
                 << User::ENCODED_RECORD_SIZE << " "
                 << Message::ENCODED_RECORD_SIZE << " "
                 << Recipient::ENCODED_RECORD_SIZE << "\n";
    }
    {
        Storage versionOneStorage(versionOneDir.string());
        if (versionOneStorage.open()) {
            versionOneStorage.close();
            std::ifstream manifest(versionOneDir / "format.meta");
            std::string magic;
            unsigned int version = 0;
            manifest >> magic >> version;
            if (magic == "FORGE_STORAGE" && version == 2)
                std::cout << "  Version 1 to version 2 manifest upgrade OK" << std::endl;
            else { std::cout << "  Version 1 to version 2 manifest upgrade FAILED" << std::endl; failures++; }
        } else {
            std::cout << "  Version 1 to version 2 manifest upgrade FAILED" << std::endl;
            failures++;
        }
    }
    std::filesystem::remove(versionOneDir / "mailbox_states.dat");
    {
        Storage missingStateStorage(versionOneDir.string());
        if (!missingStateStorage.open()) {
            std::cout << "  Missing v2 mailbox-state file rejected OK" << std::endl;
        } else {
            std::cout << "  Missing v2 mailbox-state file recreated FAILED" << std::endl;
            failures++;
            missingStateStorage.close();
        }
    }
    std::filesystem::remove_all(versionOneDir);

    // Existing message-level flags are copied to the sender and each active
    // recipient during the version 1 -> 2 mailbox-state migration.
    const auto legacyDataDir = std::filesystem::temp_directory_path() /
        ("forge_legacy_state_migration_test_" + std::to_string(time(nullptr)));
    std::filesystem::create_directories(legacyDataDir);
    User legacySender{};
    legacySender.id = 1;
    strcpy(legacySender.name, "Sender");
    strcpy(legacySender.email, "sender@example.com");
    strcpy(legacySender.password_hash, "hash");
    User legacyRecipient{};
    legacyRecipient.id = 2;
    strcpy(legacyRecipient.name, "Recipient");
    strcpy(legacyRecipient.email, "recipient@example.com");
    strcpy(legacyRecipient.password_hash, "hash");
    Message legacyMessage{};
    legacyMessage.id = 77;
    legacyMessage.sender_id = 1;
    strcpy(legacyMessage.subject, "Legacy");
    strcpy(legacyMessage.body, "Legacy body");
    legacyMessage.is_read = 1;
    legacyMessage.is_starred = 1;
    Recipient legacyRecipientRow{};
    legacyRecipientRow.message_id = 77;
    legacyRecipientRow.user_id = 2;
    strcpy(legacyRecipientRow.type, "TO");
    {
        std::ofstream users(legacyDataDir / "users.dat", std::ios::binary);
        const auto senderBytes = BinaryCodec::serializeUser(legacySender);
        const auto recipientBytes = BinaryCodec::serializeUser(legacyRecipient);
        users.write(reinterpret_cast<const char*>(senderBytes.data()), senderBytes.size());
        users.write(reinterpret_cast<const char*>(recipientBytes.data()), recipientBytes.size());
        std::ofstream messages(legacyDataDir / "messages.dat", std::ios::binary);
        const auto messageBytes = BinaryCodec::serializeMessage(legacyMessage);
        messages.write(reinterpret_cast<const char*>(messageBytes.data()), messageBytes.size());
        std::ofstream recipients(legacyDataDir / "recipients.dat", std::ios::binary);
        const auto recipientRowBytes = BinaryCodec::serializeRecipient(legacyRecipientRow);
        recipients.write(reinterpret_cast<const char*>(recipientRowBytes.data()), recipientRowBytes.size());
        std::ofstream manifest(legacyDataDir / "format.meta");
        manifest << "FORGE_STORAGE\n1\n"
                 << User::ENCODED_RECORD_SIZE << " "
                 << Message::ENCODED_RECORD_SIZE << " "
                 << Recipient::ENCODED_RECORD_SIZE << "\n";
    }
    {
        Storage legacyStateStorage(legacyDataDir.string());
        if (legacyStateStorage.open()) {
            MailboxState senderState{}, recipientState{};
            const bool senderMigrated = legacyStateStorage.readMailboxState(77, 1, senderState);
            const bool recipientMigrated = legacyStateStorage.readMailboxState(77, 2, recipientState);
            if (senderMigrated && recipientMigrated &&
                senderState.is_read == 1 && recipientState.is_read == 1 &&
                senderState.is_starred == 1 && recipientState.is_starred == 1)
                std::cout << "  Legacy per-message flags migrated to each mailbox OK" << std::endl;
            else { std::cout << "  Legacy per-message flag migration FAILED" << std::endl; failures++; }
            legacyStateStorage.close();
        } else {
            std::cout << "  Legacy per-message flag migration FAILED" << std::endl;
            failures++;
        }
    }
    std::filesystem::remove_all(legacyDataDir);

    const auto idScanDir = std::filesystem::temp_directory_path() /
        ("forge_id_scan_test_" + std::to_string(time(nullptr)));
    std::filesystem::create_directories(idScanDir);
    {
        Storage idStorage(idScanDir.string());
        User user{};
        user.id = 7;
        std::strcpy(user.name, "ID Scan");
        std::strcpy(user.email, "id-scan@example.com");
        std::strcpy(user.password_hash, "hash");
        Message message{};
        message.id = 9;
        message.sender_id = 7;
        std::strcpy(message.subject, "ID scan");
        std::strcpy(message.body, "restart");
        if (!idStorage.open() || !idStorage.createUser(user) || !idStorage.createMessage(message)) {
            std::cout << "  ID scan fixture setup FAILED" << std::endl;
            failures++;
        } else {
            idStorage.close();
            if (!idStorage.open() || idStorage.getNextUserId() != 8 ||
                idStorage.getNextMessageId() != 10) {
                std::cout << "  Restart ID scan FAILED" << std::endl;
                failures++;
            } else std::cout << "  Restart ID scan restores next IDs OK" << std::endl;
        }
        idStorage.close();
    }
    std::filesystem::remove_all(idScanDir);

    const auto overflowDir = std::filesystem::temp_directory_path() /
        ("forge_id_overflow_test_" + std::to_string(time(nullptr)));
    std::filesystem::create_directories(overflowDir);
    {
        Storage overflowStorage(overflowDir.string());
        User user{};
        user.id = UINT32_MAX;
        std::strcpy(user.name, "Maximum ID");
        std::strcpy(user.email, "max-id@example.com");
        std::strcpy(user.password_hash, "hash");
        Message message{};
        message.id = UINT32_MAX;
        message.sender_id = UINT32_MAX;
        std::strcpy(message.subject, "Maximum ID");
        std::strcpy(message.body, "overflow");
        if (!overflowStorage.open() || !overflowStorage.createUser(user) ||
            !overflowStorage.createMessage(message)) {
            std::cout << "  ID overflow fixture setup FAILED" << std::endl;
            failures++;
        } else {
            overflowStorage.close();
            if (!overflowStorage.open() || overflowStorage.getNextUserId() != 0 ||
                overflowStorage.getNextMessageId() != 0) {
                std::cout << "  ID exhaustion guard FAILED" << std::endl;
                failures++;
            } else std::cout << "  ID exhaustion guard OK" << std::endl;
        }
        overflowStorage.close();
    }
    std::filesystem::remove_all(overflowDir);


    // Simulate a process crash after partial message/recipient appends while
    // the complete write-ahead journal is durable.
    const auto journalDir = std::filesystem::temp_directory_path() /
        ("forge_journal_recovery_test_" + std::to_string(time(nullptr)));
    std::filesystem::create_directories(journalDir);
    Message journalMessage{};
    journalMessage.id = 77;
    journalMessage.sender_id = 1;
    std::strcpy(journalMessage.subject, "Journal recovery");
    std::strcpy(journalMessage.body, "replayed exactly once");
    Recipient journalRecipient{};
    journalRecipient.message_id = 77;
    journalRecipient.user_id = 2;
    std::strcpy(journalRecipient.type, "TO");
    const auto journalMessageBytes = BinaryCodec::serializeMessage(journalMessage);
    const auto journalRecipientBytes = BinaryCodec::serializeRecipient(journalRecipient);
    std::vector<uint8_t> journalPayload = journalMessageBytes;
    journalPayload.insert(journalPayload.end(), journalRecipientBytes.begin(), journalRecipientBytes.end());
    std::vector<uint8_t> journalBytes;
    const char journalMagic[8] = {'F', 'O', 'R', 'G', 'E', 'W', 'A', 'L'};
    journalBytes.insert(journalBytes.end(), std::begin(journalMagic), std::end(journalMagic));
    appendJournalU32(journalBytes, 1);
    appendJournalU32(journalBytes, static_cast<uint32_t>(Message::ENCODED_RECORD_SIZE));
    appendJournalU32(journalBytes, static_cast<uint32_t>(Recipient::ENCODED_RECORD_SIZE));
    appendJournalU32(journalBytes, 1);
    appendJournalU32(journalBytes, static_cast<uint32_t>(journalPayload.size()));
    appendJournalU32(journalBytes, journalTestChecksum(journalPayload));
    journalBytes.insert(journalBytes.end(), journalPayload.begin(), journalPayload.end());
    {
        std::ofstream messages(journalDir / "messages.dat", std::ios::binary);
        messages.write(reinterpret_cast<const char*>(journalMessageBytes.data()),
                       static_cast<std::streamsize>(journalMessageBytes.size() / 2));
        std::ofstream recipients(journalDir / "recipients.dat", std::ios::binary);
        recipients.write(reinterpret_cast<const char*>(journalRecipientBytes.data()),
                         static_cast<std::streamsize>(journalRecipientBytes.size() / 2));
        std::ofstream users(journalDir / "users.dat", std::ios::binary);
        std::ofstream journal(journalDir / "message.txn", std::ios::binary);
        journal.write(reinterpret_cast<const char*>(journalBytes.data()),
                      static_cast<std::streamsize>(journalBytes.size()));
    }
    {
        Storage journalStorage(journalDir.string());
        Message recoveredMessage{};
        std::vector<Recipient> recoveredRecipients;
        MailboxState senderState{}, recipientState{};
        if (journalStorage.open() &&
            journalStorage.readMessage(77, recoveredMessage) &&
            journalStorage.readRecipientsByMessage(77, recoveredRecipients, true) &&
            recoveredRecipients.size() == 1 &&
            journalStorage.readMailboxState(77, 1, senderState) &&
            journalStorage.readMailboxState(77, 2, recipientState) &&
            !std::filesystem::exists(journalDir / "message.txn")) {
            journalStorage.close();
            if (journalStorage.open() && journalStorage.getAllMessages(true).size() == 1 &&
                journalStorage.getAllRecipients(true).size() == 1)
                std::cout << "  WAL recovery replays partial writes exactly once OK" << std::endl;
            else { std::cout << "  WAL recovery idempotency FAILED" << std::endl; failures++; }
        } else {
            std::cout << "  WAL recovery of partial writes FAILED" << std::endl;
            failures++;
        }
        journalStorage.close();
    }
    std::filesystem::remove_all(journalDir);

    const auto invalidManifestDir = std::filesystem::temp_directory_path() /
        ("forge_invalid_manifest_test_" + std::to_string(time(nullptr)));
    std::filesystem::create_directories(invalidManifestDir);
    {
        std::ofstream manifest(invalidManifestDir / "format.meta");
        manifest << "NOT_FORGE 99 1 1 1\n";
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
