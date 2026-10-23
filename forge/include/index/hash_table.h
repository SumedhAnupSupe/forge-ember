#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <string>
#include <vector>
#include <cstdint>

namespace forge {

struct HashNode {
    std::string key;
    uint32_t value;
    HashNode* next;
    HashNode(const std::string& k, uint32_t v) : key(k), value(v), next(nullptr) {}
};

class HashTable {
public:
    HashTable(size_t bucketCount = 1009);
    ~HashTable();
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;
    HashTable(HashTable&& other) noexcept;
    HashTable& operator=(HashTable&& other) noexcept;
    
    // FNV-1a 32-bit hash
    static uint32_t fnv1aHash(const std::string& data);
    static uint32_t fnv1aHash(const char* data, size_t len);
    
    void insert(const std::string& key, uint32_t value);
    bool find(const std::string& key, std::vector<uint32_t>& values);
    bool remove(const std::string& key, uint32_t value);
    void clear();
    void rebuild();
    
private:
    std::vector<HashNode*> buckets_;
    size_t bucketCount_;
};

} // namespace forge

#endif // HASH_TABLE_H
