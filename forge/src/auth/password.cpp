#include "forge/auth/password.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <stdexcept>

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
namespace auth {

std::string Password::hashPassword(const std::string& password) {
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
    throw std::runtime_error("Argon2id not available");
}

bool Password::verifyPassword(const std::string& password, const std::string& hash) {
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
#endif
#endif
#endif
    return false;
}

} // namespace auth
} // namespace forge
