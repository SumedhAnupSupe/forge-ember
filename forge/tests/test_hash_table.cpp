#include <iostream>
#include "index/hash_table.h"

using namespace forge;

int test_hash_table() {
    int failures = 0;
    std::cout << "Testing HashTable..." << std::endl;
    try {
        HashTable ht(1009);
        ht.insert("key1", 1);
        ht.insert("key1", 2); // multiple values
        std::vector<uint32_t> vals;
        if (ht.find("key1", vals) && vals.size() == 2) std::cout << "  Multiple values OK" << std::endl;
        else { std::cout << "  Multiple values FAILED" << std::endl; failures++; }
        if (ht.remove("key1", 1)) std::cout << "  Removal OK" << std::endl;
        else { std::cout << "  Removal FAILED" << std::endl; failures++; }
        ht.clear();
    } catch (...) { failures++; }
    try {
        HashTable ht(1); // Every key collides into the same bucket.
        ht.insert("alpha", 10);
        ht.insert("beta", 20);
        ht.insert("gamma", 30);
        ht.insert("beta", 20); // Duplicate key-value pair must be ignored.
        std::vector<uint32_t> alpha, beta, gamma;
        const bool allFound = ht.find("alpha", alpha) && ht.find("beta", beta) &&
                              ht.find("gamma", gamma);
        const bool uniqueValues = alpha.size() == 1 && alpha[0] == 10 &&
                                  beta.size() == 1 && beta[0] == 20 &&
                                  gamma.size() == 1 && gamma[0] == 30;
        const bool collisionRemoval = ht.remove("beta", 20) &&
                                      !ht.find("beta", beta) &&
                                      ht.find("alpha", alpha) && ht.find("gamma", gamma);
        if (allFound && uniqueValues && collisionRemoval)
            std::cout << "  Collision-chain lookup, deduplication, and removal OK" << std::endl;
        else { std::cout << "  Collision-chain behavior FAILED" << std::endl; failures++; }
        if (!ht.remove("missing", 99)) std::cout << "  Missing-key removal OK" << std::endl;
        else { std::cout << "  Missing-key removal FAILED" << std::endl; failures++; }
        ht.clear();
        if (!ht.find("alpha", alpha) && !ht.find("gamma", gamma))
            std::cout << "  Clear releases all collision-chain entries OK" << std::endl;
        else { std::cout << "  Clear FAILED" << std::endl; failures++; }
    } catch (...) { std::cout << "  Collision-chain test threw unexpectedly" << std::endl; failures++; }
    try {
        HashTable ht(0);
        std::cout << "  Zero bucket FAILED" << std::endl; failures++;
    } catch (...) {
        std::cout << "  Zero bucket rejected OK" << std::endl;
    }
    return failures;
}
