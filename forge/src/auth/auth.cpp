#include "auth/auth.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <vector>

#ifdef HAVE_ARGON2
extern "C" {
#ifdef __has_include
#  if __has_include(<argon2.h>)
#    include <argon2.h>
#  endif
#endif
}
#endif

namespace forge {

AuthManager::AuthManager() {
}

std::string AuthManager::generateToken() {
    std::random_device rd;
    // Use cryptographically secure random device
    std::uniform_int_distribution<int> distribution(0, 255);
    
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << std::setfill('0') << std::setw(2) << distribution(rd);
    }
    return ss.str();
}

std::string AuthManager::hashPassword(const std::string& password) {
#ifdef HAVE_ARGON2
#ifdef __has_include
#  if __has_include(<argon2.h>)
    const uint32_t t_cost = 3;
    const uint32_t m_cost = (1 << 16);
    const uint32_t parallelism = 1;
    const uint32_t hashlen = 32;
    
    std::random_device rd;
    std::vector<unsigned char> salt(16);
    for (int i = 0; i < 16; ++i) salt[i] = (unsigned char)rd();
    
    std::vector<unsigned char> hash(hashlen);
    int result = argon2id_hash_raw(t_cost, m_cost, parallelism,
                                   password.c_str(), password.length(),
                                   salt.data(), salt.size(),
                                   hash.data(), hashlen);
    if (result != ARGON2_OK) {
        // Fail closed - do not use insecure fallback
        throw std::runtime_error("Argon2id hashing failed");
    }
    
    std::stringstream ss;
    ss << "$argon2id$v=19$m=" << m_cost << ",t=" << t_cost << ",p=" << parallelism << "$";
    for (auto b : salt) ss << std::hex << std::setfill('0') << std::setw(2) << (int)b;
    ss << "$";
    for (auto b : hash) ss << std::hex << std::setfill('0') << std::setw(2) << (int)b;
    return ss.str();
#  endif
#endif
#endif
    // No insecure fallback allowed per requirements
    throw std::runtime_error("Argon2id not available");
}

bool AuthManager::verifyPassword(const std::string& password, const std::string& hash) {
    if (hash.empty()) return false;
    if (hash.find("$argon2id$") != 0) return false;
#ifdef HAVE_ARGON2
#ifdef __has_include
#  if __has_include(<argon2.h>)
    // Try to use argon2 verification if available
    // For encoded hashes, argon2id_verify can be used
    try {
        int res = argon2id_verify(hash.c_str(), password.c_str(), password.length());
        if (res == ARGON2_OK) return true;
        if (res == ARGON2_VERIFY_MISMATCH) return false;
    } catch (...) {
        // Fall through
    }
    // If verification API not usable as-is, check format is proper argon2id
    // but must not accept blindly without verification
    return false;
#  endif
#endif
#endif
    return false;
}

std::string AuthManager::createSession(uint32_t userId) {
    std::string token = generateToken();
    Session session;
    session.token = token;
    session.userId = userId;
    session.createdAt = std::chrono::system_clock::now();
    session.expiresAt = session.createdAt + std::chrono::hours(24);
    sessions_[token] = session;
    return token;
}

bool AuthManager::validateSession(const std::string& token, uint32_t& userId) {
    auto it = sessions_.find(token);
    if (it == sessions_.end()) {
        return false;
    }
    if (std::chrono::system_clock::now() > it->second.expiresAt) {
        sessions_.erase(it);
        return false;
    }
    userId = it->second.userId;
    return true;
}

void AuthManager::invalidateSession(const std::string& token) {
    sessions_.erase(token);
}

void AuthManager::logout(const std::string& token) {
    sessions_.erase(token);
}

} // namespace forge
