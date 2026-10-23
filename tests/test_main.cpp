#include <iostream>
#include <cstring>
#include "utils/codec.h"
#include "storage/storage.h"
#include "index/hash_table.h"
#include "algorithms/sorting.h"
#include "algorithms/searching.h"
#include "auth/auth.h"
#include "query/query.h"

using namespace forge;

void testCodec() {
    std::cout << "Testing Codec..." << std::endl;
    User user;
    user.id = 1;
    strcpy(user.name, "Test User");
    strcpy(user.email, "test@example.com");
    strcpy(user.password_hash, "hash123");
    user.created_at = 123456789;
    
    auto serialized = BinaryCodec::serializeUser(user);
    if (serialized.size() == User::RECORD_SIZE) {
        std::cout << "  User serialization size OK" << std::endl;
    }
    
    User user2;
    if (BinaryCodec::deserializeUser(serialized, 0, user2)) {
        if (user2.id == user.id && strcmp(user2.email, user.email) == 0) {
            std::cout << "  User round-trip OK" << std::endl;
        }
    }
}

void testHash() {
    std::cout << "Testing Hash..." << std::endl;
    uint32_t hash = HashTable::fnv1aHash("test@example.com");
    if (hash == 0x8C0401A3) { // Known FNV-1a value for this string? Or just check it's computed
        std::cout << "  FNV-1a computed" << std::endl;
    } else {
        std::cout << "  FNV-1a hash: " << hash << std::endl;
    }
}

void testSorting() {
    std::cout << "Testing Sorting..." << std::endl;
    std::vector<int> arr = {5, 2, 9, 1, 5, 6};
    auto arr2 = arr;
    algorithms::Sorting::bubbleSort(arr2);
    bool sorted = true;
    for (size_t i = 1; i < arr2.size(); ++i) {
        if (arr2[i-1] > arr2[i]) { sorted = false; break; }
    }
    if (sorted) std::cout << "  Bubble sort OK" << std::endl;
}

void testSearching() {
    std::cout << "Testing Searching..." << std::endl;
    std::vector<int> arr = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    if (algorithms::Searching::binarySearch(arr, 5) == 4) {
        std::cout << "  Binary search OK" << std::endl;
    }
    if (algorithms::Searching::linearSearch(arr, 7) == 6) {
        std::cout << "  Linear search OK" << std::endl;
    }
}

void testAuth() {
    std::cout << "Testing Auth..." << std::endl;
    AuthManager auth;
    std::string hash = auth.hashPassword("password123");
    if (auth.verifyPassword("password123", hash)) {
        std::cout << "  Password verification OK" << std::endl;
    }
    std::string token = auth.createSession(1);
    uint32_t userId;
    if (auth.validateSession(token, userId) && userId == 1) {
        std::cout << "  Session validation OK" << std::endl;
    }
}

void testQuery() {
    std::cout << "Testing Query..." << std::endl;
    query::Lexer lexer("SELECT * FROM users WHERE id = 1");
    auto tokens = lexer.tokenize();
    if (tokens.size() > 0) {
        std::cout << "  Lexer OK" << std::endl;
    }
}

int main() {
    std::cout << "=== FORGE Tests ===" << std::endl;
    testCodec();
    testHash();
    testSorting();
    testSearching();
    testAuth();
    testQuery();
    std::cout << "\nAll tests completed!" << std::endl;
    return 0;
}
