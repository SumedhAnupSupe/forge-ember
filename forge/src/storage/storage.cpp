#include "storage/storage.h"
#include "utils/codec.h"
#include <iostream>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <set>
#include <unordered_map>

namespace {

constexpr unsigned int kStorageFormatVersion = 2;

bool validateStorageFormat(const std::string& dataDir) {
    const auto path = std::filesystem::path(dataDir) / "format.meta";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return !ec;
    if (ec) return false;

    std::ifstream input(path);
    if (!input.is_open()) return false;

    std::string magic;
    unsigned int version = 0;
    size_t userRecordSize = 0;
    size_t messageRecordSize = 0;
    size_t recipientRecordSize = 0;
    size_t mailboxStateRecordSize = 0;
    std::string extra;
    if (!(input >> magic >> version >> userRecordSize >> messageRecordSize >> recipientRecordSize))
        return false;
    if (version == 2) {
        if (!(input >> mailboxStateRecordSize)) return false;
    } else if (version != 1) {
        return false;
    }
    if (input >> extra) return false;

    const bool baseLayoutMatches =
        magic == "FORGE_STORAGE" &&
        userRecordSize == forge::User::ENCODED_RECORD_SIZE &&
        messageRecordSize == forge::Message::ENCODED_RECORD_SIZE &&
        recipientRecordSize == forge::Recipient::ENCODED_RECORD_SIZE;
    return baseLayoutMatches &&
           (version == 1 || mailboxStateRecordSize == forge::MailboxState::ENCODED_RECORD_SIZE);
}

bool hasRequiredMailboxStateFile(const std::string& dataDir) {
    const auto manifestPath = std::filesystem::path(dataDir) / "format.meta";
    std::error_code ec;
    if (!std::filesystem::exists(manifestPath, ec)) return !ec;
    if (ec) return false;
    std::ifstream manifest(manifestPath);
    std::string magic;
    unsigned int version = 0;
    if (!(manifest >> magic >> version)) return false;
    if (magic != "FORGE_STORAGE") return false;
    if (version < 2) return true;
    const auto statePath = std::filesystem::path(dataDir) / "mailbox_states.dat";
    const bool exists = std::filesystem::exists(statePath, ec);
    return exists && !ec;
}

bool initializeStorageFormat(const std::string& dataDir) {
    const auto path = std::filesystem::path(dataDir) / "format.meta";
    std::error_code ec;
    const bool manifestExists = std::filesystem::exists(path, ec);
    if (ec) return false;
    if (manifestExists) {
        std::ifstream current(path);
        std::string magic;
        unsigned int version = 0;
        size_t userSize = 0, messageSize = 0, recipientSize = 0;
        if (!(current >> magic >> version >> userSize >> messageSize >> recipientSize)) return false;
        if (version == 2) return validateStorageFormat(dataDir);
        if (version != 1 || magic != "FORGE_STORAGE" ||
            userSize != forge::User::ENCODED_RECORD_SIZE ||
            messageSize != forge::Message::ENCODED_RECORD_SIZE ||
            recipientSize != forge::Recipient::ENCODED_RECORD_SIZE) return false;
    }

    const auto temporaryPath = path.string() + ".tmp";
    {
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) return false;
        output << "FORGE_STORAGE\n" << kStorageFormatVersion << "\n"
               << forge::User::ENCODED_RECORD_SIZE << " "
               << forge::Message::ENCODED_RECORD_SIZE << " "
               << forge::Recipient::ENCODED_RECORD_SIZE << " "
               << forge::MailboxState::ENCODED_RECORD_SIZE << "\n";
        output.flush();
        if (!output.good()) {
            output.close();
            std::filesystem::remove(temporaryPath, ec);
            return false;
        }
        output.close();
        if (output.fail()) {
            std::filesystem::remove(temporaryPath, ec);
            return false;
        }
    }

    std::filesystem::rename(temporaryPath, path, ec);
    if (!ec) return true;

    // Another process may have initialized the same directory concurrently.
    std::error_code existsError;
    const bool targetExists = std::filesystem::exists(path, existsError);
    std::filesystem::remove(temporaryPath, existsError);
    return targetExists && !existsError && validateStorageFormat(dataDir);
}

} // namespace

