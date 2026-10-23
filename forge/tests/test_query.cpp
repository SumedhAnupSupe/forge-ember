#include <iostream>
#include "query/query.h"

using namespace forge::query;

int test_query() {
    int failures = 0;
    std::cout << "Testing Query..." << std::endl;
    Lexer lexer("SELECT * FROM users WHERE id = 1");
    auto tokens = lexer.tokenize();
    if (!tokens.empty()) std::cout << "  Lexer OK" << std::endl;
    else { failures++; }
    Lexer lexer2("SELECT * FROM users WHERE name = \"test\"");
    auto tokens2 = lexer2.tokenize();
    bool hasUnterminated = false;
    for (auto& t : tokens2) if (t.type == TokenType::END && t.value == "unterminated string") hasUnterminated = true;
    if (!hasUnterminated) std::cout << "  String lex OK" << std::endl;
    else { failures++; }
    QueryExecutor exec(nullptr);
    auto r = exec.execute("INSERT INTO users VALUES (1)");
    if (!r.success) std::cout << "  Unsupported INSERT rejected OK" << std::endl;
    else { failures++; }
    auto r2 = exec.execute("SELECT * FROM nonexistent");
    if (!r2.success) std::cout << "  Unknown table rejected OK" << std::endl;
    else { failures++; }
    return failures;
}
