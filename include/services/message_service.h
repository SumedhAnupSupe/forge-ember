#ifndef MESSAGE_SERVICE_H
#define MESSAGE_SERVICE_H

#include "storage/storage.h"
#include "index/indexes.h"
#include <string>
#include <memory>
#include <vector>

namespace forge {

class MessageService {
public:
    MessageService(std::shared_ptr<Storage> storage, std::shared_ptr<IndexManager> indexManager);
    
    bool createMessage(uint32_t senderId, const std::string& subject, const std::string& body,
                      const std::vector<std::pair<uint32_t, RecipientType>>& recipients,
                      Message& outMessage);
    bool getMessageById(uint32_t id, Message& message, bool includeDeleted = false);
    std::vector<Message> getMessagesForUser(uint32_t userId, bool includeDeleted = false);
    bool deleteMessage(uint32_t id); // soft delete
    bool updateMessage(const Message& message);
    
private:
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<IndexManager> indexManager_;
};

} // namespace forge

#endif // MESSAGE_SERVICE_H
