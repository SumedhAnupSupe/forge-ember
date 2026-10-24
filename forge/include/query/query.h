#ifndef QUERY_H
#define QUERY_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>

namespace forge {

class Storage;

namespace query {

enum class TokenType {
    SELECT, INSERT, UPDATE, DELETE, FROM, WHERE, ORDER, BY, LIMIT,
    INTO, VALUES, SET,
    AND, OR,
    EQ, NEQ, LT, LTE, GT, GTE,
    IDENTIFIER, STRING, NUMBER,
    COMMA, SEMICOLON, LPAREN, RPAREN,
    STAR,
    ASC, DESC,
    SHOW, TABLES, DESCRIBE,
    END
};

struct Token {
    TokenType type;
    std::string value;
    size_t position = 0;
};

class Lexer {
public:
    Lexer(const std::string& input);
    std::vector<Token> tokenize();
    
private:
    std::string input_;
    size_t pos_;
    char currentChar();
    void advance();
    void skipWhitespace();
    Token readIdentifier();
    Token readNumber();
    Token readString();
};

struct SelectNode {
    std::vector<std::string> columns;
    std::string table;
    std::string whereColumn;
    std::string whereOp; // =
    std::string whereValue;
    std::string orderByColumn;
    std::string orderByDir; // ASC/DESC
    int limit;
    SelectNode() : limit(-1) {}
};

struct QueryResult {
    bool success;
    std::string message;
    std::vector<std::map<std::string, std::string>> rows;
};

class QueryExecutor {
public:
    QueryExecutor(std::shared_ptr<forge::Storage> storage = nullptr);
    QueryResult execute(const std::string& query);
    // Executes a SELECT with row-level visibility restricted to the authenticated user.
    // User ID 0 is reserved for trusted internal tests/tools and must not be used by HTTP routes.
    QueryResult execute(const std::string& query, uint32_t authenticatedUserId);
private:
    std::shared_ptr<forge::Storage> storage_;
};

} // namespace query
} // namespace forge

#endif // QUERY_H
