#include "services/user_service.h"
#include <cstring>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace forge {

UserService::UserService(std::shared_ptr<Storage> storage, std::shared_ptr<IndexManager> indexManager)
    : storage_(storage), indexManager_(indexManager) {
}

std::string normalizeEmail(const std::string& email) {
    std::string res = email;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c){ return std::tolower(c); });
    return res;
}

bool UserService::isEmailUnique(const std::string& email) {
    std::string norm = normalizeEmail(email);
    uint32_t existingId;
    if (indexManager_ && indexManager_->findUserByEmail(norm, existingId)) {
        return false;
    }
    // Also check against storage
    if (storage_) {
        auto users = storage_->getAllUsers(false);
        for (const auto& u : users) {
            std::string uemail = u.email;
            std::transform(uemail.begin(), uemail.end(), uemail.begin(), [](unsigned char c){ return std::tolower(c); });
            if (uemail == norm) return false;
        }
    }
    return true;
}

bool UserService::createUser(const std::string& name, const std::string& email,
                           const std::string& passwordHash, User& outUser) {
    if (!storage_) return false;
    if (name.length() > 63 || email.length() > 127 || passwordHash.length() > 255) return false;
    if (!isEmailUnique(email)) {
        return false;
    }
    
    User user;
    memset(&user, 0, sizeof(user));
    user.id = storage_->getNextUserId();
    if (user.id == 0) return false; // ID space exhausted; never persist/reuse zero.
    strncpy(user.name, name.c_str(), 63);
    user.name[63] = '\0';
    strncpy(user.email, normalizeEmail(email).c_str(), 127);
    user.email[127] = '\0';
    strncpy(user.password_hash, passwordHash.c_str(), 255);
    user.password_hash[255] = '\0';
    user.created_at = time(nullptr);
    user.is_deleted = 0;
    
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
    std::string norm = normalizeEmail(email);
    uint32_t userId;
    if (indexManager_->findUserByEmail(norm, userId)) {
        return storage_->readUser(userId, user);
    }
    // fallback
    auto users = storage_->getAllUsers(false);
    for (const auto& u : users) {
        std::string uemail = u.email;
        std::transform(uemail.begin(), uemail.end(), uemail.begin(), [](unsigned char c){ return std::tolower(c); });
        if (uemail == norm) { user = u; return true; }
    }
    return false;
}

bool UserService::updateUser(const User& user) {
    if (!storage_) return false;
    if (storage_->updateUser(user)) {
        if (indexManager_) {
            indexManager_->onUserCreated(user); // refresh
        }
        return true;
    }
    return false;
}

bool UserService::deleteUser(uint32_t id) {
    if (!storage_) return false;
    return storage_->deleteUser(id);
}

std::vector<User> UserService::getAllUsers() {
    if (!storage_) return std::vector<User>();
    return storage_->getAllUsers(false);
}

} // namespace forge
