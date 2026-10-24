#include "ember/forge_client.hpp"
#include <algorithm>
#include <memory>

namespace ember {
ForgeClient::ForgeClient(std::string origin) : origin_(std::move(origin)) {
    while (!origin_.empty() && origin_.back() == '/') origin_.pop_back();
}

nlohmann::json ForgeClient::request(const std::string& method, const std::string& path,
                                    const std::string& bearer, const nlohmann::json* body,
                                    int& status, std::string& content_type) const {
    status = 502;
    content_type = "application/json";
    if (origin_.rfind("http://", 0) != 0) {
        return {{"error", "EMBER currently requires an http:// FORGE origin in local development."}};
    }
    const std::string authority = origin_.substr(7);
    const auto slash = authority.find('/');
    const std::string host_port = authority.substr(0, slash);
    auto client = std::make_unique<httplib::Client>("http://" + host_port);
    client->set_connection_timeout(2, 0);
    client->set_read_timeout(5, 0);
    client->set_write_timeout(5, 0);
    httplib::Headers headers;
    if (!bearer.empty()) headers.emplace("Authorization", "Bearer " + bearer);
    headers.emplace("Accept", "application/json");
    std::string target = path;
    if (slash != std::string::npos && slash + 1 < authority.size()) {
        target = "/" + authority.substr(slash + 1) + path;
    }
    httplib::Result response;
    if (method == "GET") response = client->Get(target, headers);
    else if (method == "POST") response = client->Post(target, headers, body ? body->dump() : "{}", "application/json");
    else if (method == "DELETE") response = client->Delete(target, headers);
    else return {{"error", "Unsupported upstream method"}};
    if (!response) return {{"error", "FORGE is unavailable. Start forge-server and try again."}};
    status = response->status;
    if (response->get_header_value("Content-Type").size()) content_type = response->get_header_value("Content-Type");
    try {
        return nlohmann::json::parse(response->body);
    } catch (...) {
        return {{"error", "FORGE returned a non-JSON response"}};
    }
}
}