namespace forge {

Storage::Storage(const std::string& dataDir) 
    : dataDir_(dataDir), isOpen_(false), nextUserId_(1), nextMessageId_(1), userIdExhausted_(false), messageIdExhausted_(false) {
}

Storage::~Storage() {
    close();
}

bool Storage::open() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (isOpen_) return true;
    try {
        std::filesystem::create_directories(dataDir_);
    } catch (...) {
        return false;
    }
    if (!hasRequiredMailboxStateFile(dataDir_)) return false;
    if (!openFiles()) {
        closeFiles();
        return false;
    }
    // Validate an existing manifest before interpreting record bytes. Legacy
    // directories without a manifest are scanned first and tagged only after
    // every existing record has passed validation.
    if (!validateStorageFormat(dataDir_)) {
        closeFiles();
        return false;
    }
    if (!scanForMaxIds()) {
        closeFiles();
        return false;
    }
    isOpen_ = true;
    if (!migrateMailboxStates() || !initializeStorageFormat(dataDir_)) {
        isOpen_ = false;
        closeFiles();
        return false;
    }
    return true;
}

void Storage::close() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    closeFiles();
    isOpen_ = false;
}

bool Storage::openFiles() {
    const auto openDataFile = [](std::fstream& file, const std::string& path) -> bool {
        file.clear();
        std::error_code ec;
        const bool existed = std::filesystem::exists(path, ec);
        if (ec) return false;

        if (existed) {
            // Never truncate an existing file merely because it could not be opened.
            file.open(path, std::ios::binary | std::ios::in | std::ios::out);
            return file.is_open();
        }

        // Create only a genuinely absent file. app mode avoids truncating a file
        // that may appear between the exists check and the create operation.
        file.open(path, std::ios::binary | std::ios::out | std::ios::app);
        if (!file.is_open()) return false;
        file.close();
        file.clear();
        file.open(path, std::ios::binary | std::ios::in | std::ios::out);
        return file.is_open();
    };

    const std::string usersPath = dataDir_ + "/users.dat";
    const std::string messagesPath = dataDir_ + "/messages.dat";
    const std::string recipientsPath = dataDir_ + "/recipients.dat";
    const std::string mailboxStatesPath = dataDir_ + "/mailbox_states.dat";

    if (!openDataFile(usersFile_, usersPath)) return false;
    if (!openDataFile(messagesFile_, messagesPath)) return false;
    if (!openDataFile(recipientsFile_, recipientsPath)) return false;
    if (!openDataFile(mailboxStatesFile_, mailboxStatesPath)) return false;
    return true;
}

void Storage::closeFiles() {
    if (usersFile_.is_open()) usersFile_.close();
    if (messagesFile_.is_open()) messagesFile_.close();
    if (recipientsFile_.is_open()) recipientsFile_.close();
    if (mailboxStatesFile_.is_open()) mailboxStatesFile_.close();
}

