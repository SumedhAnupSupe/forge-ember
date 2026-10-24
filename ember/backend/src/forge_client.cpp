#include "ember/forge_client.hpp"
#include <stdexcept>
#include <utility>

namespace ember {
ForgeClient::ForgeClient(std::string base_url) {
    if (base_url.rfind("http://", 0) == 0) {
        https_ = false;
        base_url.erase(0, 7);
    } else if (base_url.rfind("https://", 0) == 0) {
        https_ = true;
        base_url.erase(0, 8);
    } else {
        throw std::invalid_argument("FORGE_URL must start with http:// or https://");
    }
    const auto slash = base_url.find('/');
    if (slash != std::string::npos) base_url.resize(slash);
    const auto colon = base_url.rfind(':');
    if (colon != std::string::npos) {
        host_ = base_url.substr(0, colon);
        try { port_ = std::stoi(base_url.substr(colon + 1)); }
        catch (...) { throw std::invalid_argument("Invalid port in FORGE_URL"); }
    } else {
        host_ = base_url;
        port_ = https_ ? 443 : 80;
    }
    if (host_.empty() || port_ < 1 || port_ > 65535)
        throw std::invalid_argument("Invalid FORGE_URL");
}

json ForgeClient::request(const std::string& method, const std::string& path,
                          const json* body, const std::string& token) const {
    httplib::Headers headers{{"Accept", "application/json"}};
    if (body) headers.emplace("Content-Type", "application/json");
    if (!token.empty()) headers.emplace("Authorization", "Bearer " + token);

    httplib::Result result;
    if (https_) {
        httplib::SSLClient client(host_, port_);
        client.set_connection_timeout(3, 0);
        client.set_read_timeout(8, 0);
        client.set_write_timeout(8, 0);
        if (method == "GET") result = client.Get(path, headers);
        else if (method == "POST") result = client.Post(path, headers, body ? body->dump() : "", "application/json");
        else if (method == "DELETE") result = client.Delete(path, headers);
    } else {
        httplib::Client client(host_, port_);
        client.set_connection_timeout(3, 0);
        client.set_read_timeout(8, 0);
        client.set_write_timeout(8, 0);
        if (method == "GET") result = client.Get(path, headers);
        else if (method == "POST") result = client.Post(path, headers, body ? body->dump() : "", "application/json");
        else if (method == "DELETE") result = client.Delete(path, headers);
    }
    if (!result) return json{{"error", "FORGE service unavailable"}, {"status", 502}};
    json payload = json::object();
    try { payload = json::parse(result->body); }
    catch (...) { payload = json{{"error", "Invalid response from FORGE"}}; }
    payload["_forge_status"] = result->status;
    return payload;
}
json ForgeClient::get(const std::string& path, const std::string& token) const {
    return request("GET", path, nullptr, token);
}
json ForgeClient::post(const std::string& path, const json& body, const std::string& token) const {
    return request("POST", path, &body, token);
}
json ForgeClient::del(const std::string& path, const std::string& token) const {
    return request("DELETE", path, nullptr, token);
}
}
