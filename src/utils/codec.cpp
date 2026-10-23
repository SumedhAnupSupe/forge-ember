#include "utils/codec.h"
#include <cstring>
#include <algorithm>

namespace forge {

void BinaryCodec::writeUint32LE(std::vector<uint8_t>& data, uint32_t value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 24) & 0xFF);
}

void BinaryCodec::writeUint64LE(std::vector<uint8_t>& data, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        data.push_back((value >> (8 * i)) & 0xFF);
    }
}

uint32_t BinaryCodec::readUint32LE(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) return 0;
    uint32_t value = data[offset] | 
                    (data[offset + 1] << 8) | 
                    (data[offset + 2] << 16) | 
                    (data[offset + 3] << 24);
    offset += 4;
    return value;
}

uint64_t BinaryCodec::readUint64LE(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 8 > data.size()) return 0;
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= ((uint64_t)data[offset + i] << (8 * i));
    }
    offset += 8;
    return value;
}

void BinaryCodec::writeString(std::vector<uint8_t>& data, const char* str, size_t maxLen) {
    size_t len = str ? strlen(str) : 0;
    if (len > maxLen - 1) len = maxLen - 1;
    for (size_t i = 0; i < len; ++i) {
        data.push_back((uint8_t)str[i]);
    }
    for (size_t i = len; i < maxLen; ++i) {
        data.push_back(0);
    }
}

void BinaryCodec::readString(const std::vector<uint8_t>& data, size_t& offset, char* str, size_t maxLen) {
    if (offset + maxLen > data.size()) {
        // Fill with zeros if not enough data
        memset(str, 0, maxLen);
        offset += maxLen;
        return;
    }
    memcpy(str, &data[offset], maxLen);
    str[maxLen - 1] = '\0'; // Ensure null termination
    offset += maxLen;
}

std::vector<uint8_t> BinaryCodec::serializeUser(const User& user) {
    std::vector<uint8_t> data;
    data.reserve(User::RECORD_SIZE);
    writeUint32LE(data, user.id);
    writeString(data, user.name, 64);
    writeString(data, user.email, 128);
    writeString(data, user.password_hash, 256);
    writeUint64LE(data, user.created_at);
    return data;
}

bool BinaryCodec::deserializeUser(const std::vector<uint8_t>& data, size_t offset, User& user) {
    if (offset + User::RECORD_SIZE > data.size()) return false;
    size_t off = offset;
    user.id = readUint32LE(data, off);
    readString(data, off, user.name, 64);
    readString(data, off, user.email, 128);
    readString(data, off, user.password_hash, 256);
    user.created_at = readUint64LE(data, off);
    return true;
}

std::vector<uint8_t> BinaryCodec::serializeMessage(const Message& message) {
    std::vector<uint8_t> data;
    data.reserve(Message::RECORD_SIZE);
    writeUint32LE(data, message.id);
    writeUint32LE(data, message.sender_id);
    writeString(data, message.subject, 128);
    writeString(data, message.body, 512);
    writeUint64LE(data, message.created_at);
    data.push_back(message.is_read);
    data.push_back(message.is_starred);
    data.push_back(message.is_deleted);
    return data;
}

bool BinaryCodec::deserializeMessage(const std::vector<uint8_t>& data, size_t offset, Message& message) {
    if (offset + Message::RECORD_SIZE > data.size()) return false;
    size_t off = offset;
    message.id = readUint32LE(data, off);
    message.sender_id = readUint32LE(data, off);
    readString(data, off, message.subject, 128);
    readString(data, off, message.body, 512);
    message.created_at = readUint64LE(data, off);
    if (off < data.size()) message.is_read = data[off++];
    if (off < data.size()) message.is_starred = data[off++];
    if (off < data.size()) message.is_deleted = data[off++];
    return true;
}

std::vector<uint8_t> BinaryCodec::serializeRecipient(const Recipient& recipient) {
    std::vector<uint8_t> data;
    data.reserve(Recipient::RECORD_SIZE);
    writeUint32LE(data, recipient.message_id);
    writeUint32LE(data, recipient.user_id);
    writeString(data, recipient.type, 4);
    return data;
}

bool BinaryCodec::deserializeRecipient(const std::vector<uint8_t>& data, size_t offset, Recipient& recipient) {
    if (offset + Recipient::RECORD_SIZE > data.size()) return false;
    size_t off = offset;
    recipient.message_id = readUint32LE(data, off);
    recipient.user_id = readUint32LE(data, off);
    readString(data, off, recipient.type, 4);
    return true;
}

} // namespace forge