bool Storage::scanForMaxIds() {
    nextUserId_ = 1;
    nextMessageId_ = 1;
    userIdExhausted_ = false;
    messageIdExhausted_ = false;
    // Scan users
    usersFile_.seekg(0, std::ios::end);
    std::streampos fsize = usersFile_.tellg();
    if (fsize == std::streampos(-1)) return false;
    usersFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)User::ENCODED_RECORD_SIZE != 0) {
        // Corrupted/truncated - report error
        return false;
    }
    for (size_t offset = 0; offset < (size_t)fsize; offset += User::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(User::ENCODED_RECORD_SIZE);
        usersFile_.seekg(offset);
        usersFile_.read(reinterpret_cast<char*>(buffer.data()), User::ENCODED_RECORD_SIZE);
        if (usersFile_.gcount() != (std::streamsize)User::ENCODED_RECORD_SIZE) return false;
        User user;
        if (!BinaryCodec::deserializeUser(buffer, 0, user)) return false;
        if (user.id == UINT32_MAX) { userIdExhausted_ = true; nextUserId_ = 0; }
        else if (!userIdExhausted_ && user.id >= nextUserId_) nextUserId_ = user.id + 1;
    }
    // Scan messages
    messagesFile_.seekg(0, std::ios::end);
    fsize = messagesFile_.tellg();
    if (fsize == std::streampos(-1)) return false;
    messagesFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)Message::ENCODED_RECORD_SIZE != 0) {
        return false;
    }
    for (size_t offset = 0; offset < (size_t)fsize; offset += Message::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(Message::ENCODED_RECORD_SIZE);
        messagesFile_.seekg(offset);
        messagesFile_.read(reinterpret_cast<char*>(buffer.data()), Message::ENCODED_RECORD_SIZE);
        if (messagesFile_.gcount() != (std::streamsize)Message::ENCODED_RECORD_SIZE) return false;
        Message msg;
        if (!BinaryCodec::deserializeMessage(buffer, 0, msg)) return false;
        if (msg.id == UINT32_MAX) { messageIdExhausted_ = true; nextMessageId_ = 0; }
        else if (!messageIdExhausted_ && msg.id >= nextMessageId_) nextMessageId_ = msg.id + 1;
    }
    // Recipients - validate but no ID counter needed
    recipientsFile_.seekg(0, std::ios::end);
    fsize = recipientsFile_.tellg();
    if (fsize == std::streampos(-1)) return false;
    recipientsFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)Recipient::ENCODED_RECORD_SIZE != 0) {
        return false;
    }
    for (size_t offset = 0; offset < (size_t)fsize; offset += Recipient::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(Recipient::ENCODED_RECORD_SIZE);
        recipientsFile_.seekg(offset);
        recipientsFile_.read(reinterpret_cast<char*>(buffer.data()), Recipient::ENCODED_RECORD_SIZE);
        if (recipientsFile_.gcount() != (std::streamsize)Recipient::ENCODED_RECORD_SIZE) return false;
        Recipient rec;
        if (!BinaryCodec::deserializeRecipient(buffer, 0, rec)) return false;
    }
    mailboxStatesFile_.seekg(0, std::ios::end);
    fsize = mailboxStatesFile_.tellg();
    if (fsize == std::streampos(-1) ||
        fsize % static_cast<std::streampos>(MailboxState::ENCODED_RECORD_SIZE) != 0) return false;
    mailboxStatesFile_.seekg(0, std::ios::beg);
    for (size_t offset = 0; offset < static_cast<size_t>(fsize);
         offset += MailboxState::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(MailboxState::ENCODED_RECORD_SIZE);
        mailboxStatesFile_.seekg(static_cast<std::streamoff>(offset));
        mailboxStatesFile_.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        if (mailboxStatesFile_.gcount() != static_cast<std::streamsize>(buffer.size())) return false;
        MailboxState state{};
        if (!BinaryCodec::deserializeMailboxState(buffer, 0, state)) return false;
    }
    return true;
}

bool Storage::migrateMailboxStates() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_) return false;
    const auto messages = getAllMessages(true);
    const auto recipients = getAllRecipients(false);
    std::set<std::pair<uint32_t, uint32_t>> existingStates;
    for (const auto& state : getAllMailboxStates())
        existingStates.insert({state.message_id, state.user_id});
    std::unordered_map<uint32_t, std::set<uint32_t>> participants;
    for (const auto& message : messages) {
        if (message.sender_id != 0) participants[message.id].insert(message.sender_id);
    }
    for (const auto& recipient : recipients) {
        if (recipient.message_id != 0 && recipient.user_id != 0 && recipient.is_deleted == 0)
            participants[recipient.message_id].insert(recipient.user_id);
    }

    for (const auto& message : messages) {
        const auto found = participants.find(message.id);
        if (found == participants.end()) continue;
        for (const uint32_t userId : found->second) {
            if (existingStates.find({message.id, userId}) != existingStates.end()) continue;
            MailboxState state{};
            state.message_id = message.id;
            state.user_id = userId;
            state.is_read = message.is_read;
            state.is_starred = message.is_starred;
            state.is_deleted = message.is_deleted;
            if (!createMailboxState(state)) {
                // A concurrent process may have inserted the same row.
                if (!readMailboxState(message.id, userId, state)) return false;
            }
            existingStates.insert({message.id, userId});
        }
    }
    return true;
}

uint32_t Storage::getNextUserId() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (userIdExhausted_ || nextUserId_ == 0) return 0;
    const uint32_t id = nextUserId_;
    if (nextUserId_ == UINT32_MAX) { userIdExhausted_ = true; nextUserId_ = 0; }
    else ++nextUserId_;
    return id;
}

