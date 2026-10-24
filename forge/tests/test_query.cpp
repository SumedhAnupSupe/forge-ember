#include <iostream>
#include <filesystem>
#include <cstring>
#include <chrono>
#include "query/query.h"
#include "storage/storage.h"

using namespace forge;
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

    QueryExecutor noStorage(nullptr);
    if (!noStorage.execute("INSERT INTO users VALUES (1)").success)
        std::cout << "  Unsupported INSERT rejected OK" << std::endl;
    else failures++;
    if (!noStorage.execute("SELECT * FROM nonexistent").success)
        std::cout << "  Query without storage rejected OK" << std::endl;
    else failures++;

    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto dir = std::filesystem::temp_directory_path() /
        ("forge_query_auth_test_" + std::to_string(unique));
    auto storage = std::make_shared<Storage>(dir.string());
    if (!storage->open()) {
        std::cerr << "  Could not open temporary query-test storage" << std::endl;
        return failures + 1;
    }

    auto addUser = [&](uint32_t id, const char* name, const char* email) {
        User u{};
        u.id = id;
        std::strncpy(u.name, name, sizeof(u.name) - 1);
        std::strncpy(u.email, email, sizeof(u.email) - 1);
        u.created_at = id;
        return storage->createUser(u);
    };
    if (!addUser(1, "Alice", "alice@example.com") ||
        !addUser(2, "Bob", "bob@example.com") ||
        !addUser(3, "Carol", "carol@example.com")) {
        std::cerr << "  Could not create test users" << std::endl;
        storage->close();
        std::filesystem::remove_all(dir);
        return failures + 1;
    }

    Message message{};
    message.id = 10;
    message.sender_id = 1;
    std::strncpy(message.subject, "Private test", sizeof(message.subject) - 1);
    std::strncpy(message.body, "Body", sizeof(message.body) - 1);
    message.created_at = 10;
    if (!storage->createMessage(message)) {
        std::cerr << "  Could not create test message" << std::endl;
        storage->close();
        std::filesystem::remove_all(dir);
        return failures + 1;
    }
    Recipient bob{};
    bob.message_id = 10;
    bob.user_id = 2;
    std::strncpy(bob.type, "TO", sizeof(bob.type) - 1);
    Recipient carol{};
    carol.message_id = 10;
    carol.user_id = 3;
    std::strncpy(carol.type, "CC", sizeof(carol.type) - 1);
    if (!storage->createRecipient(bob) || !storage->createRecipient(carol)) {
        std::cerr << "  Could not create test recipients" << std::endl;
        storage->close();
        std::filesystem::remove_all(dir);
        return failures + 1;
    }

    QueryExecutor executor(storage);
    const auto ownUsers = executor.execute("SELECT ID, EMAIL FROM USERS;", 1);
    if (ownUsers.success && ownUsers.rows.size() == 1 &&
        ownUsers.rows[0].at("id") == "1" &&
        ownUsers.rows[0].at("email") == "alice@example.com")
        std::cout << "  Case-insensitive SELECT and own-user isolation OK" << std::endl;
    else { std::cerr << "  Own-user isolation failed" << std::endl; failures++; }

    const auto otherUser = executor.execute("SELECT * FROM users WHERE id = 2;", 1);
    if (otherUser.success && otherUser.rows.empty())
        std::cout << "  Cross-user query returns no rows OK" << std::endl;
    else { std::cerr << "  Cross-user query isolation failed" << std::endl; failures++; }

    const auto recipientMessage = executor.execute("SELECT id, subject FROM messages;", 2);
    if (recipientMessage.success && recipientMessage.rows.size() == 1 &&
        recipientMessage.rows[0].at("id") == "10")
        std::cout << "  Authorized recipient can read message OK" << std::endl;
    else { std::cerr << "  Recipient message access failed" << std::endl; failures++; }

    const auto hiddenRecipient = executor.execute("SELECT user_id FROM recipients;", 2);
    if (hiddenRecipient.success && hiddenRecipient.rows.size() == 1 &&
        hiddenRecipient.rows[0].at("user_id") == "2")
        std::cout << "  Recipient rows are scoped to account OK" << std::endl;
    else { std::cerr << "  Recipient-row isolation failed" << std::endl; failures++; }

    if (!executor.execute("SELECT * FROM users; SELECT * FROM messages;", 1).success)
        std::cout << "  Multiple statements rejected OK" << std::endl;
    else { std::cerr << "  Multiple-statement rejection failed" << std::endl; failures++; }

    if (!executor.execute("SELECT * FROM users LIMIT 2147483648;", 1).success)
        std::cout << "  LIMIT overflow rejected OK" << std::endl;
    else { std::cerr << "  LIMIT overflow rejection failed" << std::endl; failures++; }

    storage->close();
    std::filesystem::remove_all(dir);
    return failures;
}
