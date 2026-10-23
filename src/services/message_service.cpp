#include "services/message_service.h"
#include <cstring>
#include <ctime>
#include <iostream>

namespace forge {

MessageService::MessageService(std::shared_ptr<Storage> storage, std::shared_ptr<IndexManager> indexManager)
    : storage_(storage), indexManager_(indexManager) {
}

bool MessageService::createMessage(uint32_t senderId, const std::string& subject, const std::string& body,
                                  const std::vector<std::pair<uint32_t, RecipientType>>& recipients,
                                  Message& outMessage) {
    if (!storage_) return false;
    
    // Create message
    Message message;
    message.id = storage_->getNextMessageId();
    message.sender_id = senderId;
    strncpy(message.subject, subject.c_str(), 127);
    message.subject[127] = '\0';
    strncpy(message.body, body.c_str(), 511);
    message.body[511] = '\0';
    message.created_at = time(nullptr);
    message.is_read = 0;
    message.is_starred = 0;
    message.is_deleted = 0;
    
    // Try to create message first
    if (!storage_->createMessage(message)) {
        return false;
    }
    
    // Create recipient records
    bool allRecipientsCreated = true;
    std::vector<Recipient> createdRecipients;
    
    for (const auto& [recipientId, type] : recipients) {
        Recipient recipient;
        recipient.message_id = message.id;
        recipient.user_id = recipientId;
        std::string typeStr = recipientTypeToString(type);
        strncpy(recipient.type, typeStr.c_str(), 3);
        recipient.type[3] = '\0';
        
        if (storage_->createRecipient(recipient)) {
            createdRecipients.push_back(recipient);
            if (indexManager_) {
                indexManager_->onRecipientCreated(recipient);
            }
        } else {
            allRecipientsCreated = false;
            break;
        }
    }
    
    if (!allRecipientsCreated) {
        // Rollback - delete the message and created recipients
        storage_->deleteMessage(message.id);
        // For recipients, we don't have easy delete by message, but mark message deleted
        // This is a simplified rollback
        return false;
    }
    
    if (indexManager_) {
        indexManager_->onMessageCreated(message);
    }
    
    outMessage = message;
    return true;
}

bool MessageService::getMessageById(uint32_t id, Message& message, bool includeDeleted) {
    if (!storage_) return false;
    if (storage_->readMessage(id, message)) {
        if (includeDeleted || message.is_deleted == 0) {
            return true;
        }
    }
    return false;
}

std::vector<Message> MessageService::getMessagesForUser(uint32_t userId, bool includeDeleted) {
    std::vector<Message> result;
    if (!storage_ || !indexManager_) return result;
    
    // Find all message IDs where user is a recipient
    std::vector<uint32_t> messageIds;
    // We need to search through all messages and check recipients
    // Or better, use index - but index gives us userIds for a messageId
    // We need reverse lookup; our current index is messageId -> userIds
    // For simplicity, scan all recipients
    auto allRecipients = storage_->getAllRecipients();
    std::vector<uint32_t> relevantMessageIds;
    for (const auto& rec : allRecipients) {
        if (rec.user_id == userId) {
            // Check if already added
            bool exists = false;
            for (auto mid : relevantMessageIds) {
                if (mid == rec.message_id) { exists = true; break; }
            }
            if (!exists) relevantMessageIds.push_back(rec.message_id);
        }
    }
    
    for (uint32_t msgId : relevantMessageIds) {
        Message msg;
        if (getMessageById(msgId, msg, includeDeleted)) {
            // Also exclude if it's deleted and not including deleted
            if (includeDeleted || msg.is_deleted == 0) {
                result.push_back(msg);
            }
        }
    }
    
    return result;
}

bool MessageService::deleteMessage(uint32_t id) {
    if (!storage_) return false;
    return storage_->deleteMessage(id);
}

bool MessageService::updateMessage(const Message& message) {
    if (!storage_) return false;
    return storage_->updateMessage(message);
}

} // namespace forge
