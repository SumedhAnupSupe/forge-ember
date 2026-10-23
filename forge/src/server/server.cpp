#include "forge/server/server.h"
#include <iostream>
#include <httplib.h>
#include <json.hpp>
#include "query/query.h"

using json = nlohmann::json;

namespace forge {
namespace server {

ForgeServer::ForgeServer(const std::string& host, int port, const std::string& dataDir)
    : host_(host), port_(port), dataDir_(dataDir) {
}

bool ForgeServer::start() {
    storage_ = std::make_shared<Storage>(dataDir_);
    if (!storage_->open()) {
        std::cerr << "Failed to open storage at " << dataDir_ << std::endl;
        return false;
    }
    indexManager_ = std::make_shared<IndexManager>(storage_);
    indexManager_->buildIndexes();
    userService_ = std::make_shared<UserService>(storage_, indexManager_);
    messageService_ = std::make_shared<MessageService>(storage_, indexManager_);
    authManager_ = std::make_shared<AuthManager>();
    
    std::cout << "FORGE Server starting on " << host_ << ":" << port_ << std::endl;
    std::cout << "Data directory: " << dataDir_ << std::endl;
    
    httplib::Server svr;
    
    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        json j;
        j["status"] = "ok";
        res.set_content(j.dump(), "application/json");
    });
    
    svr.Post("/api/v1/users", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            std::string name = j.value("name", "");
            std::string email = j.value("email", "");
            std::string password = j.value("password", "");
            std::string hash;
            try {
                hash = authManager_->hashPassword(password);
            } catch (...) {
                res.status = 500;
                json err; err["error"] = "Authentication not configured";
                res.set_content(err.dump(), "application/json");
                return;
            }
            User u;
            if (userService_->createUser(name, email, hash, u)) {
                json r;
                r["id"] = u.id; r["name"] = u.name; r["email"] = u.email;
                res.status = 201;
                res.set_content(r.dump(), "application/json");
            } else {
                res.status = 400;
                json err; err["error"] = "Failed to create user";
                res.set_content(err.dump(), "application/json");
            }
        } catch (...) {
            res.status = 400;
            json err; err["error"] = "Invalid request";
            res.set_content(err.dump(), "application/json");
        }
    });
    
    svr.Post("/api/v1/auth/login", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            std::string email = j.value("email", "");
            std::string password = j.value("password", "");
            User u;
            if (userService_->getUserByEmail(email, u)) {
                try {
                    if (authManager_->verifyPassword(password, u.password_hash)) {
                        std::string token = authManager_->createSession(u.id);
                        json r; r["token"] = token; r["user_id"] = u.id;
                        res.set_content(r.dump(), "application/json");
                        return;
                    }
                } catch (...) {
                    res.status = 401;
                    json err; err["error"] = "Auth error";
                    res.set_content(err.dump(), "application/json");
                    return;
                }
            }
            res.status = 401;
            json err; err["error"] = "Invalid credentials";
            res.set_content(err.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            json err; err["error"] = "Invalid request";
            res.set_content(err.dump(), "application/json");
        }
    });
    
    svr.Post("/api/v1/auth/logout", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") == 0) {
            std::string token = auth.substr(7);
            authManager_->logout(token);
        }
        json r; r["status"] = "ok";
        res.set_content(r.dump(), "application/json");
    });
    
    svr.Get("/api/v1/users/me", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        std::string token = auth.substr(7);
        uint32_t uid;
        if (!authManager_->validateSession(token, uid)) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        User u;
        if (userService_->getUserById(uid, u)) {
            json r; r["id"] = u.id; r["name"] = u.name; r["email"] = u.email;
            res.set_content(r.dump(), "application/json");
        } else {
            res.status = 404;
            json err; err["error"] = "Not found";
            res.set_content(err.dump(), "application/json");
        }
    });
    
    svr.Get("/api/v1/messages", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        std::string token = auth.substr(7);
        uint32_t uid;
        if (!authManager_->validateSession(token, uid)) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        auto msgs = messageService_->getMessagesForUser(uid, false);
        json arr = json::array();
        for (const auto& m : msgs) {
            json item;
            item["id"] = m.id; item["sender_id"] = m.sender_id; item["subject"] = m.subject; item["body"] = m.body; item["created_at"] = m.created_at;
            arr.push_back(item);
        }
        res.set_content(arr.dump(), "application/json");
    });
    
    svr.Post("/api/v1/messages", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        std::string token = auth.substr(7);
        uint32_t uid;
        if (!authManager_->validateSession(token, uid)) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        try {
            auto j = json::parse(req.body);
            std::string subject = j.value("subject", "");
            std::string body = j.value("body", "");
            std::vector<std::pair<uint32_t, RecipientType>> recips;
            if (j.contains("recipients") && j["recipients"].is_array()) {
                for (const auto& r : j["recipients"]) {
                    uint32_t rid = r.value("user_id", 0);
                    std::string t = r.value("type", "TO");
                    if (t == "CC") recips.push_back({rid, RecipientType::CC});
                    else if (t == "BCC") recips.push_back({rid, RecipientType::BCC});
                    else recips.push_back({rid, RecipientType::TO});
                }
            }
            Message m;
            if (messageService_->createMessage(uid, subject, body, recips, m)) {
                res.status = 201;
                json r; r["id"] = m.id;
                res.set_content(r.dump(), "application/json");
            } else {
                res.status = 400;
                json err; err["error"] = "Failed to send message";
                res.set_content(err.dump(), "application/json");
            }
        } catch (...) {
            res.status = 400;
            json err; err["error"] = "Invalid request";
            res.set_content(err.dump(), "application/json");
        }
    });
    
    svr.Post("/api/v1/query", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        std::string token = auth.substr(7);
        uint32_t uid;
        if (!authManager_->validateSession(token, uid)) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        try {
            auto j = json::parse(req.body);
            std::string sql = j.value("sql", "");
            query::QueryExecutor exec(storage_);
            auto qr = exec.execute(sql);
            if (!qr.success) {
                res.status = 400;
                json err; err["error"] = qr.message;
                res.set_content(err.dump(), "application/json");
                return;
            }
            json arr = json::array();
            for (const auto& row : qr.rows) {
                json rj = row;
                arr.push_back(rj);
            }
            json result;
            result["rows"] = arr;
            res.set_content(result.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            json err; err["error"] = "Invalid request";
            res.set_content(err.dump(), "application/json");
        }
    });
    
    svr.listen(host_.c_str(), port_);
    return true;
}

void ForgeServer::stop() {
}

} // namespace server
} // namespace forge