uint32_t Storage::getNextMessageId() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (messageIdExhausted_ || nextMessageId_ == 0) return 0;
    const uint32_t id = nextMessageId_;
    if (nextMessageId_ == UINT32_MAX) { messageIdExhausted_ = true; nextMessageId_ = 0; }
    else ++nextMessageId_;
    return id;
}

void Storage::setNextUserId(uint32_t id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!userIdExhausted_ && id != 0 && id > nextUserId_) nextUserId_ = id;
}

void Storage::setNextMessageId(uint32_t id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!messageIdExhausted_ && id != 0 && id > nextMessageId_) nextMessageId_ = id;
}

// User operations
bool Storage::createUser(const User& user) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_ || user.id == 0) return false;
    std::vector<uint8_t> data = BinaryCodec::serializeUser(user);
    usersFile_.seekp(0, std::ios::end);
    usersFile_.write(reinterpret_cast<const char*>(data.data()), data.size());
    usersFile_.flush();
    return usersFile_.good();
}

bool Storage::readUser(uint32_t id, User& user) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_) return false;
    usersFile_.seekg(0, std::ios::end);
    std::streampos fsize = usersFile_.tellg();
    usersFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)User::ENCODED_RECORD_SIZE != 0) return false;
    for (size_t offset = 0; offset < (size_t)fsize; offset += User::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(User::ENCODED_RECORD_SIZE);
        usersFile_.seekg(offset);
        usersFile_.read(reinterpret_cast<char*>(buffer.data()), User::ENCODED_RECORD_SIZE);
        if (usersFile_.gcount() != (std::streamsize)User::ENCODED_RECORD_SIZE) return false;
        User tempUser;
        if (BinaryCodec::deserializeUser(buffer, 0, tempUser)) {
            if (tempUser.id == id && tempUser.is_deleted == 0) {
                user = tempUser;
                return true;
            }
        }
    }
    return false;
}

bool Storage::updateUser(const User& user) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_) return false;
    usersFile_.seekg(0, std::ios::end);
    std::streampos fsize = usersFile_.tellg();
    usersFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)User::ENCODED_RECORD_SIZE != 0) return false;
    for (size_t offset = 0; offset < (size_t)fsize; offset += User::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(User::ENCODED_RECORD_SIZE);
        usersFile_.seekg(offset);
        usersFile_.read(reinterpret_cast<char*>(buffer.data()), User::ENCODED_RECORD_SIZE);
        if (usersFile_.gcount() != (std::streamsize)User::ENCODED_RECORD_SIZE) return false;
        User tempUser;
        if (BinaryCodec::deserializeUser(buffer, 0, tempUser)) {
            if (tempUser.id == user.id) {
                std::vector<uint8_t> data = BinaryCodec::serializeUser(user);
                usersFile_.seekp(offset);
                usersFile_.write(reinterpret_cast<const char*>(data.data()), data.size());
                usersFile_.flush();
                return usersFile_.good();
            }
        }
    }
    return false;
}

bool Storage::deleteUser(uint32_t id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    User user;
    if (!readUser(id, user)) {
        // Try to find even if deleted? For normal delete, look up by scanning all
    }
    // Scan all to find
    usersFile_.seekg(0, std::ios::end);
    std::streampos fsize = usersFile_.tellg();
    usersFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)User::ENCODED_RECORD_SIZE != 0) return false;
    for (size_t offset = 0; offset < (size_t)fsize; offset += User::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(User::ENCODED_RECORD_SIZE);
        usersFile_.seekg(offset);
        usersFile_.read(reinterpret_cast<char*>(buffer.data()), User::ENCODED_RECORD_SIZE);
        if (usersFile_.gcount() != (std::streamsize)User::ENCODED_RECORD_SIZE) return false;
        User tempUser;
        if (BinaryCodec::deserializeUser(buffer, 0, tempUser)) {
            if (tempUser.id == id && tempUser.is_deleted == 0) {
                tempUser.is_deleted = 1;
                std::vector<uint8_t> data = BinaryCodec::serializeUser(tempUser);
                usersFile_.seekp(offset);
                usersFile_.write(reinterpret_cast<const char*>(data.data()), data.size());
                usersFile_.flush();
                return usersFile_.good();
            }
        }
    }
    return false;
}

