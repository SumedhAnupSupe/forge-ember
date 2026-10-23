#include "query/query.h"
#include <algorithm>
#include <cctype>
#include <iostream>

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
    while (pos_ < input_.size() && isspace(currentChar())) {
        advance();
    }
}

Token Lexer::readIdentifier() {
    std::string result;
    while (pos_ < input_.size() && (isalnum(currentChar()) || currentChar() == '_')) {
        result += currentChar();
        advance();
    }
    
    // Check keywords
    std::string upper = result;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    Token token;
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
    else if (upper == "TO") { token.type = TokenType::IDENTIFIER; token.value = result; return token; }
    else if (upper == "CC") { token.type = TokenType::IDENTIFIER; token.value = result; return token; }
    else if (upper == "BCC") { token.type = TokenType::IDENTIFIER; token.value = result; return token; }
    else token.type = TokenType::IDENTIFIER;
    
    token.value = result;
    return token;
}

Token Lexer::readNumber() {
    std::string result;
    while (pos_ < input_.size() && isdigit(currentChar())) {
        result += currentChar();
        advance();
    }
    Token token;
    token.type = TokenType::NUMBER;
    token.value = result;
    return token;
}

Token Lexer::readString() {
    std::string result;
    char quote = currentChar();
    advance(); // skip opening quote
    while (pos_ < input_.size() && currentChar() != quote) {
        result += currentChar();
        advance();
    }
    if (pos_ < input_.size()) advance(); // skip closing quote
    Token token;
    token.type = TokenType::STRING;
    token.value = result;
    return token;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos_ < input_.size()) {
        skipWhitespace();
        if (pos_ >= input_.size()) break;
        char c = currentChar();
        
        if (isalpha(c) || c == '_') {
            tokens.push_back(readIdentifier());
        } else if (isdigit(c)) {
            tokens.push_back(readNumber());
        } else if (c == '"' || c == '\'') {
            tokens.push_back(readString());
        } else if (c == ',') {
            tokens.push_back({TokenType::COMMA, ","});
            advance();
        } else if (c == ';') {
            tokens.push_back({TokenType::SEMICOLON, ";"});
            advance();
        } else if (c == '(') {
            tokens.push_back({TokenType::LPAREN, "("});
            advance();
        } else if (c == ')') {
            tokens.push_back({TokenType::RPAREN, ")"});
            advance();
        } else if (c == '*') {
            tokens.push_back({TokenType::STAR, "*"});
            advance();
        } else if (c == '=') {
            tokens.push_back({TokenType::EQ, "="});
            advance();
        } else if (c == '!') {
            advance();
            if (currentChar() == '=') {
                tokens.push_back({TokenType::NEQ, "!="});
                advance();
            }
        } else if (c == '<') {
            advance();
            if (currentChar() == '=') {
                tokens.push_back({TokenType::LTE, "<="});
                advance();
            } else {
                tokens.push_back({TokenType::LT, "<"});
            }
        } else if (c == '>') {
            advance();
            if (currentChar() == '=') {
                tokens.push_back({TokenType::GTE, ">="});
                advance();
            } else {
                tokens.push_back({TokenType::GT, ">"});
            }
        } else {
            advance();
        }
    }
    tokens.push_back({TokenType::END, ""});
    return tokens;
}

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {
}

Token Parser::currentToken() {
    if (pos_ >= tokens_.size()) return {TokenType::END, ""};
    return tokens_[pos_];
}

void Parser::advance() {
    if (pos_ < tokens_.size()) ++pos_;
}

Token Parser::eat(TokenType type) {
    Token token = currentToken();
    if (token.type != type) {
        // Try to be lenient
    }
    advance();
    return token;
}

std::unique_ptr<ASTNode> Parser::parse() {
    Token token = currentToken();
    if (token.type == TokenType::SELECT) {
        return parseSelect();
    } else if (token.type == TokenType::INSERT) {
        return parseInsert();
    } else if (token.type == TokenType::UPDATE) {
        return parseUpdate();
    } else if (token.type == TokenType::DELETE) {
        return parseDelete();
    }
    return nullptr;
}

