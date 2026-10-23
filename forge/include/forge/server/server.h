#ifndef FORGE_SERVER_H
#define FORGE_SERVER_H

#include <string>
#include <memory>
#include "storage/storage.h"
#include "services/user_service.h"
#include "services/message_service.h"
#include "index/indexes.h"
#include "auth/auth.h"

namespace forge {
namespace server {

class ForgeServer {
public:
    ForgeServer(const std::string& host, int port, const std::string& dataDir);
    bool start();
    void stop();
    
private:
    std::string host_;
    int port_;
    std::string dataDir_;
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<IndexManager> indexManager_;
    std::shared_ptr<UserService> userService_;
    std::shared_ptr<MessageService> messageService_;
    std::shared_ptr<AuthManager> authManager_;
};

} // namespace server
} // namespace forge

#endif