std::vector<User> Storage::getAllUsers(bool includeDeleted) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<User> users;
    if (!isOpen_) return users;
    usersFile_.seekg(0, std::ios::end);
    std::streampos fsize = usersFile_.tellg();
    usersFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)User::ENCODED_RECORD_SIZE != 0) return users;
    for (size_t offset = 0; offset < (size_t)fsize; offset += User::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(User::ENCODED_RECORD_SIZE);
        usersFile_.seekg(offset);
        usersFile_.read(reinterpret_cast<char*>(buffer.data()), User::ENCODED_RECORD_SIZE);
        if (usersFile_.gcount() != (std::streamsize)User::ENCODED_RECORD_SIZE) break;
        User user;
        if (BinaryCodec::deserializeUser(buffer, 0, user)) {
            if (includeDeleted || user.is_deleted == 0) {
                users.push_back(user);
            }
        }
    }
    return users;
}

// Message operations
bool Storage::createMessage(const Message& message) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_ || message.id == 0) return false;
    std::vector<uint8_t> data = BinaryCodec::serializeMessage(message);
    messagesFile_.seekp(0, std::ios::end);
    messagesFile_.write(reinterpret_cast<const char*>(data.data()), data.size());
    messagesFile_.flush();
    return messagesFile_.good();
}

bool Storage::readMessage(uint32_t id, Message& message) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_) return false;
    messagesFile_.seekg(0, std::ios::end);
    std::streampos fsize = messagesFile_.tellg();
    messagesFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)Message::ENCODED_RECORD_SIZE != 0) return false;
    for (size_t offset = 0; offset < (size_t)fsize; offset += Message::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(Message::ENCODED_RECORD_SIZE);
        messagesFile_.seekg(offset);
        messagesFile_.read(reinterpret_cast<char*>(buffer.data()), Message::ENCODED_RECORD_SIZE);
        if (messagesFile_.gcount() != (std::streamsize)Message::ENCODED_RECORD_SIZE) return false;
        Message tempMsg;
        if (BinaryCodec::deserializeMessage(buffer, 0, tempMsg)) {
            if (tempMsg.id == id) {
                message = tempMsg;
                return true;
            }
        }
    }
    return false;
}

bool Storage::updateMessage(const Message& message) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_) return false;
    messagesFile_.seekg(0, std::ios::end);
    std::streampos fsize = messagesFile_.tellg();
    messagesFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)Message::ENCODED_RECORD_SIZE != 0) return false;
    for (size_t offset = 0; offset < (size_t)fsize; offset += Message::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(Message::ENCODED_RECORD_SIZE);
        messagesFile_.seekg(offset);
        messagesFile_.read(reinterpret_cast<char*>(buffer.data()), Message::ENCODED_RECORD_SIZE);
        if (messagesFile_.gcount() != (std::streamsize)Message::ENCODED_RECORD_SIZE) return false;
        Message tempMsg;
        if (BinaryCodec::deserializeMessage(buffer, 0, tempMsg)) {
            if (tempMsg.id == message.id) {
                std::vector<uint8_t> data = BinaryCodec::serializeMessage(message);
                messagesFile_.seekp(offset);
                messagesFile_.write(reinterpret_cast<const char*>(data.data()), data.size());
                messagesFile_.flush();
                return messagesFile_.good();
            }
        }
    }
    return false;
}

bool Storage::deleteMessage(uint32_t id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    messagesFile_.seekg(0, std::ios::end);
    std::streampos fsize = messagesFile_.tellg();
    messagesFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)Message::ENCODED_RECORD_SIZE != 0) return false;
    for (size_t offset = 0; offset < (size_t)fsize; offset += Message::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(Message::ENCODED_RECORD_SIZE);
        messagesFile_.seekg(offset);
        messagesFile_.read(reinterpret_cast<char*>(buffer.data()), Message::ENCODED_RECORD_SIZE);
        if (messagesFile_.gcount() != (std::streamsize)Message::ENCODED_RECORD_SIZE) return false;
        Message tempMsg;
        if (BinaryCodec::deserializeMessage(buffer, 0, tempMsg)) {
            if (tempMsg.id == id && tempMsg.is_deleted == 0) {
                tempMsg.is_deleted = 1;
                std::vector<uint8_t> data = BinaryCodec::serializeMessage(tempMsg);
                messagesFile_.seekp(offset);
                messagesFile_.write(reinterpret_cast<const char*>(data.data()), data.size());
                messagesFile_.flush();
                return messagesFile_.good();
            }
        }
    }
    return false;
}

