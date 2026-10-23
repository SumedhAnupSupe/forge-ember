#include "utils/codec.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace forge {

void BinaryCodec::writeUint32LE(std::vector<uint8_t>& data, uint32_t value) {
    data.push_back((uint8_t)(value & 0xFF));
    data.push_back((uint8_t)((value >> 8) & 0xFF));
    data.push_back((uint8_t)((value >> 16) & 0xFF));
    data.push_back((uint8_t)((value >> 24) & 0xFF));
}

void BinaryCodec::writeUint64LE(std::vector<uint8_t>& data, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        data.push_back((uint8_t)((value >> (8 * i)) & 0xFF));
    }
}

bool BinaryCodec::readUint32LE(const std::vector<uint8_t>& data, size_t& offset, uint32_t& out) {
    if (offset + 4 > data.size()) return false;
    uint32_t value = (uint32_t)data[offset] |
                    ((uint32_t)data[offset + 1] << 8) |
                    ((uint32_t)data[offset + 2] << 16) |
                    ((uint32_t)data[offset + 3] << 24);
    offset += 4;
    out = value;
    return true;
}

bool BinaryCodec::readUint64LE(const std::vector<uint8_t>& data, size_t& offset, uint64_t& out) {
    if (offset + 8 > data.size()) return false;
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= ((uint64_t)data[offset + i] << (8 * i));
    }
    offset += 8;
    out = value;
    return true;
}

bool BinaryCodec::writeString(std::vector<uint8_t>& data, const char* str, size_t maxLen) {
    if (maxLen == 0) return false;
    size_t len = 0;
    if (str) {
        len = strlen(str);
        if (len > maxLen - 1) {
            return false; // Do not silently truncate
        }
    }
    for (size_t i = 0; i < len; ++i) {
        data.push_back((uint8_t)str[i]);
    }
    for (size_t i = len; i < maxLen; ++i) {
        data.push_back(0);
    }
    return true;
}

bool BinaryCodec::readString(const std::vector<uint8_t>& data, size_t& offset, char* str, size_t maxLen) {
    if (maxLen == 0) return false;
    if (offset + maxLen > data.size()) return false;
    memcpy(str, &data[offset], maxLen);
    str[maxLen - 1] = '\0';
    offset += maxLen;
    return true;
}

std::vector<uint8_t> BinaryCodec::serializeUser(const User& user) {
    std::vector<uint8_t> data;
    data.reserve(User::ENCODED_RECORD_SIZE);
    writeUint32LE(data, user.id);
    if (!writeString(data, user.name, 64)) throw std::runtime_error("String too long");
    if (!writeString(data, user.email, 128)) throw std::runtime_error("String too long");
    if (!writeString(data, user.password_hash, 256)) throw std::runtime_error("String too long");
    writeUint64LE(data, user.created_at);
    if (user.is_deleted != 0 && user.is_deleted != 1) throw std::runtime_error("Invalid flag");
    data.push_back(user.is_deleted);
    return data;
}

bool BinaryCodec::deserializeUser(const std::vector<uint8_t>& data, size_t offset, User& user) {
    if (offset + User::ENCODED_RECORD_SIZE > data.size()) return false;
    size_t off = offset;
    uint32_t id = 0; uint64_t created = 0;
    if (!readUint32LE(data, off, id)) return false;
    if (!readString(data, off, user.name, 64)) return false;
    if (!readString(data, off, user.email, 128)) return false;
    if (!readString(data, off, user.password_hash, 256)) return false;
    if (!readUint64LE(data, off, created)) return false;
    if (off >= data.size()) return false;
    uint8_t del = data[off++];
    if (del != 0 && del != 1) return false;
    user.id = id;
    user.created_at = created;
    user.is_deleted = del;
    return true;
}

std::vector<uint8_t> BinaryCodec::serializeMessage(const Message& message) {
    std::vector<uint8_t> data;
    data.reserve(Message::ENCODED_RECORD_SIZE);
    writeUint32LE(data, message.id);
    writeUint32LE(data, message.sender_id);
    if (!writeString(data, message.subject, 128)) throw std::runtime_error("String too long");
    if (!writeString(data, message.body, 512)) throw std::runtime_error("String too long");
    writeUint64LE(data, message.created_at);
    if (message.is_read != 0 && message.is_read != 1) throw std::runtime_error("Invalid flag");
    if (message.is_starred != 0 && message.is_starred != 1) throw std::runtime_error("Invalid flag");
    if (message.is_deleted != 0 && message.is_deleted != 1) throw std::runtime_error("Invalid flag");
    data.push_back(message.is_read);
    data.push_back(message.is_starred);
    data.push_back(message.is_deleted);
    return data;
}

bool BinaryCodec::deserializeMessage(const std::vector<uint8_t>& data, size_t offset, Message& message) {
    if (offset + Message::ENCODED_RECORD_SIZE > data.size()) return false;
    size_t off = offset;
    uint32_t id = 0, sender = 0; uint64_t created = 0;
    if (!readUint32LE(data, off, id)) return false;
    if (!readUint32LE(data, off, sender)) return false;
    if (!readString(data, off, message.subject, 128)) return false;
    if (!readString(data, off, message.body, 512)) return false;
    if (!readUint64LE(data, off, created)) return false;
    if (off + 3 > data.size()) return false;
    uint8_t r = data[off++], s = data[off++], d = data[off++];
    if ((r!=0&&r!=1)||(s!=0&&s!=1)||(d!=0&&d!=1)) return false;
    message.id = id; message.sender_id = sender; message.created_at = created;
    message.is_read = r; message.is_starred = s; message.is_deleted = d;
    return true;
}

std::vector<uint8_t> BinaryCodec::serializeRecipient(const Recipient& recipient) {
    std::vector<uint8_t> data;
    data.reserve(Recipient::ENCODED_RECORD_SIZE);
    writeUint32LE(data, recipient.message_id);
    writeUint32LE(data, recipient.user_id);
    if (!writeString(data, recipient.type, 4)) throw std::runtime_error("String too long");
    std::string t = recipient.type;
    if (t != "TO" && t != "CC" && t != "BCC") throw std::runtime_error("Invalid recipient type");
    if (recipient.is_deleted != 0 && recipient.is_deleted != 1) throw std::runtime_error("Invalid flag");
    data.push_back(recipient.is_deleted);
    return data;
}

bool BinaryCodec::deserializeRecipient(const std::vector<uint8_t>& data, size_t offset, Recipient& recipient) {
    if (offset + Recipient::ENCODED_RECORD_SIZE > data.size()) return false;
    size_t off = offset;
    uint32_t mid = 0, uid = 0;
    if (!readUint32LE(data, off, mid)) return false;
    if (!readUint32LE(data, off, uid)) return false;
    if (!readString(data, off, recipient.type, 4)) return false;
    std::string t = recipient.type;
    if (t != "TO" && t != "CC" && t != "BCC") return false;
    if (off >= data.size()) return false;
    uint8_t d = data[off++];
    if (d != 0 && d != 1) return false;
    recipient.message_id = mid; recipient.user_id = uid; recipient.is_deleted = d;
    return true;
}

} // namespace forge
