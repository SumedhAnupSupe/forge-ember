#include "forge/cli/cli.h"
#include <iostream>
#include <string>
#include <httplib.h>
#include <json.hpp>
#include <vector>
#include <algorithm>

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
            if (row.find(cols[i]) != row.end() && row.at(cols[i]).size() > widths[i]) {
                widths[i] = row.at(cols[i]).size();
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
            std::cout << " " << val;
            for (size_t j = val.size(); j < widths[i]; ++j) std::cout << " ";
            std::cout << " |";
        }
        std::cout << std::endl;
    }
    std::cout << "+";
    for (size_t i = 0; i < widths.size(); ++i) { for (size_t j = 0; j < widths[i]+2; ++j) std::cout << "-"; std::cout << "+"; }
    std::cout << std::endl;
    std::cout << rows.size() << " rows returned" << std::endl;
}

void ForgeCLI::run() {
    std::cout << "FORGE Database Console" << std::endl;
    std::cout << "Connected to " << host_ << ":" << port_ << std::endl << std::endl;
    std::string line;
    while (true) {
        if (!token_.empty()) std::cout << "forge [logged in]> ";
        else std::cout << "forge> ";
        std::getline(std::cin, line);
        if (line.empty()) continue;
        // trim
        while (!line.empty() && isspace((unsigned char)line.front())) line.erase(line.begin());
        while (!line.empty() && isspace((unsigned char)line.back())) line.pop_back();
        if (line == "exit" || line == "quit") break;
        if (line == "help") {
            std::cout << "Commands: help, status, login, logout, whoami, exit, quit\n" << std::endl;
            continue;
        }
        if (line == "status") {
            httplib::Client cli(host_.c_str(), port_);
            auto res = cli.Get("/health");
            if (res && res->status == 200) std::cout << "Server OK" << std::endl;
            else std::cout << "Server unreachable" << std::endl;
            continue;
        }
        if (line == "login") {
            std::string email, pass;
            std::cout << "Email: "; std::getline(std::cin, email);
            std::cout << "Password: "; std::getline(std::cin, pass);
            httplib::Client cli(host_.c_str(), port_);
            json j = {{"email", email}, {"password", pass}};
            auto res = cli.Post("/api/v1/auth/login", j.dump(), "application/json");
            if (res && res->status == 200) {
                auto rj = json::parse(res->body);
                token_ = rj.value("token", "");
                std::cout << "Login successful" << std::endl;
            } else {
                std::cout << "Login failed" << std::endl;
            }
            continue;
        }
        if (line == "logout") {
            httplib::Client cli(host_.c_str(), port_);
            httplib::Headers h = {{"Authorization", "Bearer " + token_}};
            auto res = cli.Post("/api/v1/auth/logout", h, "", "");
            token_ = "";
            std::cout << "Logged out" << std::endl;
            continue;
        }
        if (line == "whoami") {
            if (token_.empty()) { std::cout << "Not logged in" << std::endl; continue; }
            httplib::Client cli(host_.c_str(), port_);
            httplib::Headers h = {{"Authorization", "Bearer " + token_}};
            auto res = cli.Get("/api/v1/users/me", h);
            if (res && res->status == 200) std::cout << res->body << std::endl;
            else std::cout << "Failed" << std::endl;
            continue;
        }
        if (line == "SHOW TABLES;" || line == "show tables;") {
            std::cout << "+-----------+" << std::endl;
            std::cout << "| Tables    |" << std::endl;
            std::cout << "+-----------+" << std::endl;
            std::cout << "| users     |" << std::endl;
            std::cout << "| messages  |" << std::endl;
            std::cout << "| recipients|" << std::endl;
            std::cout << "+-----------+" << std::endl;
            continue;
        }
        if (line.find("SELECT") == 0 || line.find("select") == 0) {
            if (token_.empty()) { std::cout << "Unauthorized" << std::endl; continue; }
            httplib::Client cli(host_.c_str(), port_);
            httplib::Headers h = {{"Authorization", "Bearer " + token_}};
            json j = {{"sql", line}};
            auto res = cli.Post("/api/v1/query", h, j.dump(), "application/json");
            if (res && res->status == 200) {
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
            } else if (res) {
                std::cout << "Error: " << res->body << std::endl;
            } else {
                std::cout << "Connection error" << std::endl;
            }
            continue;
        }
        std::cout << "Unknown command. Type 'help'." << std::endl;
    }
}
