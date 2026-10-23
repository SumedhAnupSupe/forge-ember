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
        HashTable ht(0);
        std::cout << "  Zero bucket FAILED" << std::endl; failures++;
    } catch (...) {
        std::cout << "  Zero bucket rejected OK" << std::endl;
    }
    return failures;
}
