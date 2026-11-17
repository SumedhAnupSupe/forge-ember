#include "services/message_service.h"
#include <cstring>
#include <ctime>
#include <iostream>
#include <set>
#include <unordered_map>

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
    if (message.id == 0) return false; // ID space exhausted; never persist/reuse zero.
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
        } else {
            allRecipientsCreated = false;
            break;
        }
    }
    
    if (!allRecipientsCreated) {
        const bool messageRolledBack = storage_->deleteMessage(message.id);
        const bool recipientsRolledBack = storage_->deleteRecipientsByMessage(message.id);
        if (!messageRolledBack || !recipientsRolledBack) {
            std::cerr << "Message creation rollback was incomplete for message "
                      << message.id << std::endl;
        }
        return false;
    }

    // Every participant receives an independent mailbox-state row. The sender
    // is included even when they are not also listed as a recipient.
    std::set<uint32_t> mailboxUsers{senderId};
    for (const auto& recipient : recipients) mailboxUsers.insert(recipient.first);
    bool allStatesCreated = true;
    for (const uint32_t userId : mailboxUsers) {
        MailboxState state{};
        state.message_id = message.id;
        state.user_id = userId;
        if (!storage_->createMailboxState(state)) {
            allStatesCreated = false;
            break;
        }
    }
    if (!allStatesCreated) {
        storage_->deleteMailboxStatesByMessage(message.id);
        const bool messageRolledBack = storage_->deleteMessage(message.id);
        const bool recipientsRolledBack = storage_->deleteRecipientsByMessage(message.id);
        if (!messageRolledBack || !recipientsRolledBack) {
            std::cerr << "Message/mailbox-state rollback was incomplete for message "
                      << message.id << std::endl;
        }
        return false;
    }

    // Publish index entries only after every record has been persisted.
    if (indexManager_) {
        indexManager_->onMessageCreated(message);
        for (const auto& recipient : createdRecipients)
            indexManager_->onRecipientCreated(recipient);
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
    std::unordered_map<uint32_t, MailboxState> statesByMessage;
    for (const auto& state : storage_->getAllMailboxStates()) {
        if (state.user_id == userId) statesByMessage[state.message_id] = state;
    }
    const auto applyMailboxState = [&](Message& message) {
        const auto found = statesByMessage.find(message.id);
        if (found == statesByMessage.end()) return false;
        message.is_read = found->second.is_read;
        message.is_starred = found->second.is_starred;
        message.is_deleted = found->second.is_deleted;
        return true;
    };
    const auto allMessages = storage_->getAllMessages(includeDeleted);
    for (auto message : allMessages) {
        if (message.sender_id == userId && applyMailboxState(message)) {
            result.push_back(message);
            includedIds.insert(message.id);
        }
    }

    const auto allRecipients = storage_->getAllRecipients(includeDeleted);
    for (const auto& recipient : allRecipients) {
        if (recipient.user_id != userId || (!includeDeleted && recipient.is_deleted != 0)) continue;
        if (includedIds.find(recipient.message_id) != includedIds.end()) continue;

        Message message;
        if (getMessageById(recipient.message_id, message, includeDeleted) &&
            applyMailboxState(message)) {
            result.push_back(message);
            includedIds.insert(message.id);
        }
    }
    return result;
}

bool MessageService::deleteMessage(uint32_t id) {
    if (!storage_) return false;
    const bool deleted = storage_->deleteMessage(id);
    if (deleted) {
        const bool recipientsDeleted = storage_->deleteRecipientsByMessage(id);
        const bool mailboxStatesDeleted = storage_->deleteMailboxStatesByMessage(id);
        if (!recipientsDeleted || !mailboxStatesDeleted) {
            std::cerr << "Message deletion cleanup was incomplete for message " << id << std::endl;
        }
    }
    return deleted;
}

bool MessageService::updateMessage(const Message& message) {
    if (!storage_) return false;
    return storage_->updateMessage(message);
}

} // namespace forge
