#ifndef AUTH_H
#define AUTH_H

#include "models/records.h"
#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <mutex>
#include <vector>

namespace forge {

struct Session {
    std::string token;
    uint32_t userId;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point expiresAt;
};

class AuthManager {
public:
    AuthManager();
    
    // Password hashing with Argon2id
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    
    // Session management
    std::string createSession(uint32_t userId);
    bool validateSession(const std::string& token, uint32_t& userId);
    void invalidateSession(const std::string& token);
    void logout(const std::string& token);

    // Five failed attempts per normalized email + remote address in 15 minutes.
    bool allowLoginAttempt(const std::string& key);
    void recordFailedLogin(const std::string& key);
    void clearFailedLogins(const std::string& key);
    
private:
    std::map<std::string, Session> sessions_;
    std::map<std::string, std::vector<std::chrono::steady_clock::time_point>> failedLoginAttempts_;
    std::mutex mutex_;
    std::string generateToken();
};

} // namespace forge

#endif // AUTH_H
