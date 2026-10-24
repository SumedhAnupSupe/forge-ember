#include "forge/cli/cli.h"
#include <iostream>
#include <string>
#include <httplib.h>
#include <json.hpp>
#include <vector>
#include <algorithm>
#include <cctype>

using json = nlohmann::json;

ForgeCLI::ForgeCLI(const std::string& host, int port) : host_(host), port_(port) {
}

void printTable(const std::vector<std::map<std::string, std::string>>& rows) {
    if (rows.empty()) {
        std::cout << "Empty set" << std::endl;
        return;
    }
    std::vector<std::string> cols;
    for (const auto& p : rows[0]) cols.push_back(p.first);
    std::vector<size_t> widths(cols.size(), 0);
    for (size_t i = 0; i < cols.size(); ++i) widths[i] = cols[i].size();
    for (const auto& row : rows) {
        for (size_t i = 0; i < cols.size(); ++i) {
            if (row.find(cols[i]) != row.end()) {
                if (row.at(cols[i]).size() > widths[i]) {
                    if (row.at(cols[i]).size() > 80) widths[i] = 80;
                    else widths[i] = row.at(cols[i]).size();
                }
            }
        }
    }
    // print header
    std::cout << "+";
    for (size_t i = 0; i < widths.size(); ++i) { for (size_t j = 0; j < widths[i]+2; ++j) std::cout << "-"; std::cout << "+"; }
    std::cout << std::endl;
    std::cout << "|";
    for (size_t i = 0; i < widths.size(); ++i) { std::cout << " " << cols[i]; for (size_t j = cols[i].size(); j < widths[i]; ++j) std::cout << " "; std::cout << " |"; }
    std::cout << std::endl;
    std::cout << "+";
    for (size_t i = 0; i < widths.size(); ++i) { for (size_t j = 0; j < widths[i]+2; ++j) std::cout << "-"; std::cout << "+"; }
    std::cout << std::endl;
    for (const auto& row : rows) {
        std::cout << "|";
        for (size_t i = 0; i < widths.size(); ++i) {
            std::string val = row.find(cols[i]) != row.end() ? row.at(cols[i]) : "";
            if (val.size() > widths[i]) val = val.substr(0, widths[i]);
            std::cout << " " << val;
            for (size_t j = val.size(); j < widths[i]; ++j) std::cout << " ";
            std::cout << " |";
        }
        std::cout << std::endl;
    }
    std::cout << "+";
    for (size_t i = 0; i < widths.size(); ++i) { for (size_t j = 0; j < widths[i]+2; ++j) std::cout << "-"; std::cout << "+"; }
    std::cout << std::endl;
    std::cout << rows.size() << " rows in set" << std::endl;
}

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

