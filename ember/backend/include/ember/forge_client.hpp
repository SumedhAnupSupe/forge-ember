#pragma once
#include <string>
#include <httplib.h>
#include <json.hpp>

namespace ember {
class ForgeClient {
public:
    explicit ForgeClient(std::string origin);
    nlohmann::json request(const std::string& method, const std::string& path,
                           const std::string& bearer, const nlohmann::json* body,
                           int& status, std::string& content_type) const;
private:
    std::string origin_;
};
}