std::vector<Message> Storage::getAllMessages(bool includeDeleted) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<Message> messages;
    if (!isOpen_) return messages;
    messagesFile_.seekg(0, std::ios::end);
    std::streampos fsize = messagesFile_.tellg();
    messagesFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)Message::ENCODED_RECORD_SIZE != 0) return messages;
    for (size_t offset = 0; offset < (size_t)fsize; offset += Message::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(Message::ENCODED_RECORD_SIZE);
        messagesFile_.seekg(offset);
        messagesFile_.read(reinterpret_cast<char*>(buffer.data()), Message::ENCODED_RECORD_SIZE);
        if (messagesFile_.gcount() != (std::streamsize)Message::ENCODED_RECORD_SIZE) break;
        Message msg;
        if (BinaryCodec::deserializeMessage(buffer, 0, msg)) {
            if (includeDeleted || msg.is_deleted == 0) {
                messages.push_back(msg);
            }
        }
    }
    return messages;
}

// Recipient operations
bool Storage::createRecipient(const Recipient& recipient) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_ || recipient.message_id == 0 || recipient.user_id == 0) return false;
    std::vector<uint8_t> data = BinaryCodec::serializeRecipient(recipient);
    recipientsFile_.seekp(0, std::ios::end);
    recipientsFile_.write(reinterpret_cast<const char*>(data.data()), data.size());
    recipientsFile_.flush();
    return recipientsFile_.good();
}

bool Storage::readRecipientsByMessage(uint32_t messageId, std::vector<Recipient>& recipients, bool includeDeleted) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_) return false;
    recipients.clear();
    recipientsFile_.seekg(0, std::ios::end);
    std::streampos fsize = recipientsFile_.tellg();
    recipientsFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)Recipient::ENCODED_RECORD_SIZE != 0) return false;
    for (size_t offset = 0; offset < (size_t)fsize; offset += Recipient::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(Recipient::ENCODED_RECORD_SIZE);
        recipientsFile_.seekg(offset);
        recipientsFile_.read(reinterpret_cast<char*>(buffer.data()), Recipient::ENCODED_RECORD_SIZE);
        if (recipientsFile_.gcount() != (std::streamsize)Recipient::ENCODED_RECORD_SIZE) break;
        Recipient rec;
        if (BinaryCodec::deserializeRecipient(buffer, 0, rec)) {
            if (rec.message_id == messageId && (includeDeleted || rec.is_deleted == 0)) {
                recipients.push_back(rec);
            }
        }
    }
    return true;
}

bool Storage::deleteRecipientsByMessage(uint32_t messageId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_) return false;
    recipientsFile_.seekg(0, std::ios::end);
    std::streampos fsize = recipientsFile_.tellg();
    recipientsFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)Recipient::ENCODED_RECORD_SIZE != 0) return false;
    bool ok = true;
    for (size_t offset = 0; offset < (size_t)fsize; offset += Recipient::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(Recipient::ENCODED_RECORD_SIZE);
        recipientsFile_.seekg(offset);
        recipientsFile_.read(reinterpret_cast<char*>(buffer.data()), Recipient::ENCODED_RECORD_SIZE);
        if (recipientsFile_.gcount() != (std::streamsize)Recipient::ENCODED_RECORD_SIZE) break;
        Recipient rec;
        if (BinaryCodec::deserializeRecipient(buffer, 0, rec)) {
            if (rec.message_id == messageId && rec.is_deleted == 0) {
                rec.is_deleted = 1;
                std::vector<uint8_t> data = BinaryCodec::serializeRecipient(rec);
                recipientsFile_.seekp(offset);
                recipientsFile_.write(reinterpret_cast<const char*>(data.data()), data.size());
                recipientsFile_.flush();
                if (!recipientsFile_.good()) ok = false;
            }
        }
    }
    return ok;
}

