#include "ember/server.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    const char* host_env = std::getenv("EMBER_HOST");
    const char* port_env = std::getenv("EMBER_PORT");
    const char* forge_env = std::getenv("FORGE_ORIGIN");
    const std::string host = host_env ? host_env : "127.0.0.1";
    const int port = port_env ? std::stoi(port_env) : 8081;
    const std::string forge_origin = forge_env ? forge_env : "http://127.0.0.1:8080";
    ember::EmberServer server(host, port, forge_origin);
    if (!server.start()) {
        std::cerr << "EMBER failed to start on " << host << ":" << port << "\n";
        return 1;
    }
    return 0;
}
