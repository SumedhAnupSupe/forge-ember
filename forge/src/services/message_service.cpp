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
    if (subject.length() > 127 || body.length() > 511) return false;
    if (recipients.empty()) return false;
    
    User sender;
    if (!storage_->readUser(senderId, sender) || sender.is_deleted) {
        return false;
    }
    for (const auto& [recipientId, type] : recipients) {
        User recUser;
        if (!storage_->readUser(recipientId, recUser) || recUser.is_deleted) {
            return false;
        }
    }
    
    // Create message
    Message message;
    memset(&message, 0, sizeof(message));
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
    
    if (!storage_->createMessage(message)) {
        return false;
    }
    
    std::vector<Recipient> createdRecipients;
    bool allRecipientsCreated = true;
    for (const auto& [recipientId, type] : recipients) {
        Recipient recipient;
        memset(&recipient, 0, sizeof(recipient));
        recipient.message_id = message.id;
        recipient.user_id = recipientId;
        std::string typeStr = recipientTypeToString(type);
        strncpy(recipient.type, typeStr.c_str(), 3);
        recipient.type[3] = '\0';
        recipient.is_deleted = 0;
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
        storage_->deleteMessage(message.id);
        for (const auto& cr : createdRecipients) {
            storage_->deleteRecipientsByMessage(cr.message_id);
        }
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
    
    std::vector<Recipient> allRecipients = storage_->getAllRecipients(includeDeleted);
    std::vector<uint32_t> relevantMessageIds;
    for (const auto& rec : allRecipients) {
        if (rec.user_id == userId && (includeDeleted || rec.is_deleted == 0)) {
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
            if (includeDeleted || msg.is_deleted == 0) {
                result.push_back(msg);
            }
        }
    }
    
    return result;
}

bool MessageService::deleteMessage(uint32_t id) {
    if (!storage_) return false;
    bool res = storage_->deleteMessage(id);
    if (res) {
        storage_->deleteRecipientsByMessage(id);
    }
    return res;
}

bool MessageService::updateMessage(const Message& message) {
    if (!storage_) return false;
    return storage_->updateMessage(message);
}

} // namespace forge
