#ifndef STORAGE_H
#define STORAGE_H

#include "models/records.h"
#include <string>
#include <vector>
#include <fstream>
#include <memory>

namespace forge {

class Storage {
public:
    Storage(const std::string& filename);
    ~Storage();
    
    bool open();
    void close();
    
    // User operations
    bool createUser(const User& user);
    bool readUser(uint32_t id, User& user);
    bool updateUser(const User& user);
    bool deleteUser(uint32_t id);
    std::vector<User> getAllUsers();
    
    // Message operations
    bool createMessage(const Message& message);
    bool readMessage(uint32_t id, Message& message);
    bool updateMessage(const Message& message);
    bool deleteMessage(uint32_t id); // soft delete
    std::vector<Message> getAllMessages(bool includeDeleted = false);
    
    // Recipient operations
    bool createRecipient(const Recipient& recipient);
    bool readRecipientsByMessage(uint32_t messageId, std::vector<Recipient>& recipients);
    bool deleteRecipientsByMessage(uint32_t messageId);
    std::vector<Recipient> getAllRecipients();
    
    // ID management
    uint32_t getNextUserId();
    uint32_t getNextMessageId();
    void setNextUserId(uint32_t id);
    void setNextMessageId(uint32_t id);
    
private:
    std::string filename_;
    std::fstream file_;
    bool isOpen_;
    
    uint32_t nextUserId_;
    uint32_t nextMessageId_;
    
    // Helper methods
    bool fileExists();
    void initializeFile();
    size_t getFileSize();
};

} // namespace forge

#endif // STORAGE_H
