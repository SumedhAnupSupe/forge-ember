#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include "storage/storage.h"
#include "index/indexes.h"
#include <string>
#include <memory>

namespace forge {

class UserService {
public:
    UserService(std::shared_ptr<Storage> storage, std::shared_ptr<IndexManager> indexManager);
    
    bool createUser(const std::string& name, const std::string& email, 
                   const std::string& passwordHash, User& outUser);
    bool getUserById(uint32_t id, User& user);
    bool getUserByEmail(const std::string& email, User& user);
    bool updateUser(const User& user);
    bool deleteUser(uint32_t id);
    std::vector<User> getAllUsers();
    
    bool isEmailUnique(const std::string& email);
    
private:
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<IndexManager> indexManager_;
    std::mutex createMutex_;
};

} // namespace forge

#endif // USER_SERVICE_H
