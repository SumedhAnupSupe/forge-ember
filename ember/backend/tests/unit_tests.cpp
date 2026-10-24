#include <nlohmann/json.hpp>
#include <cassert>
#include <string>

int main() {
    using nlohmann::json;
    const auto request = json{{"name", "Ada"}, {"email", "ada@example.test"}, {"password", "long-enough-password"}};
    assert(request.is_object());
    assert(request["name"].get<std::string>() == "Ada");
    assert(request["email"].get<std::string>().find('@') != std::string::npos);
    const auto response = json{{"token", "example-token"}, {"user_id", 7}};
    assert(response["token"].is_string());
    assert(response["user_id"].is_number_unsigned() || response["user_id"].is_number_integer());
    return 0;
}
