#ifndef QUERY_H
#define QUERY_H

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace forge {
namespace query {

enum class TokenType {
    SELECT, INSERT, UPDATE, DELETE, FROM, WHERE, ORDER, BY, LIMIT,
    INTO, VALUES, SET,
    AND, OR,
    EQ, NEQ, LT, LTE, GT, GTE,
    IDENTIFIER, STRING, NUMBER,
    COMMA, SEMICOLON, LPAREN, RPAREN,
    STAR,
    END
};

struct Token {
    TokenType type;
    std::string value;
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

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct SelectNode : public ASTNode {
    std::vector<std::string> columns;
    std::string table;
    std::string whereClause;
    std::string orderBy;
    int limit;
    SelectNode() : limit(-1) {}
};

struct InsertNode : public ASTNode {
    std::string table;
    std::vector<std::string> columns;
    std::vector<std::string> values;
};

struct UpdateNode : public ASTNode {
    std::string table;
    std::map<std::string, std::string> setClauses;
    std::string whereClause;
};

struct DeleteNode : public ASTNode {
    std::string table;
    std::string whereClause;
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<ASTNode> parse();
    
private:
    std::vector<Token> tokens_;
    size_t pos_;
    Token currentToken();
    void advance();
    Token eat(TokenType type);
    std::unique_ptr<SelectNode> parseSelect();
    std::unique_ptr<InsertNode> parseInsert();
    std::unique_ptr<UpdateNode> parseUpdate();
    std::unique_ptr<DeleteNode> parseDelete();
};

struct QueryResult {
    bool success;
    std::string message;
    std::vector<std::map<std::string, std::string>> rows;
};

class QueryExecutor {
public:
    QueryResult execute(const std::string& query);
};

} // namespace query
} // namespace forge

#endif // QUERY_H
