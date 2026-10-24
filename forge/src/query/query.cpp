#include "query/query.h"
#include "storage/storage.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <climits>

namespace forge {
namespace query {

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {
}

char Lexer::currentChar() {
    if (pos_ >= input_.size()) return '\0';
    return input_[pos_];
}

void Lexer::advance() {
    if (pos_ < input_.size()) ++pos_;
}

void Lexer::skipWhitespace() {
    while (pos_ < input_.size() && isspace((unsigned char)currentChar())) {
        advance();
    }
}

Token Lexer::readIdentifier() {
    std::string result;
    size_t startPos = pos_;
    while (pos_ < input_.size() && (isalnum((unsigned char)currentChar()) || currentChar() == '_')) {
        result += currentChar();
        advance();
    }
    
    std::string upper = result;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    Token token;
    token.position = startPos;
    if (upper == "SELECT") token.type = TokenType::SELECT;
    else if (upper == "INSERT") token.type = TokenType::INSERT;
    else if (upper == "UPDATE") token.type = TokenType::UPDATE;
    else if (upper == "DELETE") token.type = TokenType::DELETE;
    else if (upper == "FROM") token.type = TokenType::FROM;
    else if (upper == "WHERE") token.type = TokenType::WHERE;
    else if (upper == "ORDER") token.type = TokenType::ORDER;
    else if (upper == "BY") token.type = TokenType::BY;
    else if (upper == "LIMIT") token.type = TokenType::LIMIT;
    else if (upper == "INTO") token.type = TokenType::INTO;
    else if (upper == "VALUES") token.type = TokenType::VALUES;
    else if (upper == "SET") token.type = TokenType::SET;
    else if (upper == "AND") token.type = TokenType::AND;
    else if (upper == "OR") token.type = TokenType::OR;
    else if (upper == "ASC") token.type = TokenType::ASC;
    else if (upper == "DESC") token.type = TokenType::DESC;
    else if (upper == "SHOW") token.type = TokenType::SHOW;
    else if (upper == "TABLES") token.type = TokenType::TABLES;
    else if (upper == "DESCRIBE") token.type = TokenType::DESCRIBE;
    else token.type = TokenType::IDENTIFIER;
    
    token.value = result;
    return token;
}

Token Lexer::readNumber() {
    std::string result;
    size_t startPos = pos_;
    while (pos_ < input_.size() && isdigit((unsigned char)currentChar())) {
        result += currentChar();
        advance();
    }
    Token token;
    token.type = TokenType::NUMBER;
    token.value = result;
    token.position = startPos;
    return token;
}

Token Lexer::readString() {
    std::string result;
    size_t startPos = pos_;
    char quote = currentChar();
    advance(); // skip opening quote
    while (pos_ < input_.size() && currentChar() != quote) {
        if (currentChar() == '\\') {
            advance();
            if (pos_ < input_.size()) {
                result += currentChar();
                advance();
                continue;
            }
        }
        result += currentChar();
        advance();
    }
    if (pos_ >= input_.size() || currentChar() != quote) {
        Token token;
        token.type = TokenType::END; // indicate error
        token.value = "unterminated string";
        token.position = startPos;
        return token;
    }
    advance(); // skip closing quote
    Token token;
    token.type = TokenType::STRING;
    token.value = result;
    token.position = startPos;
    return token;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos_ < input_.size()) {
        skipWhitespace();
        if (pos_ >= input_.size()) break;
        char c = currentChar();
        
        if (isalpha((unsigned char)c) || c == '_') {
            tokens.push_back(readIdentifier());
        } else if (isdigit((unsigned char)c)) {
            tokens.push_back(readNumber());
        } else if (c == '"' || c == '\'') {
            Token t = readString();
            if (t.type == TokenType::END && t.value == "unterminated string") {
                tokens.push_back(t);
                return tokens;
            }
            tokens.push_back(t);
        } else if (c == ',') {
            tokens.push_back({TokenType::COMMA, ",", pos_});
            advance();
        } else if (c == ';') {
            tokens.push_back({TokenType::SEMICOLON, ";", pos_});
            advance();
        } else if (c == '(') {
            tokens.push_back({TokenType::LPAREN, "(", pos_});
            advance();
        } else if (c == ')') {
            tokens.push_back({TokenType::RPAREN, ")", pos_});
            advance();
        } else if (c == '*') {
            tokens.push_back({TokenType::STAR, "*", pos_});
            advance();
        } else if (c == '=') {
            tokens.push_back({TokenType::EQ, "=", pos_});
            advance();
        } else if (c == '!') {
            advance();
            if (pos_ < input_.size() && currentChar() == '=') {
                tokens.push_back({TokenType::NEQ, "!=", pos_});
                advance();
            } else {
                return tokens;
            }
        } else if (c == '<') {
            advance();
            if (pos_ < input_.size() && currentChar() == '=') {
                tokens.push_back({TokenType::LTE, "<=", pos_});
                advance();
            } else {
                tokens.push_back({TokenType::LT, "<", pos_});
            }
        } else if (c == '>') {
            advance();
            if (pos_ < input_.size() && currentChar() == '=') {
                tokens.push_back({TokenType::GTE, ">=", pos_});
                advance();
            } else {
                tokens.push_back({TokenType::GT, ">", pos_});
            }
        } else {
            return tokens;
        }
    }
    tokens.push_back({TokenType::END, "", pos_});
    return tokens;
}

QueryExecutor::QueryExecutor(std::shared_ptr<forge::Storage> storage) : storage_(storage) {
}

QueryResult QueryExecutor::execute(const std::string& query) {
    return execute(query, 0);
}

QueryResult QueryExecutor::execute(const std::string& query, uint32_t authenticatedUserId) {
    QueryResult result;
    result.success = false;
    Lexer lexer(query);
    auto tokens = lexer.tokenize();
    if (tokens.empty()) {
        result.message = "Empty query";
        return result;
    }
    // Check for unterminated string
    for (const auto& t : tokens) {
        if (t.type == TokenType::END && t.value == "unterminated string") {
            result.message = "Parse error: unterminated string";
            return result;
        }
    }
    
    size_t pos = 0;
    auto current = [&]() -> const Token& { if (pos >= tokens.size()) return tokens.empty() ? tokens.back() : tokens.back(); return tokens[pos]; };
    auto advance = [&]() { if (pos < tokens.size()) pos++; };
    auto eat = [&](TokenType type) -> bool { if (current().type == type) { advance(); return true; } return false; };
    
    if (current().type != TokenType::SELECT) {
        result.message = "Unsupported statement";
        return result;
    }
    advance(); // SELECT
    
    SelectNode node;
    if (current().type == TokenType::STAR) {
        node.columns.push_back("*");
        advance();
    } else {
        while (current().type == TokenType::IDENTIFIER || current().type == TokenType::COMMA) {
            if (current().type == TokenType::COMMA) { advance(); continue; }
            if (current().type == TokenType::IDENTIFIER) {
                node.columns.push_back(current().value);
                advance();
                continue;
            }
            break;
        }
        if (node.columns.empty()) {
            result.message = "Parse error: missing columns";
            return result;
        }
    }
    
    for (auto& column : node.columns) {
        std::transform(column.begin(), column.end(), column.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }
    
    if (!eat(TokenType::FROM)) {
        result.message = "Parse error: expected FROM";
        return result;
    }
    if (current().type != TokenType::IDENTIFIER) {
        result.message = "Parse error: expected table name";
        return result;
    }
    node.table = current().value;
    std::transform(node.table.begin(), node.table.end(), node.table.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    advance();
    
    if (current().type == TokenType::WHERE) {
        advance();
        if (current().type != TokenType::IDENTIFIER) {
            result.message = "Parse error: invalid WHERE";
            return result;
        }
        node.whereColumn = current().value;
        std::transform(node.whereColumn.begin(), node.whereColumn.end(), node.whereColumn.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        advance();
        if (current().type == TokenType::EQ) {
            node.whereOp = "=";
            advance();
        } else {
            result.message = "Parse error: unsupported operator";
            return result;
        }
        if (current().type == TokenType::STRING) {
            node.whereValue = current().value;
            advance();
        } else if (current().type == TokenType::NUMBER) {
            node.whereValue = current().value;
            advance();
        } else {
            result.message = "Parse error: invalid WHERE value";
            return result;
        }
    }
    
    if (current().type == TokenType::ORDER) {
        advance();
        if (!eat(TokenType::BY)) {
            result.message = "Parse error: expected BY";
            return result;
        }
        if (current().type != TokenType::IDENTIFIER) {
            result.message = "Parse error: invalid ORDER BY";
            return result;
        }
        node.orderByColumn = current().value;
        std::transform(node.orderByColumn.begin(), node.orderByColumn.end(), node.orderByColumn.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        advance();
        if (current().type == TokenType::ASC) { node.orderByDir = "ASC"; advance(); }
        else if (current().type == TokenType::DESC) { node.orderByDir = "DESC"; advance(); }
        else { node.orderByDir = "ASC"; }
    }
    
    if (current().type == TokenType::LIMIT) {
        advance();
        if (current().type != TokenType::NUMBER) {
            result.message = "Parse error: invalid LIMIT";
            return result;
        }
        try {
            // Check for overflow
            std::string numstr = current().value;
            // Simple check
            if (numstr.length() > 18) {
                result.message = "Parse error: invalid LIMIT";
                return result;
            }
            long long lval = std::stoll(numstr);
            if (lval < 0 || lval > INT_MAX) { result.message = "Parse error: invalid LIMIT"; return result; }
            node.limit = static_cast<int>(lval);
        } catch (...) {
            result.message = "Parse error: invalid LIMIT";
            return result;
        }
        advance();
    }
    
    if (current().type == TokenType::SEMICOLON) advance();
    if (current().type != TokenType::END) {
        result.message = "Parse error: unexpected trailing tokens";
        return result;
    }
    
    if (!storage_) {
        result.message = "No storage";
        return result;
    }
    
    // Validate table
    if (node.table != "users" && node.table != "messages" && node.table != "recipients") {
        result.message = "Unknown table";
        return result;
    }
    
    // Validate columns
    if (node.table == "users") {
        const std::vector<std::string> allowed = {"id", "name", "email", "created_at", "password_hash"};
        if (node.columns.size() == 1 && node.columns[0] == "*") {
            // allowed public columns only
        } else {
            for (const auto& c : node.columns) {
                if (c == "password_hash") { result.message = "Unknown column"; return result; } // don't allow exposing hash
                bool ok = false;
                for (const auto& a : {"id", "name", "email", "created_at"}) {
                    if (c == a) { ok = true; break; }
                }
                if (!ok) { result.message = "Unknown column"; return result; }
            }
        }
        if (!node.whereColumn.empty()) {
            if (node.whereColumn == "password_hash") { result.message = "Unknown column"; return result; }
            bool ok = false;
            for (const auto& a : {"id", "name", "email", "created_at"}) {
                if (node.whereColumn == a) { ok = true; break; }
            }
            if (!ok) { result.message = "Unknown column"; return result; }
        }
        if (!node.orderByColumn.empty()) {
            bool ok = false;
            for (const auto& a : {"id", "name", "email", "created_at"}) {
                if (node.orderByColumn == a) { ok = true; break; }
            }
            if (!ok) { result.message = "Unknown column"; return result; }
        }
        auto users = storage_->getAllUsers(false);
        for (const auto& u : users) {
            if (authenticatedUserId != 0 && u.id != authenticatedUserId) continue;
            if (!node.whereColumn.empty()) {
                if (node.whereColumn == "email" && node.whereValue != u.email) continue;
                else if (node.whereColumn == "id" && node.whereValue != std::to_string(u.id)) continue;
                else if (node.whereColumn == "name" && node.whereValue != u.name) continue;
                else if (node.whereColumn == "created_at" && node.whereValue != std::to_string(u.created_at)) continue;
                else continue;
            }
            std::map<std::string, std::string> row;
            if (node.columns.size() == 1 && node.columns[0] == "*") {
                row["id"] = std::to_string(u.id);
                row["name"] = u.name;
                row["email"] = u.email;
                row["created_at"] = std::to_string(u.created_at);
            } else {
                for (const auto& c : node.columns) {
                    if (c == "id") row[c] = std::to_string(u.id);
                    else if (c == "name") row[c] = u.name;
                    else if (c == "email") row[c] = u.email;
                    else if (c == "created_at") row[c] = std::to_string(u.created_at);
                }
            }
            if (!node.orderByColumn.empty() && !row.count(node.orderByColumn)) {
                if (node.orderByColumn == "id") row[node.orderByColumn] = std::to_string(u.id);
                else if (node.orderByColumn == "name") row[node.orderByColumn] = u.name;
                else if (node.orderByColumn == "email") row[node.orderByColumn] = u.email;
                else if (node.orderByColumn == "created_at") row[node.orderByColumn] = std::to_string(u.created_at);
            }
            result.rows.push_back(row);
        }
        if (!node.orderByColumn.empty()) {
            bool desc = (node.orderByDir == "DESC");
            std::stable_sort(result.rows.begin(), result.rows.end(),
                [&](const auto& a, const auto& b) {
                    std::string av = a.count(node.orderByColumn) ? a.at(node.orderByColumn) : "";
                    std::string bv = b.count(node.orderByColumn) ? b.at(node.orderByColumn) : "";
                    // try numeric
                    try {
                        // check if both numeric
                        bool anum = !av.empty() && std::all_of(av.begin(), av.end(), ::isdigit);
                        bool bnum = !bv.empty() && std::all_of(bv.begin(), bv.end(), ::isdigit);
                        if (anum && bnum) {
                            long long ai = std::stoll(av);
                            long long bi = std::stoll(bv);
                            if (desc) return ai > bi;
                            return ai < bi;
                        }
                    } catch (...) {}
                    if (desc) return av > bv;
                    return av < bv;
                });
        }
        if (!(node.columns.size() == 1 && node.columns[0] == "*") &&
            !node.orderByColumn.empty() &&
            std::find(node.columns.begin(), node.columns.end(), node.orderByColumn) == node.columns.end()) {
            for (auto& row : result.rows) row.erase(node.orderByColumn);
        }
        if (node.limit >= 0 && (int)result.rows.size() > node.limit) {
            result.rows.resize(node.limit);
        }
        result.success = true;
        result.message = "OK";
        return result;
    } else if (node.table == "messages") {
        const std::vector<std::string> allowed = {"id", "sender_id", "subject", "body", "created_at"};
        if (!(node.columns.size() == 1 && node.columns[0] == "*")) {
            for (const auto& c : node.columns) {
                bool ok = false;
                for (const auto& a : {"id", "sender_id", "subject", "body", "created_at"}) {
                    if (c == a) { ok = true; break; }
                }
                if (!ok) { result.message = "Unknown column"; return result; }
            }
        }
        if (!node.whereColumn.empty()) {
            bool ok = false;
            for (const auto& a : {"id", "sender_id", "subject", "body", "created_at"}) {
                if (node.whereColumn == a) { ok = true; break; }
            }
            if (!ok) { result.message = "Unknown column"; return result; }
        }
        if (!node.orderByColumn.empty()) {
            bool ok = false;
            for (const auto& a : {"id", "sender_id", "subject", "body", "created_at"}) {
                if (node.orderByColumn == a) { ok = true; break; }
            }
            if (!ok) { result.message = "Unknown column"; return result; }
        }
        auto messages = storage_->getAllMessages(false);
        for (const auto& m : messages) {
            if (authenticatedUserId != 0 && m.sender_id != authenticatedUserId) {
                std::vector<Recipient> visibleRecipients;
                if (!storage_->readRecipientsByMessage(m.id, visibleRecipients, false)) continue;
                const bool isRecipient = std::any_of(visibleRecipients.begin(), visibleRecipients.end(),
                    [&](const Recipient& recipient) { return recipient.user_id == authenticatedUserId && recipient.is_deleted == 0; });
                if (!isRecipient) continue;
            }
            if (!node.whereColumn.empty()) {
                if (node.whereColumn == "id" && node.whereValue != std::to_string(m.id)) continue;
                else if (node.whereColumn == "sender_id" && node.whereValue != std::to_string(m.sender_id)) continue;
                else if (node.whereColumn == "subject" && node.whereValue != m.subject) continue;
                else if (node.whereColumn == "body" && node.whereValue != m.body) continue;
                else if (node.whereColumn == "created_at" && node.whereValue != std::to_string(m.created_at)) continue;
                else continue;
            }
            std::map<std::string, std::string> row;
            if (node.columns.size() == 1 && node.columns[0] == "*") {
                row["id"] = std::to_string(m.id);
                row["sender_id"] = std::to_string(m.sender_id);
                row["subject"] = m.subject;
                row["body"] = m.body;
                row["created_at"] = std::to_string(m.created_at);
            } else {
                for (const auto& c : node.columns) {
                    if (c == "id") row[c] = std::to_string(m.id);
                    else if (c == "sender_id") row[c] = std::to_string(m.sender_id);
                    else if (c == "subject") row[c] = m.subject;
                    else if (c == "body") row[c] = m.body;
                    else if (c == "created_at") row[c] = std::to_string(m.created_at);
                }
            }
            if (!node.orderByColumn.empty() && !row.count(node.orderByColumn)) {
                if (node.orderByColumn == "id") row[node.orderByColumn] = std::to_string(m.id);
                else if (node.orderByColumn == "sender_id") row[node.orderByColumn] = std::to_string(m.sender_id);
                else if (node.orderByColumn == "subject") row[node.orderByColumn] = m.subject;
                else if (node.orderByColumn == "body") row[node.orderByColumn] = m.body;
                else if (node.orderByColumn == "created_at") row[node.orderByColumn] = std::to_string(m.created_at);
            }
            result.rows.push_back(row);
        }
        if (!node.orderByColumn.empty()) {
            bool desc = (node.orderByDir == "DESC");
            std::stable_sort(result.rows.begin(), result.rows.end(),
                [&](const auto& a, const auto& b) {
                    std::string av = a.count(node.orderByColumn) ? a.at(node.orderByColumn) : "";
                    std::string bv = b.count(node.orderByColumn) ? b.at(node.orderByColumn) : "";
                    try {
                        bool anum = !av.empty() && std::all_of(av.begin(), av.end(), [](unsigned char ch){ return std::isdigit(ch); });
                        bool bnum = !bv.empty() && std::all_of(bv.begin(), bv.end(), [](unsigned char ch){ return std::isdigit(ch); });
                        if (anum && bnum) {
                            // handle negative? not expected
                            long long ai = std::stoll(av);
                            long long bi = std::stoll(bv);
                            if (desc) return ai > bi;
                            return ai < bi;
                        }
                    } catch (...) {}
                    if (desc) return av > bv;
                    return av < bv;
                });
        }
        if (!(node.columns.size() == 1 && node.columns[0] == "*") &&
            !node.orderByColumn.empty() &&
            std::find(node.columns.begin(), node.columns.end(), node.orderByColumn) == node.columns.end()) {
            for (auto& row : result.rows) row.erase(node.orderByColumn);
        }
        if (node.limit >= 0 && (int)result.rows.size() > node.limit) {
            result.rows.resize(node.limit);
        }
        result.success = true;
        result.message = "OK";
        return result;
    } else if (node.table == "recipients") {
        const std::vector<std::string> allowed = {"message_id", "user_id", "type"};
        if (!(node.columns.size() == 1 && node.columns[0] == "*")) {
            for (const auto& c : node.columns) {
                bool ok = false;
                for (const auto& a : allowed) {
                    if (c == a) { ok = true; break; }
                }
                if (!ok) { result.message = "Unknown column"; return result; }
            }
        }
        if (!node.whereColumn.empty()) {
            bool ok = false;
            for (const auto& a : allowed) {
                if (node.whereColumn == a) { ok = true; break; }
            }
            if (!ok) { result.message = "Unknown column"; return result; }
        }
        if (!node.orderByColumn.empty()) {
            bool ok = false;
            for (const auto& a : allowed) {
                if (node.orderByColumn == a) { ok = true; break; }
            }
            if (!ok) { result.message = "Unknown column"; return result; }
        }
        auto all = storage_->getAllRecipients(false);
        for (const auto& r : all) {
            if (authenticatedUserId != 0 && r.user_id != authenticatedUserId) {
                Message parentMessage;
                if (!storage_->readMessage(r.message_id, parentMessage) || parentMessage.sender_id != authenticatedUserId) continue;
            }
            if (!node.whereColumn.empty()) {
                if (node.whereColumn == "message_id" && node.whereValue != std::to_string(r.message_id)) continue;
                else if (node.whereColumn == "user_id" && node.whereValue != std::to_string(r.user_id)) continue;
                else if (node.whereColumn == "type" && node.whereValue != std::string(r.type)) continue;
                else continue;
            }
            std::map<std::string, std::string> row;
            if (node.columns.size() == 1 && node.columns[0] == "*") {
                row["message_id"] = std::to_string(r.message_id);
                row["user_id"] = std::to_string(r.user_id);
                row["type"] = std::string(r.type);
            } else {
                for (const auto& c : node.columns) {
                    if (c == "message_id") row[c] = std::to_string(r.message_id);
                    else if (c == "user_id") row[c] = std::to_string(r.user_id);
                    else if (c == "type") row[c] = std::string(r.type);
                }
            }
            if (!node.orderByColumn.empty() && !row.count(node.orderByColumn)) {
                if (node.orderByColumn == "message_id") row[node.orderByColumn] = std::to_string(r.message_id);
                else if (node.orderByColumn == "user_id") row[node.orderByColumn] = std::to_string(r.user_id);
                else if (node.orderByColumn == "type") row[node.orderByColumn] = std::string(r.type);
            }
            result.rows.push_back(row);
        }
        if (!node.orderByColumn.empty()) {
            bool desc = (node.orderByDir == "DESC");
            std::stable_sort(result.rows.begin(), result.rows.end(),
                [&](const auto& a, const auto& b) {
                    std::string av = a.count(node.orderByColumn) ? a.at(node.orderByColumn) : "";
                    std::string bv = b.count(node.orderByColumn) ? b.at(node.orderByColumn) : "";
                    try {
                        bool anum = !av.empty() && std::all_of(av.begin(), av.end(), [](unsigned char ch){ return std::isdigit(ch); });
                        bool bnum = !bv.empty() && std::all_of(bv.begin(), bv.end(), [](unsigned char ch){ return std::isdigit(ch); });
                        if (anum && bnum) {
                            long long ai = std::stoll(av);
                            long long bi = std::stoll(bv);
                            if (desc) return ai > bi;
                            return ai < bi;
                        }
                    } catch (...) {}
                    if (desc) return av > bv;
                    return av < bv;
                });
        }
        if (!(node.columns.size() == 1 && node.columns[0] == "*") &&
            !node.orderByColumn.empty() &&
            std::find(node.columns.begin(), node.columns.end(), node.orderByColumn) == node.columns.end()) {
            for (auto& row : result.rows) row.erase(node.orderByColumn);
        }
        if (node.limit >= 0 && (int)result.rows.size() > node.limit) {
            result.rows.resize(node.limit);
        }
        result.success = true;
        result.message = "OK";
        return result;
    }
    
    result.message = "Unknown table";
    return result;
}

} // namespace query
} // namespace forge
