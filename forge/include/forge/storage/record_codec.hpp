#ifndef CODEC_H
#define CODEC_H

#include "models/records.h"
#include <vector>
#include <string>
#include <cstdint>

namespace forge {

class BinaryCodec {
public:
    // User serialization
    static std::vector<uint8_t> serializeUser(const User& user);
    static bool deserializeUser(const std::vector<uint8_t>& data, size_t offset, User& user);
    
    // Message serialization
    static std::vector<uint8_t> serializeMessage(const Message& message);
    static bool deserializeMessage(const std::vector<uint8_t>& data, size_t offset, Message& message);
    
    // Recipient serialization
    static std::vector<uint8_t> serializeRecipient(const Recipient& recipient);
    static bool deserializeRecipient(const std::vector<uint8_t>& data, size_t offset, Recipient& recipient);
    
    // Helper functions for endianness
    static void writeUint32LE(std::vector<uint8_t>& data, uint32_t value);
    static void writeUint64LE(std::vector<uint8_t>& data, uint64_t value);
    static uint32_t readUint32LE(const std::vector<uint8_t>& data, size_t& offset);
    static uint64_t readUint64LE(const std::vector<uint8_t>& data, size_t& offset);
    
    static void writeString(std::vector<uint8_t>& data, const char* str, size_t maxLen);
    static void readString(const std::vector<uint8_t>& data, size_t& offset, char* str, size_t maxLen);
};

} // namespace forge

#endif // CODEC_H