std::vector<Recipient> Storage::getAllRecipients(bool includeDeleted) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<Recipient> recipients;
    if (!isOpen_) return recipients;
    recipientsFile_.seekg(0, std::ios::end);
    std::streampos fsize = recipientsFile_.tellg();
    recipientsFile_.seekg(0, std::ios::beg);
    if (fsize % (std::streampos)Recipient::ENCODED_RECORD_SIZE != 0) return recipients;
    for (size_t offset = 0; offset < (size_t)fsize; offset += Recipient::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(Recipient::ENCODED_RECORD_SIZE);
        recipientsFile_.seekg(offset);
        recipientsFile_.read(reinterpret_cast<char*>(buffer.data()), Recipient::ENCODED_RECORD_SIZE);
        if (recipientsFile_.gcount() != (std::streamsize)Recipient::ENCODED_RECORD_SIZE) break;
        Recipient rec;
        if (BinaryCodec::deserializeRecipient(buffer, 0, rec)) {
            if (includeDeleted || rec.is_deleted == 0) {
                recipients.push_back(rec);
            }
        }
    }
    return recipients;
}


bool Storage::createMailboxState(const MailboxState& state) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_ || state.message_id == 0 || state.user_id == 0) return false;
    MailboxState existing{};
    if (readMailboxState(state.message_id, state.user_id, existing)) return false;
    const auto data = BinaryCodec::serializeMailboxState(state);
    mailboxStatesFile_.clear();
    mailboxStatesFile_.seekp(0, std::ios::end);
    if (!mailboxStatesFile_.good()) return false;
    mailboxStatesFile_.write(reinterpret_cast<const char*>(data.data()),
                             static_cast<std::streamsize>(data.size()));
    mailboxStatesFile_.flush();
    return mailboxStatesFile_.good();
}

bool Storage::readMailboxState(uint32_t messageId, uint32_t userId, MailboxState& state) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_ || messageId == 0 || userId == 0) return false;
    mailboxStatesFile_.clear();
    mailboxStatesFile_.seekg(0, std::ios::end);
    const std::streampos fileSize = mailboxStatesFile_.tellg();
    if (fileSize == std::streampos(-1) ||
        fileSize % static_cast<std::streampos>(MailboxState::ENCODED_RECORD_SIZE) != 0) return false;
    mailboxStatesFile_.seekg(0, std::ios::beg);
    for (size_t offset = 0; offset < static_cast<size_t>(fileSize);
         offset += MailboxState::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(MailboxState::ENCODED_RECORD_SIZE);
        mailboxStatesFile_.seekg(static_cast<std::streamoff>(offset));
        mailboxStatesFile_.read(reinterpret_cast<char*>(buffer.data()),
                                static_cast<std::streamsize>(buffer.size()));
        if (mailboxStatesFile_.gcount() != static_cast<std::streamsize>(buffer.size())) return false;
        MailboxState candidate{};
        if (!BinaryCodec::deserializeMailboxState(buffer, 0, candidate)) return false;
        if (candidate.message_id == messageId && candidate.user_id == userId) {
            state = candidate;
            return true;
        }
    }
    return false;
}

bool Storage::updateMailboxState(const MailboxState& state) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_ || state.message_id == 0 || state.user_id == 0) return false;
    const auto data = BinaryCodec::serializeMailboxState(state);
    mailboxStatesFile_.clear();
    mailboxStatesFile_.seekg(0, std::ios::end);
    const std::streampos fileSize = mailboxStatesFile_.tellg();
    if (fileSize == std::streampos(-1) ||
        fileSize % static_cast<std::streampos>(MailboxState::ENCODED_RECORD_SIZE) != 0) return false;
    mailboxStatesFile_.seekg(0, std::ios::beg);
    for (size_t offset = 0; offset < static_cast<size_t>(fileSize);
         offset += MailboxState::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(MailboxState::ENCODED_RECORD_SIZE);
        mailboxStatesFile_.seekg(static_cast<std::streamoff>(offset));
        mailboxStatesFile_.read(reinterpret_cast<char*>(buffer.data()),
                                static_cast<std::streamsize>(buffer.size()));
        if (mailboxStatesFile_.gcount() != static_cast<std::streamsize>(buffer.size())) return false;
        MailboxState candidate{};
        if (!BinaryCodec::deserializeMailboxState(buffer, 0, candidate)) return false;
        if (candidate.message_id == state.message_id && candidate.user_id == state.user_id) {
            mailboxStatesFile_.clear();
            mailboxStatesFile_.seekp(static_cast<std::streamoff>(offset));
            mailboxStatesFile_.write(reinterpret_cast<const char*>(data.data()),
                                     static_cast<std::streamsize>(data.size()));
            mailboxStatesFile_.flush();
            return mailboxStatesFile_.good();
        }
    }
    return false;
}

