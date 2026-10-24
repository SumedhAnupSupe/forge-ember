#include "ember/server.hpp"
#include <cstdlib>
#include <iostream>
#include <regex>
#include <utility>

namespace ember {
namespace {
json error_json(const std::string& message) { return json{{"error", message}}; }
void respond(httplib::Response& res, const json& payload, int status = 200) {
    res.status = status;
    res.set_content(payload.dump(), "application/json; charset=utf-8");
}
std::string token_from(const httplib::Request& req) {
    const auto value = req.get_header_value("Authorization");
    if (value.rfind("Bearer ", 0) != 0 || value.size() <= 7) return {};
    return value.substr(7);
}
int forge_status(const json& value, int fallback = 502) {
    if (value.contains("_forge_status") && value["_forge_status"].is_number_integer())
        return value["_forge_status"].get<int>();
    if (value.contains("status") && value["status"] == 502) return 502;
    return fallback;
}
json clean(json value) { value.erase("_forge_status"); return value; }
}
Server::Server(std::string host, int port, std::string forge_url)
    : host_(std::move(host)), port_(port), forge_(std::move(forge_url)) { register_routes(); }

void Server::register_routes() {
    server_.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        auto forge_health = forge_.get("/health");
        const int upstream = forge_status(forge_health, 200);
        respond(res, json{{"service", "ember"}, {"status", "ok"},
                          {"forge_reachable", upstream == 200}}, 200);
    });
    server_.Post("/api/v1/auth/register", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            const auto body = json::parse(req.body);
            if (!body.is_object() || !body.contains("name") || !body.contains("email") ||
                !body.contains("password") || !body["name"].is_string() ||
                !body["email"].is_string() || !body["password"].is_string()) {
                respond(res, error_json("name, email and password are required"), 400); return;
            }
            auto result = forge_.post("/api/v1/users", body);
            const int status = forge_status(result, 502);
            respond(res, clean(result), status);
        } catch (...) { respond(res, error_json("Invalid JSON request"), 400); }
    });
    server_.Post("/api/v1/auth/login", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            const auto body = json::parse(req.body);
            if (!body.is_object() || !body.contains("email") || !body.contains("password") ||
                !body["email"].is_string() || !body["password"].is_string()) {
                respond(res, error_json("email and password are required"), 400); return;
            }
            auto result = forge_.post("/api/v1/auth/login", body);
            const int status = forge_status(result, 502);
            respond(res, clean(result), status);
        } catch (...) { respond(res, error_json("Invalid JSON request"), 400); }
    });
    server_.Post("/api/v1/auth/logout", [&](const httplib::Request& req, httplib::Response& res) {
        const auto token = token_from(req);
        if (token.empty()) { respond(res, error_json("Unauthorized"), 401); return; }
        auto result = forge_.post("/api/v1/auth/logout", json::object(), token);
        respond(res, clean(result), forge_status(result));
    });
    server_.Get("/api/v1/auth/me", [&](const httplib::Request& req, httplib::Response& res) {
        const auto token = token_from(req);
        if (token.empty()) { respond(res, error_json("Unauthorized"), 401); return; }
        auto result = forge_.get("/api/v1/users/me", token);
        respond(res, clean(result), forge_status(result));
    });
    server_.Get("/api/v1/messages", [&](const httplib::Request& req, httplib::Response& res) {
        const auto token = token_from(req);
        if (token.empty()) { respond(res, error_json("Unauthorized"), 401); return; }
        auto result = forge_.get("/api/v1/messages", token);
        respond(res, clean(result), forge_status(result));
    });
    server_.Get(R"(/api/v1/messages/([0-9]+))", [&](const httplib::Request& req, httplib::Response& res) {
        const auto token = token_from(req);
        if (token.empty()) { respond(res, error_json("Unauthorized"), 401); return; }
        auto result = forge_.get("/api/v1/messages/" + req.matches[1].str(), token);
        respond(res, clean(result), forge_status(result));
    });
    server_.Post("/api/v1/messages", [&](const httplib::Request& req, httplib::Response& res) {
        const auto token = token_from(req);
        if (token.empty()) { respond(res, error_json("Unauthorized"), 401); return; }
        try {
            auto body = json::parse(req.body);
            if (!body.is_object() || !body.contains("subject") || !body.contains("body") ||
                !body.contains("recipients")) {
                respond(res, error_json("subject, body and recipients are required"), 400); return;
            }
            auto result = forge_.post("/api/v1/messages", body, token);
            respond(res, clean(result), forge_status(result));
        } catch (...) { respond(res, error_json("Invalid JSON request"), 400); }
    });
    server_.Delete(R"(/api/v1/messages/([0-9]+))", [&](const httplib::Request& req, httplib::Response& res) {
        const auto token = token_from(req);
        if (token.empty()) { respond(res, error_json("Unauthorized"), 401); return; }
        auto result = forge_.del("/api/v1/messages/" + req.matches[1].str(), token);
        respond(res, clean(result), forge_status(result));
    });

    // Mailbox flags and restore/permanent-delete routes intentionally are not
    // simulated here: FORGE currently stores read/star/delete on the message
    // record itself, which would incorrectly share those flags across users.
    // Implement them after choosing a per-user mailbox-state storage contract.
    server_.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        respond(res, error_json("Not found"), 404);
    });
    server_.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "http://localhost:5173");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        res.status = 204;
    });
    server_.set_pre_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "http://localhost:5173");
        res.set_header("Vary", "Origin");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        return httplib::Server::HandlerResponse::Unhandled;
    });
}
bool Server::start() {
    std::cout << "EMBER API listening on http://" << host_ << ":" << port_ << '\n';
    return server_.listen(host_, port_);
}
}
