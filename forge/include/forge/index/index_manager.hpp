#ifndef INDEXES_H
#define INDEXES_H

#include "index/hash_table.h"
#include "storage/storage.h"
#include <memory>

namespace forge {

class IndexManager {
public:
    IndexManager(std::shared_ptr<Storage> storage);
    
    // Build indexes from storage
    void buildIndexes();
    
    // User email index
    void indexUserEmail(const std::string& email, uint32_t userId);
    bool findUserByEmail(const std::string& email, uint32_t& userId);
    
    // Message id index
    void indexMessageId(uint32_t messageId);
    bool hasMessageId(uint32_t messageId);
    
    // Recipients by message index
    void indexRecipientByMessage(uint32_t messageId, uint32_t userId);
    bool findRecipientsByMessage(uint32_t messageId, std::vector<uint32_t>& userIds);
    
    // Update operations
    void onUserCreated(const User& user);
    void onMessageCreated(const Message& message);
    void onRecipientCreated(const Recipient& recipient);
    void onMessageDeleted(uint32_t messageId);
    
private:
    std::shared_ptr<Storage> storage_;
    std::unique_ptr<HashTable> usersEmailIndex_;
    std::unique_ptr<HashTable> messagesIdIndex_;
    std::unique_ptr<HashTable> recipientsMessageIndex_;
};

} // namespace forge

#endif // INDEXES_H