void ForgeCLI::run() {
    std::cout << "FORGE Database Console" << std::endl;
    std::cout << "Connected to " << host_ << ":" << port_ << std::endl << std::endl;
    std::string line;
    while (true) {
        if (!token_.empty()) std::cout << "forge [logged in]> ";
        else std::cout << "forge> ";
        std::getline(std::cin, line);
        if (std::cin.eof()) break;
        if (line.empty()) continue;
        std::string cmd = trim(line);
        if (cmd == "exit" || cmd == "quit") break;
        if (cmd == "help") {
            std::cout << "Commands: help, status, login, logout, whoami, SHOW TABLES;, DESCRIBE <table>;, SELECT ..., exit, quit\n" << std::endl;
            continue;
        }
        if (cmd == "status") {
            try {
                httplib::Client cli(host_.c_str(), port_);
                cli.set_connection_timeout(2);
                auto res = cli.Get("/health");
                if (res && res->status == 200) std::cout << "Server OK" << std::endl;
                else std::cout << "Server unreachable" << std::endl;
            } catch (...) {
                std::cout << "Server unreachable" << std::endl;
            }
            continue;
        }
        if (cmd == "login") {
            if (token_.empty()) {
                std::string email, pass;
                std::cout << "Email: "; std::getline(std::cin, email); email = trim(email);
                std::cout << "Password: "; 
#ifdef _WIN32
                std::getline(std::cin, pass);
#else
                // Try to disable echo if possible - simple approach
                std::getline(std::cin, pass);
#endif
                pass = trim(pass);
                try {
                    httplib::Client cli(host_.c_str(), port_);
                    cli.set_connection_timeout(5);
                    json j = {{"email", email}, {"password", pass}};
                    auto res = cli.Post("/api/v1/auth/login", j.dump(), "application/json");
                    if (res && res->status == 200) {
                        try {
                            auto rj = json::parse(res->body);
                            token_ = rj.value("token", "");
                            if (!token_.empty()) std::cout << "Login successful" << std::endl;
                            else std::cout << "Login failed" << std::endl;
                        } catch (...) {
                            std::cout << "Login failed" << std::endl;
                        }
                    } else {
                        std::cout << "Login failed" << std::endl;
                    }
                } catch (...) {
                    std::cout << "Connection error" << std::endl;
                }
            } else {
                std::cout << "Already logged in" << std::endl;
            }
            continue;
        }
        if (cmd == "logout") {
            try {
                httplib::Client cli(host_.c_str(), port_);
                httplib::Headers h;
                if (!token_.empty()) h.emplace("Authorization", "Bearer " + token_);
                cli.Post("/api/v1/auth/logout", h, "", "");
            } catch (...) {}
            token_ = "";
            std::cout << "Logged out" << std::endl;
            continue;
        }
        if (cmd == "whoami") {
            if (token_.empty()) { std::cout << "Not logged in" << std::endl; continue; }
            try {
                httplib::Client cli(host_.c_str(), port_);
                httplib::Headers h = {{"Authorization", "Bearer " + token_}};
                auto res = cli.Get("/api/v1/users/me", h);
                if (res && res->status == 200) std::cout << res->body << std::endl;
                else if (res && res->status == 401) { std::cout << "Session expired or invalid" << std::endl; token_ = ""; }
                else std::cout << "Failed" << std::endl;
            } catch (...) {
                std::cout << "Connection error" << std::endl;
            }
            continue;
        }
        // Case insensitive
        std::string upper = cmd;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        if (upper == "SHOW TABLES;" || upper == "SHOW TABLES") {
            std::cout << "+-----------+" << std::endl;
            std::cout << "| Tables    |" << std::endl;
            std::cout << "+-----------+" << std::endl;
            std::cout << "| users     |" << std::endl;
            std::cout << "| messages  |" << std::endl;
            std::cout << "| recipients|" << std::endl;
            std::cout << "+-----------+" << std::endl;
            continue;
        }
        if (upper.find("DESCRIBE ") == 0) {
            std::string rest = upper.substr(9);
            // remove semicolon
            while (!rest.empty() && (rest.back() == ';' || isspace((unsigned char)rest.back()))) rest.pop_back();
            if (rest == "USERS") {
                std::cout << "+------------+----------+------+" << std::endl;
                std::cout << "| Field      | Type     | Null |" << std::endl;
                std::cout << "+------------+----------+------+" << std::endl;
                std::cout << "| id         | uint32   | NO   |" << std::endl;
                std::cout << "| name       | char[64] | NO   |" << std::endl;
                std::cout << "| email      | char[128]| NO   |" << std::endl;
                std::cout << "| created_at | uint64   | NO   |" << std::endl;
                std::cout << "+------------+----------+------+" << std::endl;
            } else if (rest == "MESSAGES") {
                std::cout << "+-----------+----------+------+" << std::endl;
                std::cout << "| Field     | Type     | Null |" << std::endl;
                std::cout << "+-----------+----------+------+" << std::endl;
                std::cout << "| id        | uint32   | NO   |" << std::endl;
                std::cout << "| sender_id | uint32   | NO   |" << std::endl;
                std::cout << "| subject   | char[128]| NO   |" << std::endl;
                std::cout << "| body      | char[512]| NO   |" << std::endl;
                std::cout << "| created_at| uint64   | NO   |" << std::endl;
                std::cout << "+-----------+----------+------+" << std::endl;
            } else if (rest == "RECIPIENTS") {
                std::cout << "+-----------+---------+------+" << std::endl;
                std::cout << "| Field     | Type    | Null |" << std::endl;
                std::cout << "+-----------+---------+------+" << std::endl;
                std::cout << "| message_id| uint32  | NO   |" << std::endl;
                std::cout << "| user_id   | uint32  | NO   |" << std::endl;
                std::cout << "| type      | char[4] | NO   |" << std::endl;
                std::cout << "+-----------+---------+------+" << std::endl;
            } else {
                std::cout << "Unknown table" << std::endl;
            }
            continue;
        }
        if (upper.find("SELECT") == 0) {
            if (token_.empty()) { std::cout << "Unauthorized" << std::endl; continue; }
            try {
                httplib::Client cli(host_.c_str(), port_);
                cli.set_connection_timeout(5);
                httplib::Headers h = {{"Authorization", "Bearer " + token_}};
                json j = {{"sql", cmd}};
                auto res = cli.Post("/api/v1/query", h, j.dump(), "application/json");
                if (res && res->status == 200) {
                    try {
                        auto rj = json::parse(res->body);
                        if (rj.contains("rows") && rj["rows"].is_array()) {
                            std::vector<std::map<std::string, std::string>> rows;
                            for (const auto& rr : rj["rows"]) {
                                std::map<std::string, std::string> m;
                                for (auto it = rr.begin(); it != rr.end(); ++it) {
                                    if (it->is_string()) m[it.key()] = it->get<std::string>();
                                    else m[it.key()] = it->dump();
                                }
                                rows.push_back(m);
                            }
                            printTable(rows);
                        } else {
                            std::cout << res->body << std::endl;
                        }
                    } catch (...) {
                        std::cout << res->body << std::endl;
                    }
                } else if (res) {
                    if (res->status == 401) { std::cout << "Session expired or invalid" << std::endl; token_ = ""; continue; }
                    if (res->status == 403) { std::cout << "Error: " << res->body << std::endl; continue; }
                    std::cout << "Error: " << res->body << std::endl;
                } else {
                    std::cout << "Connection error" << std::endl;
                }
            } catch (...) {
                std::cout << "Connection error" << std::endl;
            }
            continue;
        }
        std::cout << "Unknown command. Type 'help'." << std::endl;
    }
}
