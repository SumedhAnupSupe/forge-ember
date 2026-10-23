#include "services/user_service.h"
#include <cstring>
#include <ctime>
#include <iostream>

namespace forge {

UserService::UserService(std::shared_ptr<Storage> storage, std::shared_ptr<IndexManager> indexManager)
    : storage_(storage), indexManager_(indexManager) {
}

bool UserService::isEmailUnique(const std::string& email) {
    uint32_t existingId;
    if (indexManager_ && indexManager_->findUserByEmail(email, existingId)) {
        return false;
    }
    return true;
}

bool UserService::createUser(const std::string& name, const std::string& email,
                           const std::string& passwordHash, User& outUser) {
    if (!storage_) return false;
    
    if (!isEmailUnique(email)) {
        return false;
    }
    
    User user;
    user.id = storage_->getNextUserId();
    strncpy(user.name, name.c_str(), 63);
    user.name[63] = '\0';
    strncpy(user.email, email.c_str(), 127);
    user.email[127] = '\0';
    strncpy(user.password_hash, passwordHash.c_str(), 255);
    user.password_hash[255] = '\0';
    user.created_at = time(nullptr);
    
    if (storage_->createUser(user)) {
        if (indexManager_) {
            indexManager_->onUserCreated(user);
        }
        outUser = user;
        return true;
    }
    return false;
}

bool UserService::getUserById(uint32_t id, User& user) {
    if (!storage_) return false;
    return storage_->readUser(id, user);
}

bool UserService::getUserByEmail(const std::string& email, User& user) {
    if (!storage_ || !indexManager_) return false;
    uint32_t userId;
    if (indexManager_->findUserByEmail(email, userId)) {
        return storage_->readUser(userId, user);
    }
    return false;
}

bool UserService::updateUser(const User& user) {
    if (!storage_) return false;
    return storage_->updateUser(user);
}

bool UserService::deleteUser(uint32_t id) {
    if (!storage_) return false;
    return storage_->deleteUser(id);
}

std::vector<User> UserService::getAllUsers() {
    if (!storage_) return std::vector<User>();
    return storage_->getAllUsers();
}

} // namespace forge
