#include "forge/cli/cli.h"
#include <iostream>
#include <string>
#include <httplib.h>
#include <json.hpp>
#include <vector>
#include <map>
#include <utility>
#include <exception>
#include <algorithm>
#include <cctype>
#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using json = nlohmann::json;

ForgeCLI::ForgeCLI(const std::string& host, int port) : host_(host), port_(port) {
}

static void setEcho(bool enable) {
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    if (!enable) mode &= ~ENABLE_ECHO_INPUT;
    else mode |= ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdin, mode);
#else
    struct termios tty;
    if (tcgetattr(STDIN_FILENO, &tty) != 0) return;
    if (!enable) tty.c_lflag &= ~ECHO;
    else tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
#endif
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
            std::string email, pass;
            std::cout << "Email: "; std::getline(std::cin, email); email = trim(email);
            std::cout << "Password: ";
            setEcho(false);
            std::getline(std::cin, pass);
            setEcho(true);
            std::cout << std::endl;
            // Do NOT trim password
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
                } else if (res) {
                    if (res->status == 401) std::cout << "Login failed" << std::endl;
                    else std::cout << "Login failed" << std::endl;
                } else {
                    std::cout << "Connection error" << std::endl;
                }
            } catch (...) {
                std::cout << "Connection error" << std::endl;
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
        std::string upper = cmd;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        if (upper == "SHOW TABLES;" || upper == "SHOW TABLES") {
            if (token_.empty()) { std::cout << "Unauthorized: please login first." << std::endl; continue; }
            try {
                httplib::Client cli(host_.c_str(), port_);
                cli.set_connection_timeout(3);
                httplib::Headers headers = {{"Authorization", "Bearer " + token_}};
                auto res = cli.Get("/api/v1/schema/tables", headers);
                if (!res) { std::cout << "Connection error." << std::endl; continue; }
                if (res->status == 401) { token_.clear(); std::cout << "Session expired or invalid. Please login again." << std::endl; continue; }
                if (res->status != 200) { std::cout << "Failed to fetch schema." << std::endl; continue; }
                const json response = json::parse(res->body);
                if (!response.contains("tables") || !response["tables"].is_array()) {
                    std::cout << "Invalid schema response." << std::endl; continue;
                }
                std::vector<std::map<std::string, std::string>> rows;
                for (const auto& table : response["tables"]) {
                    if (table.is_string()) rows.push_back({{"table", table.get<std::string>()}});
                }
                printTable(rows);
            } catch (const std::exception&) { std::cout << "Invalid schema response." << std::endl; }
            continue;
        }
        if (upper.find("DESCRIBE ") == 0) {
            if (token_.empty()) { std::cout << "Unauthorized: please login first." << std::endl; continue; }
            std::string table = trim(cmd.substr(9));
            while (!table.empty() && (table.back() == ';' || isspace((unsigned char)table.back()))) table.pop_back();
            std::transform(table.begin(), table.end(), table.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (table != "users" && table != "messages" && table != "recipients") {
                std::cout << "Unknown table" << std::endl; continue;
            }
            try {
                httplib::Client cli(host_.c_str(), port_);
                cli.set_connection_timeout(3);
                httplib::Headers headers = {{"Authorization", "Bearer " + token_}};
                auto res = cli.Get(("/api/v1/schema/" + table).c_str(), headers);
                if (!res) { std::cout << "Connection error." << std::endl; continue; }
                if (res->status == 401) { token_.clear(); std::cout << "Session expired or invalid. Please login again." << std::endl; continue; }
                if (res->status != 200) { std::cout << "Failed to fetch schema." << std::endl; continue; }
                const json response = json::parse(res->body);
                if (!response.contains("fields") || !response["fields"].is_array()) {
                    std::cout << "Invalid schema response." << std::endl; continue;
                }
                std::vector<std::map<std::string, std::string>> rows;
                for (const auto& field : response["fields"]) {
                    if (!field.is_object()) continue;
                    rows.push_back({
                        {"Field", field.value("field", "")},
                        {"Type", field.value("type", "")},
                        {"Null", field.value("nullable", true) ? "YES" : "NO"}
                    });
                }
                printTable(rows);
            } catch (const std::exception&) { std::cout << "Invalid schema response." << std::endl; }
            continue;
        }
        if (upper.find("SELECT") == 0) {
            if (token_.empty()) { std::cout << "Unauthorized: please login first." << std::endl; continue; }
            try {
                httplib::Client cli(host_.c_str(), port_);
                cli.set_connection_timeout(3);
                cli.set_read_timeout(10);
                httplib::Headers headers = {{"Authorization", "Bearer " + token_}};
                json request = {{"query", cmd}};
                auto res = cli.Post("/api/v1/query", headers, request.dump(), "application/json");
                if (!res) {
                    std::cout << "Connection error while executing query." << std::endl;
                    continue;
                }
                if (res->status == 401) {
                    token_.clear();
                    std::cout << "Session expired or invalid. Please login again." << std::endl;
                    continue;
                }
                json response = json::parse(res->body);
                if (res->status != 200) {
                    std::cout << "Query error: " << response.value("error", "Request failed") << std::endl;
                    continue;
                }
                if (!response.contains("rows") || !response["rows"].is_array()) {
                    std::cout << "Invalid response from server." << std::endl;
                    continue;
                }
                std::vector<std::map<std::string, std::string>> rows;
                for (const auto& item : response["rows"]) {
                    if (!item.is_object()) continue;
                    std::map<std::string, std::string> row;
                    for (auto it = item.begin(); it != item.end(); ++it) {
                        row[it.key()] = it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
                    }
                    rows.push_back(std::move(row));
                }
                if (rows.empty()) std::cout << "Empty set (0 rows)" << std::endl;
                else printTable(rows);
            } catch (const std::exception&) {
                std::cout << "Invalid response from server." << std::endl;
            }
            continue;
        }
        std::cout << "Unknown command. Type 'help'." << std::endl;
    }
}
