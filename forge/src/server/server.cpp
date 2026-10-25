#include "forge/server/server.h"
#include <iostream>
#include <unordered_set>
#include <httplib.h>
#include <json.hpp>
#include "query/query.h"

using json = nlohmann::json;

namespace forge {
namespace server {

ForgeServer::ForgeServer(const std::string& host, int port, const std::string& dataDir)
    : host_(host), port_(port), dataDir_(dataDir), running_(false) {
}

bool ForgeServer::start() {
    if (!startInBackground()) return false;
    if (serverThread_.joinable()) serverThread_.join();
    return listenerSucceeded_.load();
}

bool ForgeServer::startInBackground() {
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
    
    svr_ = std::make_shared<httplib::Server>();
    
    svr_->Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        json j;
        j["status"] = "ok";
        res.set_content(j.dump(), "application/json");
    });
    
    svr_->Post("/api/v1/users", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            if (!j.is_object()) {
                res.status = 400;
                json err; err["error"] = "Invalid request";
                res.set_content(err.dump(), "application/json");
                return;
            }
            if (!j.contains("name") || !j.contains("email") || !j.contains("password")) {
                res.status = 400;
                json err; err["error"] = "Missing required fields";
                res.set_content(err.dump(), "application/json");
                return;
            }
            if (!j["name"].is_string() || !j["email"].is_string() || !j["password"].is_string()) {
                res.status = 400;
                json err; err["error"] = "Invalid request";
                res.set_content(err.dump(), "application/json");
                return;
            }
            std::string name = j["name"].get<std::string>();
            std::string email = j["email"].get<std::string>();
            std::string password = j["password"].get<std::string>();
            if (name.empty() || name.size() > 63 || email.empty() || email.size() > 127 ||
                email.find('@') == std::string::npos || password.size() < 8 || password.size() > 1024) {
                res.status = 400;
                json err; err["error"] = "Invalid name, email, or password length";
                res.set_content(err.dump(), "application/json");
                return;
            }
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
                res.status = 409;
                json err; err["error"] = "Failed to create user";
                res.set_content(err.dump(), "application/json");
            }
        } catch (...) {
            res.status = 400;
            json err; err["error"] = "Invalid request";
            res.set_content(err.dump(), "application/json");
        }
    });
    
    svr_->Post("/api/v1/auth/login", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            if (!j.is_object() || !j.contains("email") || !j.contains("password") || !j["email"].is_string() || !j["password"].is_string()) {
                res.status = 400;
                json err; err["error"] = "Invalid request";
                res.set_content(err.dump(), "application/json");
                return;
            }
            std::string email = j["email"].get<std::string>();
            std::string password = j["password"].get<std::string>();
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
                    json err; err["error"] = "Invalid credentials";
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
    
    svr_->Post("/api/v1/auth/logout", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0 || auth.size() <= 7) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        const std::string token = auth.substr(7);
        uint32_t userId = 0;
        if (!authManager_->validateSession(token, userId)) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        authManager_->logout(token);
        json response; response["status"] = "ok";
        res.set_content(response.dump(), "application/json");
    });
    
    svr_->Get("/api/v1/users/lookup", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string auth = req.get_header_value("Authorization");
        uint32_t uid = 0;
        if (auth.rfind("Bearer ", 0) != 0 || !authManager_->validateSession(auth.substr(7), uid)) {
            res.status = 401; res.set_content(R"({"error":"Unauthorized"})", "application/json"); return;
        }
        User user{};
        bool found = false;
        if (req.has_param("email")) found = userService_->getUserByEmail(req.get_param_value("email"), user);
        else if (req.has_param("id")) {
            try { found = userService_->getUserById(static_cast<uint32_t>(std::stoul(req.get_param_value("id"))), user); }
            catch (...) { found = false; }
        } else {
            res.status = 400; res.set_content(R"({"error":"email or id query parameter required"})", "application/json"); return;
        }
        if (!found || user.is_deleted) {
            res.status = 404; res.set_content(R"({"error":"User not found"})", "application/json"); return;
        }
        json response; response["id"] = user.id; response["name"] = user.name; response["email"] = user.email;
        res.set_content(response.dump(), "application/json");
    });

    svr_->Get("/api/v1/users/me", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0 || auth.length() <= 7) {
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
    
    svr_->Get("/api/v1/messages", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0 || auth.length() <= 7) {
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
        const bool includeDeleted = req.has_param("include_deleted") && req.get_param_value("include_deleted") == "true";
        auto msgs = messageService_->getMessagesForUser(uid, includeDeleted);
        json arr = json::array();
        for (const auto& m : msgs) {
            json item;
            item["id"] = m.id; item["sender_id"] = m.sender_id; item["subject"] = m.subject; item["body"] = m.body; item["created_at"] = m.created_at; item["is_read"] = (m.is_read != 0); item["is_starred"] = (m.is_starred != 0); item["is_deleted"] = (m.is_deleted != 0); item["is_read"] = (m.is_read != 0); item["is_starred"] = (m.is_starred != 0); item["is_deleted"] = (m.is_deleted != 0);
            arr.push_back(item);
        }
        res.set_content(arr.dump(), "application/json");
    });
    
    svr_->Get(R"(/api/v1/messages/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0 || auth.length() <= 7) {
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
            if (req.matches.size() < 2) {
                res.status = 404;
                json err; err["error"] = "Not found";
                res.set_content(err.dump(), "application/json");
                return;
            }
            uint32_t mid = (uint32_t)std::stoul(req.matches[1]);
            Message m;
            if (!messageService_->getMessageById(mid, m, false)) {
                res.status = 404;
                json err; err["error"] = "Not found";
                res.set_content(err.dump(), "application/json");
                return;
            }
            if (m.sender_id != uid) {
                std::vector<Recipient> recips;
                if (!storage_->readRecipientsByMessage(mid, recips, false)) {
                    res.status = 404;
                    json err; err["error"] = "Not found";
                    res.set_content(err.dump(), "application/json");
                    return;
                }
                bool authorized = false;
                for (const auto& r : recips) {
                    if (r.user_id == uid && r.is_deleted == 0) { authorized = true; break; }
                }
                if (!authorized) {
                    res.status = 404;
                    json err; err["error"] = "Not found";
                    res.set_content(err.dump(), "application/json");
                    return;
                }
            }
            json item;
            item["id"] = m.id; item["sender_id"] = m.sender_id; item["subject"] = m.subject; item["body"] = m.body; item["created_at"] = m.created_at;
            res.set_content(item.dump(), "application/json");
        } catch (...) {
            res.status = 404;
            json err; err["error"] = "Not found";
            res.set_content(err.dump(), "application/json");
        }
    });
    
    svr_->Post("/api/v1/messages", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0 || auth.length() <= 7) {
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
            if (!j.is_object()) {
                res.status = 400;
                json err; err["error"] = "Invalid request";
                res.set_content(err.dump(), "application/json");
                return;
            }
            if (!j.contains("subject") || !j["subject"].is_string() ||
                !j.contains("body") || !j["body"].is_string() ||
                !j.contains("recipients") || !j["recipients"].is_array()) {
                res.status = 400;
                json err; err["error"] = "Invalid request";
                res.set_content(err.dump(), "application/json");
                return;
            }
            const std::string subject = j["subject"].get<std::string>();
            const std::string body = j["body"].get<std::string>();
            if (subject.size() > 127 || body.size() > 511 ||
                j["recipients"].empty() || j["recipients"].size() > 100) {
                res.status = 400;
                json err; err["error"] = "Subject, body, or recipient count exceeds allowed limits";
                res.set_content(err.dump(), "application/json");
                return;
            }
            std::vector<std::pair<uint32_t, RecipientType>> recips;
            std::unordered_set<uint32_t> recipientIds;
            for (const auto& r : j["recipients"]) {
                if (!r.is_object() || !r.contains("user_id") ||
                    (!r["user_id"].is_number_unsigned() && !r["user_id"].is_number_integer())) {
                    res.status = 400;
                    json err; err["error"] = "Invalid recipient";
                    res.set_content(err.dump(), "application/json");
                    return;
                }
                const auto rawId = r["user_id"].get<long long>();
                if (rawId <= 0 || static_cast<unsigned long long>(rawId) > UINT32_MAX) {
                    res.status = 400;
                    json err; err["error"] = "Invalid recipient user_id";
                    res.set_content(err.dump(), "application/json");
                    return;
                }
                if (!recipientIds.insert(static_cast<uint32_t>(rawId)).second) {
                    res.status = 400;
                    json err; err["error"] = "Duplicate recipient user_id";
                    res.set_content(err.dump(), "application/json");
                    return;
                }
                if (r.contains("type") && !r["type"].is_string()) {
                    res.status = 400;
                    json err; err["error"] = "Invalid recipient type";
                    res.set_content(err.dump(), "application/json");
                    return;
                }
                const std::string t = r.value("type", "TO");
                if (t == "TO") recips.push_back({static_cast<uint32_t>(rawId), RecipientType::TO});
                else if (t == "CC") recips.push_back({static_cast<uint32_t>(rawId), RecipientType::CC});
                else if (t == "BCC") recips.push_back({static_cast<uint32_t>(rawId), RecipientType::BCC});
                else {
                    res.status = 400;
                    json err; err["error"] = "Unknown recipient type";
                    res.set_content(err.dump(), "application/json");
                    return;
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
    
    svr_->Delete(R"(/api/v1/messages/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0 || auth.length() <= 7) {
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
            if (req.matches.size() < 2) {
                res.status = 404;
                json err; err["error"] = "Not found";
                res.set_content(err.dump(), "application/json");
                return;
            }
            uint32_t mid = (uint32_t)std::stoul(req.matches[1]);
            Message m;
            if (!messageService_->getMessageById(mid, m, false)) {
                res.status = 404;
                json err; err["error"] = "Not found";
                res.set_content(err.dump(), "application/json");
                return;
            }
            if (m.sender_id != uid) {
                res.status = 403;
                json err; err["error"] = "Forbidden";
                res.set_content(err.dump(), "application/json");
                return;
            }
            if (messageService_->deleteMessage(mid)) {
                json r; r["status"] = "ok";
                res.set_content(r.dump(), "application/json");
            } else {
                res.status = 500;
                json err; err["error"] = "Failed";
                res.set_content(err.dump(), "application/json");
            }
        } catch (...) {
            res.status = 404;
            json err; err["error"] = "Not found";
            res.set_content(err.dump(), "application/json");
        }
    });
    
    svr_->Get("/api/v1/schema/tables", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string auth = req.get_header_value("Authorization");
        uint32_t uid = 0;
        if (auth.find("Bearer ") != 0 || auth.size() <= 7 ||
            !authManager_->validateSession(auth.substr(7), uid)) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        json response;
        response["tables"] = {"users", "messages", "recipients"};
        res.set_content(response.dump(), "application/json");
    });

    svr_->Get(R"(/api/v1/schema/(users|messages|recipients))", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string auth = req.get_header_value("Authorization");
        uint32_t uid = 0;
        if (auth.find("Bearer ") != 0 || auth.size() <= 7 ||
            !authManager_->validateSession(auth.substr(7), uid)) {
            res.status = 401;
            json err; err["error"] = "Unauthorized";
            res.set_content(err.dump(), "application/json");
            return;
        }
        if (req.matches.size() < 2) {
            res.status = 404;
            json err; err["error"] = "Unknown table";
            res.set_content(err.dump(), "application/json");
            return;
        }
        const std::string table = req.matches[1];
        json fields = json::array();
        auto addField = [&](const char* name, const char* type) {
            fields.push_back({{"field", name}, {"type", type}, {"nullable", false}});
        };
        if (table == "users") {
            addField("id", "uint32"); addField("name", "char[64]");
            addField("email", "char[128]"); addField("created_at", "uint64");
        } else if (table == "messages") {
            addField("id", "uint32"); addField("sender_id", "uint32");
            addField("subject", "char[128]"); addField("body", "char[512]");
            addField("created_at", "uint64");
        } else if (table == "recipients") {
            addField("message_id", "uint32"); addField("user_id", "uint32");
            addField("type", "char[4]");
        } else {
            res.status = 404;
            json err; err["error"] = "Unknown table";
            res.set_content(err.dump(), "application/json");
            return;
        }
        json response;
        response["table"] = table;
        response["fields"] = std::move(fields);
        res.set_content(response.dump(), "application/json");
    });

    // Authenticated mailbox state mutation endpoints used by the FastAPI adapter.
    // The current FORGE record layout stores read/star/trash flags on Message, so
    // these flags are shared by all recipients until per-recipient state is modeled.
    svr_->Patch(R"(/api/v1/messages/(\d+)/state)", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string auth = req.get_header_value("Authorization");
        uint32_t uid = 0;
        if (auth.rfind("Bearer ", 0) != 0 || !authManager_->validateSession(auth.substr(7), uid)) {
            res.status = 401; res.set_content(R"({"error":"Unauthorized"})", "application/json"); return;
        }
        try {
            const auto body = json::parse(req.body);
            if (!body.is_object() || body.size() != 1 ||
                (!body.contains("is_read") && !body.contains("is_starred"))) {
                res.status = 400; res.set_content(R"({"error":"Expected is_read or is_starred boolean"})", "application/json"); return;
            }
            const std::string field = body.contains("is_read") ? "is_read" : "is_starred";
            if (!body[field].is_boolean()) {
                res.status = 400; res.set_content(R"({"error":"State value must be boolean"})", "application/json"); return;
            }
            const uint32_t id = static_cast<uint32_t>(std::stoul(req.matches[1]));
            Message message{};
            if (!messageService_->getMessageById(id, message, false)) {
                res.status = 404; res.set_content(R"({"error":"Message not found"})", "application/json"); return;
            }
            bool authorized = message.sender_id == uid;
            std::vector<Recipient> recipients;
            if (storage_->readRecipientsByMessage(id, recipients, false)) {
                for (const auto& recipient : recipients) if (recipient.user_id == uid) authorized = true;
            }
            if (!authorized) {
                res.status = 404; res.set_content(R"({"error":"Message not found"})", "application/json"); return;
            }
            if (field == "is_read") message.is_read = body[field].get<bool>() ? 1 : 0;
            else message.is_starred = body[field].get<bool>() ? 1 : 0;
            if (!messageService_->updateMessage(message)) {
                res.status = 500; res.set_content(R"({"error":"Could not update message state"})", "application/json"); return;
            }
            res.set_content(R"({"status":"ok"})", "application/json");
        } catch (...) {
            res.status = 400; res.set_content(R"({"error":"Invalid request"})", "application/json");
        }
    });

    svr_->Patch(R"(/api/v1/messages/(\d+)/mailbox)", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string auth = req.get_header_value("Authorization");
        uint32_t uid = 0;
        if (auth.rfind("Bearer ", 0) != 0 || !authManager_->validateSession(auth.substr(7), uid)) {
            res.status = 401; res.set_content(R"({"error":"Unauthorized"})", "application/json"); return;
        }
        try {
            const auto body = json::parse(req.body);
            if (!body.is_object() || !body.contains("is_deleted") || !body["is_deleted"].is_boolean()) {
                res.status = 400; res.set_content(R"({"error":"Expected boolean is_deleted"})", "application/json"); return;
            }
            const uint32_t id = static_cast<uint32_t>(std::stoul(req.matches[1]));
            Message message{};
            if (!messageService_->getMessageById(id, message, true)) {
                res.status = 404; res.set_content(R"({"error":"Message not found"})", "application/json"); return;
            }
            bool authorized = message.sender_id == uid;
            std::vector<Recipient> recipients;
            if (storage_->readRecipientsByMessage(id, recipients, false)) {
                for (const auto& recipient : recipients) if (recipient.user_id == uid) authorized = true;
            }
            if (!authorized) {
                res.status = 404; res.set_content(R"({"error":"Message not found"})", "application/json"); return;
            }
            message.is_deleted = body["is_deleted"].get<bool>() ? 1 : 0;
            if (!messageService_->updateMessage(message)) {
                res.status = 500; res.set_content(R"({"error":"Could not update mailbox state"})", "application/json"); return;
            }
            res.set_content(R"({"status":"ok"})", "application/json");
        } catch (...) {
            res.status = 400; res.set_content(R"({"error":"Invalid request"})", "application/json");
        }
    });

    svr_->Delete(R"(/api/v1/messages/(\d+)/mailbox)", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string auth = req.get_header_value("Authorization");
        uint32_t uid = 0;
        if (auth.rfind("Bearer ", 0) != 0 || !authManager_->validateSession(auth.substr(7), uid)) {
            res.status = 401; res.set_content(R"({"error":"Unauthorized"})", "application/json"); return;
        }
        try {
            const uint32_t id = static_cast<uint32_t>(std::stoul(req.matches[1]));
            Message message{};
            if (!messageService_->getMessageById(id, message, false)) {
                res.status = 404; res.set_content(R"({"error":"Message not found"})", "application/json"); return;
            }
            bool authorized = message.sender_id == uid;
            std::vector<Recipient> recipients;
            if (storage_->readRecipientsByMessage(id, recipients, false)) {
                for (const auto& recipient : recipients) if (recipient.user_id == uid) authorized = true;
            }
            if (!authorized) {
                res.status = 404; res.set_content(R"({"error":"Message not found"})", "application/json"); return;
            }
            if (!messageService_->deleteMessage(id)) {
                res.status = 500; res.set_content(R"({"error":"Could not permanently delete message"})", "application/json"); return;
            }
            res.set_content(R"({"status":"ok"})", "application/json");
        } catch (...) {
            res.status = 400; res.set_content(R"({"error":"Invalid request"})", "application/json");
        }
    });

    svr_->Post("/api/v1/query", [&](const httplib::Request& req, httplib::Response& res) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.find("Bearer ") != 0 || auth.length() <= 7) {
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
            const auto body = json::parse(req.body);
            if (!body.is_object() || !body.contains("query") || !body["query"].is_string()) {
                res.status = 400;
                json err; err["error"] = "Expected a JSON object with a string query field";
                res.set_content(err.dump(), "application/json");
                return;
            }
            const std::string sql = body["query"].get<std::string>();
            if (sql.empty() || sql.size() > 4096) {
                res.status = 400;
                json err; err["error"] = "Query must be between 1 and 4096 bytes";
                res.set_content(err.dump(), "application/json");
                return;
            }
            query::QueryExecutor executor(storage_);
            const auto queryResult = executor.execute(sql, uid);
            if (!queryResult.success) {
                res.status = 400;
                json err; err["error"] = queryResult.message;
                res.set_content(err.dump(), "application/json");
                return;
            }
            json response;
            response["rows"] = json::array();
            for (const auto& row : queryResult.rows) {
                json item = json::object();
                for (const auto& column : row) item[column.first] = column.second;
                response["rows"].push_back(std::move(item));
            }
            response["row_count"] = queryResult.rows.size();
            res.status = 200;
            res.set_content(response.dump(), "application/json");
        } catch (const json::exception&) {
            res.status = 400;
            json err; err["error"] = "Invalid JSON request";
            res.set_content(err.dump(), "application/json");
        } catch (...) {
            res.status = 500;
            json err; err["error"] = "Query failed";
            res.set_content(err.dump(), "application/json");
        }
    });
    
    // Bind synchronously so startInBackground() reports address/port errors to its caller.
    if (!svr_->bind_to_port(host_.c_str(), port_)) {
        running_ = false;
        storage_->close();
        svr_.reset();
        return false;
    }
    listenerSucceeded_ = false;
    running_ = true;
    serverThread_ = std::thread([this]() {
        listenerSucceeded_ = svr_->listen_after_bind();
        running_ = false;
    });
    return true;
}

void ForgeServer::stop() {
    if (svr_) {
        svr_->stop();
    }
    running_ = false;
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    if (storage_) {
        storage_->close();
    }
}

bool ForgeServer::isRunning() const {
    return running_;
}

} // namespace server
} // namespace forge
