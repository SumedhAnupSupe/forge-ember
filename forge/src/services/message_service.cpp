#include "services/message_service.h"
#include <cstring>
#include <ctime>
#include <iostream>
#include <set>

namespace forge {

MessageService::MessageService(std::shared_ptr<Storage> storage, std::shared_ptr<IndexManager> indexManager)
    : storage_(storage), indexManager_(indexManager) {
}

bool MessageService::createMessage(uint32_t senderId, const std::string& subject, const std::string& body,
                                  const std::vector<std::pair<uint32_t, RecipientType>>& recipients,
                                  Message& outMessage) {
    if (!storage_) return false;
    if (subject.length() > 127 || body.length() > 511) return false;
    if (recipients.empty() || recipients.size() > 100) return false;

    std::set<uint32_t> recipientIds;
    for (const auto& recipient : recipients) {
        if (recipient.first == 0 || !recipientIds.insert(recipient.first).second) return false;
        if (recipient.second != RecipientType::TO &&
            recipient.second != RecipientType::CC &&
            recipient.second != RecipientType::BCC) return false;
    }

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
    if (!storage_) return result;

    // Include sent messages as well as messages addressed to this user.
    // Use IDs to avoid returning a message twice when the sender also appears
    // in its recipient list.
    std::set<uint32_t> includedIds;
    const auto allMessages = storage_->getAllMessages(includeDeleted);
    for (const auto& message : allMessages) {
        if (message.sender_id == userId) {
            result.push_back(message);
            includedIds.insert(message.id);
        }
    }

    const auto allRecipients = storage_->getAllRecipients(includeDeleted);
    for (const auto& recipient : allRecipients) {
        if (recipient.user_id != userId || (!includeDeleted && recipient.is_deleted != 0)) continue;
        if (includedIds.find(recipient.message_id) != includedIds.end()) continue;

        Message message;
        if (getMessageById(recipient.message_id, message, includeDeleted)) {
            result.push_back(message);
            includedIds.insert(message.id);
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
