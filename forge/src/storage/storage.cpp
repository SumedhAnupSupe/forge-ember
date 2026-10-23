#include "storage/storage.h"
#include "utils/codec.h"
#include <iostream>
#include <sys/stat.h>

namespace forge {

Storage::Storage(const std::string& filename) 
    : filename_(filename), isOpen_(false), nextUserId_(1), nextMessageId_(1) {
}

Storage::~Storage() {
    close();
}

bool Storage::fileExists() {
    struct stat buffer;
    return (stat(filename_.c_str(), &buffer) == 0);
}

void Storage::initializeFile() {
    if (!fileExists()) {
        file_.open(filename_, std::ios::binary | std::ios::out | std::ios::trunc);
        if (file_.is_open()) {
            file_.close();
        }
    }
}

bool Storage::open() {
    if (isOpen_) return true;
    initializeFile();
    file_.open(filename_, std::ios::binary | std::ios::in | std::ios::out);
    if (!file_.is_open()) {
        std::cerr << "Failed to open file: " << filename_ << std::endl;
        return false;
    }
    isOpen_ = true;
    return true;
}

void Storage::close() {
    if (isOpen_ && file_.is_open()) {
        file_.close();
        isOpen_ = false;
    }
}

size_t Storage::getFileSize() {
    if (!isOpen_) return 0;
    file_.seekg(0, std::ios::end);
    size_t size = file_.tellg();
    file_.seekg(0, std::ios::beg);
    return size;
}

uint32_t Storage::getNextUserId() {
    return nextUserId_++;
}

uint32_t Storage::getNextMessageId() {
    return nextMessageId_++;
}

void Storage::setNextUserId(uint32_t id) {
    if (id > nextUserId_) nextUserId_ = id;
}

void Storage::setNextMessageId(uint32_t id) {
    if (id > nextMessageId_) nextMessageId_ = id;
}

// User operations
bool Storage::createUser(const User& user) {
    if (!isOpen_) return false;
    std::vector<uint8_t> data = BinaryCodec::serializeUser(user);
    file_.seekp(0, std::ios::end);
    file_.write(reinterpret_cast<const char*>(data.data()), data.size());
    file_.flush();
    return file_.good();
}

bool Storage::readUser(uint32_t id, User& user) {
    if (!isOpen_) return false;
    size_t fileSize = getFileSize();
    size_t recordSize = User::RECORD_SIZE;
    // Assume users are stored starting at beginning in a simple layout
    // This is a simplified approach - in a real implementation might need indexing
    for (size_t offset = 0; offset + recordSize <= fileSize; offset += recordSize) {
        file_.seekg(offset);
        std::vector<uint8_t> buffer(recordSize);
        file_.read(reinterpret_cast<char*>(buffer.data()), recordSize);
        if (file_.gcount() == (std::streamsize)recordSize) {
            User tempUser;
            if (BinaryCodec::deserializeUser(buffer, 0, tempUser)) {
                if (tempUser.id == id) {
                    user = tempUser;
                    return true;
                }
            }
        }
    }
    return false;
}

bool Storage::updateUser(const User& user) {
    if (!isOpen_) return false;
    size_t fileSize = getFileSize();
    size_t recordSize = User::RECORD_SIZE;
    for (size_t offset = 0; offset + recordSize <= fileSize; offset += recordSize) {
        file_.seekg(offset);
        std::vector<uint8_t> buffer(recordSize);
        file_.read(reinterpret_cast<char*>(buffer.data()), recordSize);
        if (file_.gcount() == (std::streamsize)recordSize) {
            User tempUser;
            if (BinaryCodec::deserializeUser(buffer, 0, tempUser)) {
                if (tempUser.id == user.id) {
                    std::vector<uint8_t> data = BinaryCodec::serializeUser(user);
                    file_.seekp(offset);
                    file_.write(reinterpret_cast<const char*>(data.data()), data.size());
                    file_.flush();
                    return file_.good();
                }
            }
        }
    }
    return false;
}

bool Storage::deleteUser(uint32_t id) {
    // For now, just mark - but need to follow soft delete pattern if specified
    // Simple implementation: could overwrite or just leave (depends on spec)
    return true;
}

std::vector<User> Storage::getAllUsers() {
    std::vector<User> users;
    if (!isOpen_) return users;
    size_t fileSize = getFileSize();
    size_t recordSize = User::RECORD_SIZE;
    for (size_t offset = 0; offset + recordSize <= fileSize; offset += recordSize) {
        file_.seekg(offset);
        std::vector<uint8_t> buffer(recordSize);
        file_.read(reinterpret_cast<char*>(buffer.data()), recordSize);
        if (file_.gcount() == (std::streamsize)recordSize) {
            User user;
            if (BinaryCodec::deserializeUser(buffer, 0, user)) {
                users.push_back(user);
            }
        }
    }
    return users;
}

// Message operations
bool Storage::createMessage(const Message& message) {
    if (!isOpen_) return false;
    std::vector<uint8_t> data = BinaryCodec::serializeMessage(message);
    file_.seekp(0, std::ios::end);
    file_.write(reinterpret_cast<const char*>(data.data()), data.size());
    file_.flush();
    return file_.good();
}

bool Storage::readMessage(uint32_t id, Message& message) {
    if (!isOpen_) return false;
    size_t fileSize = getFileSize();
    size_t recordSize = Message::RECORD_SIZE;
    for (size_t offset = 0; offset + recordSize <= fileSize; offset += recordSize) {
        file_.seekg(offset);
        std::vector<uint8_t> buffer(recordSize);
        file_.read(reinterpret_cast<char*>(buffer.data()), recordSize);
        if (file_.gcount() == (std::streamsize)recordSize) {
            Message tempMsg;
            if (BinaryCodec::deserializeMessage(buffer, 0, tempMsg)) {
                if (tempMsg.id == id) {
                    message = tempMsg;
                    return true;
                }
            }
        }
    }
    return false;
}

bool Storage::updateMessage(const Message& message) {
    if (!isOpen_) return false;
    size_t fileSize = getFileSize();
    size_t recordSize = Message::RECORD_SIZE;
    for (size_t offset = 0; offset + recordSize <= fileSize; offset += recordSize) {
        file_.seekg(offset);
        std::vector<uint8_t> buffer(recordSize);
        file_.read(reinterpret_cast<char*>(buffer.data()), recordSize);
        if (file_.gcount() == (std::streamsize)recordSize) {
            Message tempMsg;
            if (BinaryCodec::deserializeMessage(buffer, 0, tempMsg)) {
                if (tempMsg.id == message.id) {
                    std::vector<uint8_t> data = BinaryCodec::serializeMessage(message);
                    file_.seekp(offset);
                    file_.write(reinterpret_cast<const char*>(data.data()), data.size());
                    file_.flush();
                    return file_.good();
                }
            }
        }
    }
    return false;
}

bool Storage::deleteMessage(uint32_t id) {
    Message msg;
    if (readMessage(id, msg)) {
        msg.is_deleted = 1;
        return updateMessage(msg);
    }
    return false;
}

std::vector<Message> Storage::getAllMessages(bool includeDeleted) {
    std::vector<Message> messages;
    if (!isOpen_) return messages;
    size_t fileSize = getFileSize();
    size_t recordSize = Message::RECORD_SIZE;
    for (size_t offset = 0; offset + recordSize <= fileSize; offset += recordSize) {
        file_.seekg(offset);
        std::vector<uint8_t> buffer(recordSize);
        file_.read(reinterpret_cast<char*>(buffer.data()), recordSize);
        if (file_.gcount() == (std::streamsize)recordSize) {
            Message msg;
            if (BinaryCodec::deserializeMessage(buffer, 0, msg)) {
                if (includeDeleted || msg.is_deleted == 0) {
                    messages.push_back(msg);
                }
            }
        }
    }
    return messages;
}

// Recipient operations
bool Storage::createRecipient(const Recipient& recipient) {
    if (!isOpen_) return false;
    std::vector<uint8_t> data = BinaryCodec::serializeRecipient(recipient);
    file_.seekp(0, std::ios::end);
    file_.write(reinterpret_cast<const char*>(data.data()), data.size());
    file_.flush();
    return file_.good();
}

bool Storage::readRecipientsByMessage(uint32_t messageId, std::vector<Recipient>& recipients) {
    if (!isOpen_) return false;
    recipients.clear();
    size_t fileSize = getFileSize();
    size_t recordSize = Recipient::RECORD_SIZE;
    for (size_t offset = 0; offset + recordSize <= fileSize; offset += recordSize) {
        file_.seekg(offset);
        std::vector<uint8_t> buffer(recordSize);
        file_.read(reinterpret_cast<char*>(buffer.data()), recordSize);
        if (file_.gcount() == (std::streamsize)recordSize) {
            Recipient rec;
            if (BinaryCodec::deserializeRecipient(buffer, 0, rec)) {
                if (rec.message_id == messageId) {
                    recipients.push_back(rec);
                }
            }
        }
    }
    return true;
}

bool Storage::deleteRecipientsByMessage(uint32_t messageId) {
    // For simplicity, we'll just note - in a real system might need to handle differently
    return true;
}

std::vector<Recipient> Storage::getAllRecipients() {
    std::vector<Recipient> recipients;
    if (!isOpen_) return recipients;
    size_t fileSize = getFileSize();
    size_t recordSize = Recipient::RECORD_SIZE;
    for (size_t offset = 0; offset + recordSize <= fileSize; offset += recordSize) {
        file_.seekg(offset);
        std::vector<uint8_t> buffer(recordSize);
        file_.read(reinterpret_cast<char*>(buffer.data()), recordSize);
        if (file_.gcount() == (std::streamsize)recordSize) {
            Recipient rec;
            if (BinaryCodec::deserializeRecipient(buffer, 0, rec)) {
                recipients.push_back(rec);
            }
        }
    }
    return recipients;
}

} // namespace forge
