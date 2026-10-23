#include "auth/auth.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <sys/random.h>

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
    unsigned char buf[32];
    ssize_t ret = getrandom(buf, sizeof(buf), 0);
    if (ret != (ssize_t)sizeof(buf)) {
        throw std::runtime_error("Secure random generation failed");
    }
    
    std::stringstream ss;
    for (size_t i = 0; i < sizeof(buf); ++i) {
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)buf[i];
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
    
    unsigned char salt[16];
    ssize_t ret = getrandom(salt, sizeof(salt), 0);
    if (ret != (ssize_t)sizeof(salt)) {
        throw std::runtime_error("Secure random generation failed");
    }
    
    char* encoded = nullptr;
    size_t encoded_len = 0;
    // Calculate encoded length
    encoded_len = argon2_encodedlen(t_cost, m_cost, parallelism, 16, hashlen, Argon2_id);
    encoded = (char*)malloc(encoded_len);
    if (!encoded) throw std::runtime_error("Memory allocation failed");
    
    int result = argon2id_hash_encoded(t_cost, m_cost, parallelism,
                                       password.c_str(), password.length(),
                                       salt, sizeof(salt),
                                       hashlen,
                                       encoded, encoded_len);
    if (result != ARGON2_OK) {
        free(encoded);
        throw std::runtime_error("Argon2id hashing failed");
    }
    std::string res = encoded;
    free(encoded);
    return res;
#  endif
#endif
#endif
    throw std::runtime_error("Argon2id not available");
}

bool AuthManager::verifyPassword(const std::string& password, const std::string& hash) {
    if (hash.empty()) return false;
    if (hash.find("$argon2id$") != 0) return false;
#ifdef HAVE_ARGON2
#ifdef __has_include
#  if __has_include(<argon2.h>)
    try {
        int res = argon2id_verify(hash.c_str(), password.c_str(), password.length());
        if (res == ARGON2_OK) return true;
        if (res == ARGON2_VERIFY_MISMATCH) return false;
    } catch (...) {
        return false;
    }
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
