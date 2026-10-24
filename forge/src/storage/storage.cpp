#include "storage/storage.h"
#include "utils/codec.h"
#include <iostream>
#include <sys/stat.h>
#include <cstring>

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
    if (!openFiles()) {
        closeFiles();
        return false;
    }
    if (!scanForMaxIds()) {
        closeFiles();
        return false;
    }
    isOpen_ = true;
    return true;
}

void Storage::close() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    closeFiles();
    isOpen_ = false;
}

bool Storage::openFiles() {
    std::string usersPath = dataDir_ + "/users.dat";
    std::string messagesPath = dataDir_ + "/messages.dat";
    std::string recipientsPath = dataDir_ + "/recipients.dat";
    
    usersFile_.open(usersPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!usersFile_.is_open()) {
        usersFile_.open(usersPath, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!usersFile_.is_open()) return false;
        usersFile_.close();
        usersFile_.open(usersPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!usersFile_.is_open()) return false;
    }
    
    messagesFile_.open(messagesPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!messagesFile_.is_open()) {
        messagesFile_.open(messagesPath, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!messagesFile_.is_open()) return false;
        messagesFile_.close();
        messagesFile_.open(messagesPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!messagesFile_.is_open()) return false;
    }
    
    recipientsFile_.open(recipientsPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!recipientsFile_.is_open()) {
        recipientsFile_.open(recipientsPath, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!recipientsFile_.is_open()) return false;
        recipientsFile_.close();
        recipientsFile_.open(recipientsPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!recipientsFile_.is_open()) return false;
    }
    return true;
}

void Storage::closeFiles() {
    if (usersFile_.is_open()) usersFile_.close();
    if (messagesFile_.is_open()) messagesFile_.close();
    if (recipientsFile_.is_open()) recipientsFile_.close();
}

bool Storage::scanForMaxIds() {
    nextUserId_ = 1;
    nextMessageId_ = 1;
    userIdExhausted_ = false;
    messageIdExhausted_ = false;
    // Scan users
    usersFile_.seekg(0, std::ios::end);
    std::streampos fsize = usersFile_.tellg();
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
            if (tempMsg.id == id && tempMsg.is_deleted == 0) {
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

} // namespace forge