std::unique_ptr<SelectNode> Parser::parseSelect() {
    auto node = std::make_unique<SelectNode>();
    eat(TokenType::SELECT);
    
    if (currentToken().type == TokenType::STAR) {
        node->columns.push_back("*");
        advance();
    } else {
        while (currentToken().type == TokenType::IDENTIFIER) {
            node->columns.push_back(currentToken().value);
            advance();
            if (currentToken().type == TokenType::COMMA) {
                advance();
            } else {
                break;
            }
        }
    }
    
    if (currentToken().type == TokenType::FROM) {
        eat(TokenType::FROM);
        if (currentToken().type == TokenType::IDENTIFIER) {
            node->table = currentToken().value;
            advance();
        }
    }
    
    if (currentToken().type == TokenType::WHERE) {
        eat(TokenType::WHERE);
        // Simple where clause - just consume tokens until ORDER BY or LIMIT or END
        std::string whereStr;
        while (currentToken().type != TokenType::ORDER && 
               currentToken().type != TokenType::LIMIT &&
               currentToken().type != TokenType::END &&
               currentToken().type != TokenType::SEMICOLON) {
            whereStr += currentToken().value + " ";
            advance();
        }
        node->whereClause = whereStr;
    }
    
    if (currentToken().type == TokenType::ORDER) {
        eat(TokenType::ORDER);
        if (currentToken().type == TokenType::BY) {
            eat(TokenType::BY);
            std::string orderStr;
            while (currentToken().type != TokenType::LIMIT &&
                   currentToken().type != TokenType::END &&
                   currentToken().type != TokenType::SEMICOLON) {
                orderStr += currentToken().value + " ";
                advance();
            }
            node->orderBy = orderStr;
        }
    }
    
    if (currentToken().type == TokenType::LIMIT) {
        eat(TokenType::LIMIT);
        if (currentToken().type == TokenType::NUMBER) {
            node->limit = std::stoi(currentToken().value);
            advance();
        }
    }
    
    return node;
}

std::unique_ptr<InsertNode> Parser::parseInsert() {
    auto node = std::make_unique<InsertNode>();
    eat(TokenType::INSERT);
    if (currentToken().type == TokenType::INTO) eat(TokenType::INTO);
    if (currentToken().type == TokenType::IDENTIFIER) {
        node->table = currentToken().value;
        advance();
    }
    // Simplified parsing
    if (currentToken().type == TokenType::LPAREN) {
        advance();
        while (currentToken().type == TokenType::IDENTIFIER) {
            node->columns.push_back(currentToken().value);
            advance();
            if (currentToken().type == TokenType::COMMA) advance();
            else break;
        }
        if (currentToken().type == TokenType::RPAREN) advance();
    }
    if (currentToken().type == TokenType::VALUES) eat(TokenType::VALUES);
    if (currentToken().type == TokenType::LPAREN) {
        advance();
        while (currentToken().type == TokenType::STRING || currentToken().type == TokenType::NUMBER || currentToken().type == TokenType::IDENTIFIER) {
            node->values.push_back(currentToken().value);
            advance();
            if (currentToken().type == TokenType::COMMA) advance();
            else break;
        }
        if (currentToken().type == TokenType::RPAREN) advance();
    }
    return node;
}

std::unique_ptr<UpdateNode> Parser::parseUpdate() {
    auto node = std::make_unique<UpdateNode>();
    eat(TokenType::UPDATE);
    if (currentToken().type == TokenType::IDENTIFIER) {
        node->table = currentToken().value;
        advance();
    }
    if (currentToken().type == TokenType::SET) eat(TokenType::SET);
    // Simplified
    while (currentToken().type == TokenType::IDENTIFIER) {
        std::string col = currentToken().value;
        advance();
        if (currentToken().type == TokenType::EQ) advance();
        if (currentToken().type == TokenType::STRING || currentToken().type == TokenType::NUMBER) {
            node->setClauses[col] = currentToken().value;
            advance();
        }
        if (currentToken().type == TokenType::COMMA) advance();
        else break;
    }
    if (currentToken().type == TokenType::WHERE) {
        eat(TokenType::WHERE);
        std::string whereStr;
        while (currentToken().type != TokenType::END && currentToken().type != TokenType::SEMICOLON) {
            whereStr += currentToken().value + " ";
            advance();
        }
        node->whereClause = whereStr;
    }
    return node;
}

std::unique_ptr<DeleteNode> Parser::parseDelete() {
    auto node = std::make_unique<DeleteNode>();
    eat(TokenType::DELETE);
    if (currentToken().type == TokenType::FROM) eat(TokenType::FROM);
    if (currentToken().type == TokenType::IDENTIFIER) {
        node->table = currentToken().value;
        advance();
    }
    if (currentToken().type == TokenType::WHERE) {
        eat(TokenType::WHERE);
        std::string whereStr;
        while (currentToken().type != TokenType::END && currentToken().type != TokenType::SEMICOLON) {
            whereStr += currentToken().value + " ";
            advance();
        }
        node->whereClause = whereStr;
    }
    return node;
}

QueryResult QueryExecutor::execute(const std::string& query) {
    QueryResult result;
    result.success = false;
    result.message = "Query execution not fully implemented";
    return result;
}

} // namespace query
} // namespace forge
