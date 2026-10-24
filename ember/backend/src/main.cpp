#include "ember/server.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
std::string env_or(const char* key, const char* fallback) {
    const char* value = std::getenv(key);
    return value && *value ? value : fallback;
}
int env_port(const char* key, int fallback) {
    const auto value = env_or(key, "");
    if (value.empty()) return fallback;
    try { int port = std::stoi(value); if (port > 0 && port <= 65535) return port; }
    catch (...) {}
    throw std::runtime_error(std::string("Invalid port in ") + key);
}
}
int main() {
    try {
        ember::Server server(env_or("EMBER_HOST", "127.0.0.1"),
                             env_port("EMBER_PORT", 8090),
                             env_or("FORGE_URL", "http://127.0.0.1:8080"));
        return server.start() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "EMBER startup failed: " << error.what() << '\n';
        return 1;
    }
}