std::vector<MailboxState> Storage::getAllMailboxStates() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<MailboxState> states;
    if (!isOpen_) return states;
    mailboxStatesFile_.clear();
    mailboxStatesFile_.seekg(0, std::ios::end);
    const std::streampos fileSize = mailboxStatesFile_.tellg();
    if (fileSize == std::streampos(-1) ||
        fileSize % static_cast<std::streampos>(MailboxState::ENCODED_RECORD_SIZE) != 0) return states;
    mailboxStatesFile_.seekg(0, std::ios::beg);
    for (size_t offset = 0; offset < static_cast<size_t>(fileSize);
         offset += MailboxState::ENCODED_RECORD_SIZE) {
        std::vector<uint8_t> buffer(MailboxState::ENCODED_RECORD_SIZE);
        mailboxStatesFile_.seekg(static_cast<std::streamoff>(offset));
        mailboxStatesFile_.read(reinterpret_cast<char*>(buffer.data()),
                                static_cast<std::streamsize>(buffer.size()));
        if (mailboxStatesFile_.gcount() != static_cast<std::streamsize>(buffer.size())) break;
        MailboxState state{};
        if (BinaryCodec::deserializeMailboxState(buffer, 0, state)) states.push_back(state);
    }
    return states;
}

bool Storage::deleteMailboxStatesByMessage(uint32_t messageId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_ || messageId == 0) return false;
    bool found = false;
    for (auto state : getAllMailboxStates()) {
        if (state.message_id != messageId || state.is_deleted != 0) continue;
        state.is_deleted = 1;
        if (!updateMailboxState(state)) return false;
        found = true;
    }
    return found;
}


bool Storage::removeMailboxState(uint32_t messageId, uint32_t userId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!isOpen_ || messageId == 0 || userId == 0) return false;

    const auto sourcePath = std::filesystem::path(dataDir_) / "mailbox_states.dat";
    const auto temporaryPath = std::filesystem::path(dataDir_) / "mailbox_states.dat.tmp";
    std::ifstream input(sourcePath, std::ios::binary);
    if (!input.is_open()) return false;
    std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) return false;

    bool removed = false;
    std::vector<uint8_t> buffer(MailboxState::ENCODED_RECORD_SIZE);
    while (input.read(reinterpret_cast<char*>(buffer.data()),
                      static_cast<std::streamsize>(buffer.size()))) {
        MailboxState state{};
        if (!BinaryCodec::deserializeMailboxState(buffer, 0, state)) {
            output.close();
            input.close();
            std::error_code cleanupError;
            std::filesystem::remove(temporaryPath, cleanupError);
            return false;
        }
        if (state.message_id == messageId && state.user_id == userId) {
            removed = true;
            continue;
        }
        output.write(reinterpret_cast<const char*>(buffer.data()),
                     static_cast<std::streamsize>(buffer.size()));
        if (!output.good()) break;
    }
    const bool inputComplete = input.eof() && input.gcount() == 0;
    output.flush();
    const bool outputComplete = output.good();
    input.close();
    output.close();
    if (!inputComplete || !outputComplete || !removed) {
        std::error_code cleanupError;
        std::filesystem::remove(temporaryPath, cleanupError);
        return false;
    }

    mailboxStatesFile_.close();
    std::error_code renameError;
    std::filesystem::rename(temporaryPath, sourcePath, renameError);
    if (renameError) {
        mailboxStatesFile_.clear();
        mailboxStatesFile_.open(sourcePath, std::ios::binary | std::ios::in | std::ios::out);
        std::error_code cleanupError;
        std::filesystem::remove(temporaryPath, cleanupError);
        return false;
    }
    mailboxStatesFile_.clear();
    mailboxStatesFile_.open(sourcePath, std::ios::binary | std::ios::in | std::ios::out);
    return mailboxStatesFile_.is_open();
}

} // namespace forge
