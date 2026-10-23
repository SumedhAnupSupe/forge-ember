#include "index/indexes.h"
#include <iostream>

namespace forge {

IndexManager::IndexManager(std::shared_ptr<Storage> storage) 
    : storage_(storage) {
    usersEmailIndex_ = std::make_unique<HashTable>(1009);
    messagesIdIndex_ = std::make_unique<HashTable>(1009);
    recipientsMessageIndex_ = std::make_unique<HashTable>(1009);
}

void IndexManager::buildIndexes() {
    if (!storage_) return;
    
    // Clear existing indexes
    usersEmailIndex_->clear();
    messagesIdIndex_->clear();
    recipientsMessageIndex_->clear();
    
    // Index all users
    auto users = storage_->getAllUsers();
    for (const auto& user : users) {
        usersEmailIndex_->insert(std::string(user.email), user.id);
    }
    
    // Index all messages
    auto messages = storage_->getAllMessages(true); // include deleted for building
    for (const auto& msg : messages) {
        messagesIdIndex_->insert(std::to_string(msg.id), msg.id);
    }
    
    // Index all recipients
    auto recipients = storage_->getAllRecipients();
    for (const auto& rec : recipients) {
        std::string key = std::to_string(rec.message_id);
        recipientsMessageIndex_->insert(key, rec.user_id);
    }
}

void IndexManager::indexUserEmail(const std::string& email, uint32_t userId) {
    usersEmailIndex_->insert(email, userId);
}

bool IndexManager::findUserByEmail(const std::string& email, uint32_t& userId) {
    std::vector<uint32_t> values;
    if (usersEmailIndex_->find(email, values) && !values.empty()) {
        userId = values[0];
        return true;
    }
    return false;
}

void IndexManager::indexMessageId(uint32_t messageId) {
    messagesIdIndex_->insert(std::to_string(messageId), messageId);
}

bool IndexManager::hasMessageId(uint32_t messageId) {
    std::vector<uint32_t> values;
    return messagesIdIndex_->find(std::to_string(messageId), values) && !values.empty();
}

void IndexManager::indexRecipientByMessage(uint32_t messageId, uint32_t userId) {
    std::string key = std::to_string(messageId);
    recipientsMessageIndex_->insert(key, userId);
}

bool IndexManager::findRecipientsByMessage(uint32_t messageId, std::vector<uint32_t>& userIds) {
    std::string key = std::to_string(messageId);
    return recipientsMessageIndex_->find(key, userIds);
}

void IndexManager::onUserCreated(const User& user) {
    usersEmailIndex_->insert(std::string(user.email), user.id);
}

void IndexManager::onMessageCreated(const Message& message) {
    messagesIdIndex_->insert(std::to_string(message.id), message.id);
}

void IndexManager::onRecipientCreated(const Recipient& recipient) {
    std::string key = std::to_string(recipient.message_id);
    recipientsMessageIndex_->insert(key, recipient.user_id);
}

void IndexManager::onMessageDeleted(uint32_t messageId) {
    // Index entry remains but we handle deletion in queries
}

} // namespace forge
