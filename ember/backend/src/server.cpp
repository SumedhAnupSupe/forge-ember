#include "ember/server.hpp"
#include <json.hpp>
#include <regex>
#include <utility>

namespace ember {
using json = nlohmann::json;

EmberServer::EmberServer(std::string host, int port, std::string forge_origin)
    : host_(std::move(host)), port_(port), forge_(std::move(forge_origin)) {
    configureRoutes();
}

void EmberServer::configureRoutes() {
    server_.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "http://localhost:3000");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        res.set_header("Vary", "Origin");
        if (req.method == "OPTIONS") { res.status = 204; return httplib::Server::HandlerResponse::Handled; }
        return httplib::Server::HandlerResponse::Unhandled;
    });
    server_.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok","service":"ember"})", "application/json");
    });

    auto proxy = [this](const httplib::Request& req, httplib::Response& res,
                        const std::string& method, const std::string& path) {
        std::string bearer;
        const auto auth = req.get_header_value("Authorization");
        if (auth.rfind("Bearer ", 0) == 0) bearer = auth.substr(7);
        json body;
        const json* body_ptr = nullptr;
        if (method == "POST") {
            try { body = json::parse(req.body); body_ptr = &body; }
            catch (...) { res.status = 400; res.set_content(R"({"error":"Invalid JSON body"})", "application/json"); return; }
        }
        int status = 502;
        std::string content_type;
        const json result = forge_.request(method, path, bearer, body_ptr, status, content_type);
        res.status = status;
        res.set_content(result.dump(), "application/json");
    };

    server_.Post("/api/v1/auth/login", [proxy](const httplib::Request& req, httplib::Response& res) { proxy(req, res, "POST", "/api/v1/auth/login"); });
    server_.Post("/api/v1/auth/logout", [proxy](const httplib::Request& req, httplib::Response& res) { proxy(req, res, "POST", "/api/v1/auth/logout"); });
    server_.Post("/api/v1/users", [proxy](const httplib::Request& req, httplib::Response& res) { proxy(req, res, "POST", "/api/v1/users"); });
    server_.Get("/api/v1/users/me", [proxy](const httplib::Request& req, httplib::Response& res) { proxy(req, res, "GET", "/api/v1/users/me"); });
    server_.Get("/api/v1/users/lookup", [proxy](const httplib::Request& req, httplib::Response& res) { proxy(req, res, "GET", req.target); });
    server_.Post("/api/v1/query", [proxy](const httplib::Request& req, httplib::Response& res) { proxy(req, res, "POST", "/api/v1/query"); });
    server_.Get("/api/v1/messages", [proxy](const httplib::Request& req, httplib::Response& res) { proxy(req, res, "GET", "/api/v1/messages"); });
    server_.Post("/api/v1/messages", [proxy](const httplib::Request& req, httplib::Response& res) { proxy(req, res, "POST", "/api/v1/messages"); });
    server_.Get(R"(/api/v1/messages/(\d+))", [proxy](const httplib::Request& req, httplib::Response& res) {
        proxy(req, res, "GET", req.path);
    });
    server_.Delete(R"(/api/v1/messages/(\d+))", [proxy](const httplib::Request& req, httplib::Response& res) {
        proxy(req, res, "DELETE", req.path);
    });
}

bool EmberServer::start() {
    return server_.listen(host_, port_);
}
}
