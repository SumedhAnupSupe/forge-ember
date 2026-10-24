#ifndef STORAGE_H
#define STORAGE_H

#include "models/records.h"
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <mutex>
#include <filesystem>

namespace forge {

class Storage {
public:
    Storage(const std::string& dataDir = "data");
    ~Storage();
    
    bool open();
    void close();
    
    // User operations
    bool createUser(const User& user);
    bool readUser(uint32_t id, User& user);
    bool updateUser(const User& user);
    bool deleteUser(uint32_t id);
    std::vector<User> getAllUsers(bool includeDeleted = false);
    
    // Message operations
    bool createMessage(const Message& message);
    bool readMessage(uint32_t id, Message& message);
    bool updateMessage(const Message& message);
    bool deleteMessage(uint32_t id);
    std::vector<Message> getAllMessages(bool includeDeleted = false);
    
    // Recipient operations
    bool createRecipient(const Recipient& recipient);
    bool readRecipientsByMessage(uint32_t messageId, std::vector<Recipient>& recipients, bool includeDeleted = false);
    bool deleteRecipientsByMessage(uint32_t messageId);
    std::vector<Recipient> getAllRecipients(bool includeDeleted = false);
    
    // ID management
    uint32_t getNextUserId();
    uint32_t getNextMessageId();
    void setNextUserId(uint32_t id);
    void setNextMessageId(uint32_t id);
    
private:
    std::string dataDir_;
    std::fstream usersFile_;
    std::fstream messagesFile_;
    std::fstream recipientsFile_;
    bool isOpen_;
    // The stream-backed storage is shared by HTTP worker threads.
    mutable std::recursive_mutex mutex_;
    
    uint32_t nextUserId_;
    uint32_t nextMessageId_;
    
    bool openFiles();
    void closeFiles();
    bool scanForMaxIds();
};

} // namespace forge

#endif // STORAGE_H
