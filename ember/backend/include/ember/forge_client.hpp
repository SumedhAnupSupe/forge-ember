#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>

namespace ember {
using json = nlohmann::json;

class ForgeClient {
public:
    explicit ForgeClient(std::string base_url);
    json get(const std::string& path, const std::string& token = "") const;
    json post(const std::string& path, const json& body, const std::string& token = "") const;
    json del(const std::string& path, const std::string& token = "") const;

private:
    std::string host_;
    int port_;
    bool https_;
    json request(const std::string& method, const std::string& path,
                 const json* body, const std::string& token) const;
};
}
