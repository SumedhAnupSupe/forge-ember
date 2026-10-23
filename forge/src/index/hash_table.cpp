#include "index/hash_table.h"
#include <stdexcept>

namespace forge {

HashTable::HashTable(size_t bucketCount) : bucketCount_(bucketCount) {
    if (bucketCount_ == 0) throw std::invalid_argument("Bucket count cannot be zero");
    buckets_.resize(bucketCount_, nullptr);
}

HashTable::~HashTable() {
    clear();
}

HashTable::HashTable(HashTable&& other) noexcept 
    : buckets_(std::move(other.buckets_)), bucketCount_(other.bucketCount_) {
    other.bucketCount_ = 0;
}

HashTable& HashTable::operator=(HashTable&& other) noexcept {
    if (this != &other) {
        clear();
        buckets_ = std::move(other.buckets_);
        bucketCount_ = other.bucketCount_;
        other.bucketCount_ = 0;
    }
    return *this;
}

uint32_t HashTable::fnv1aHash(const std::string& data) {
    const uint32_t offsetBasis = 2166136261u;
    const uint32_t prime = 16777619u;
    uint32_t hash = offsetBasis;
    for (unsigned char c : data) {
        hash ^= c;
        hash *= prime;
    }
    return hash;
}

uint32_t HashTable::fnv1aHash(const char* data, size_t len) {
    const uint32_t offsetBasis = 2166136261u;
    const uint32_t prime = 16777619u;
    uint32_t hash = offsetBasis;
    for (size_t i = 0; i < len; ++i) {
        hash ^= (unsigned char)data[i];
        hash *= prime;
    }
    return hash;
}

void HashTable::insert(const std::string& key, uint32_t value) {
    if (bucketCount_ == 0) throw std::invalid_argument("Bucket count cannot be zero");
    uint32_t hash = fnv1aHash(key);
    size_t index = hash % bucketCount_;
    // Avoid duplicate key-value pairs
    HashNode* current = buckets_[index];
    while (current) {
        if (current->key == key && current->value == value) {
            return;
        }
        current = current->next;
    }
    HashNode* newNode = new HashNode(key, value);
    newNode->next = buckets_[index];
    buckets_[index] = newNode;
}

bool HashTable::find(const std::string& key, std::vector<uint32_t>& values) {
    values.clear();
    if (bucketCount_ == 0) return false;
    uint32_t hash = fnv1aHash(key);
    size_t index = hash % bucketCount_;
    HashNode* current = buckets_[index];
    while (current) {
        if (current->key == key) {
            values.push_back(current->value);
        }
        current = current->next;
    }
    return !values.empty();
}

bool HashTable::remove(const std::string& key, uint32_t value) {
    if (bucketCount_ == 0) return false;
    uint32_t hash = fnv1aHash(key);
    size_t index = hash % bucketCount_;
    HashNode* current = buckets_[index];
    HashNode* prev = nullptr;
    while (current) {
        if (current->key == key && current->value == value) {
            if (prev) {
                prev->next = current->next;
            } else {
                buckets_[index] = current->next;
            }
            delete current;
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

void HashTable::clear() {
    for (size_t i = 0; i < bucketCount_; ++i) {
        HashNode* current = buckets_[i];
        while (current) {
            HashNode* temp = current;
            current = current->next;
            delete temp;
        }
        buckets_[i] = nullptr;
    }
}

void HashTable::rebuild() {
    // No-op; rebuild is handled by IndexManager
}

} // namespace forge
