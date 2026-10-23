#include <iostream>
#include "auth/auth.h"

using namespace forge;

int test_auth() {
    int failures = 0;
    std::cout << "Testing Auth..." << std::endl;
    AuthManager auth;
    try {
        std::string hash = auth.hashPassword("password123");
        if (auth.verifyPassword("password123", hash)) std::cout << "  Password verification OK" << std::endl;
        else { std::cout << "  Password verification FAILED" << std::endl; failures++; }
        std::string hash2 = auth.hashPassword("password123");
        if (hash != hash2) std::cout << "  Different salts OK" << std::endl;
        else { std::cout << "  Different salts FAILED" << std::endl; failures++; }
    } catch (const std::exception& e) {
        std::cout << "  Auth blocked as expected: " << e.what() << std::endl;
    }
    try {
        std::string token = auth.createSession(1);
        if (token.length() == 64) std::cout << "  Secure token format OK" << std::endl;
        else { std::cout << "  Secure token format FAILED" << std::endl; failures++; }
        uint32_t uid; if (auth.validateSession(token, uid)) std::cout << "  Session validation OK" << std::endl;
        else { std::cout << "  Session validation FAILED" << std::endl; failures++; }
        auth.logout(token);
        if (!auth.validateSession(token, uid)) std::cout << "  Logout invalidation OK" << std::endl;
        else { std::cout << "  Logout invalidation FAILED" << std::endl; failures++; }
    } catch (const std::exception& e) {
        std::cout << "  Token generation blocked: " << e.what() << std::endl;
        failures++;
    }
    if (!auth.verifyPassword("pass", "$argon2id$bad")) {
        std::cout << "  Malformed hash rejected OK" << std::endl;
    }
    return failures;
}
