#include "forge/cli/cli.h"

#include <iostream>
#include <string>

namespace {
void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [--host HOST] [--port PORT]\n"
              << "       " << program << " --help\n"
              << "Options:\n"
              << "  --host HOST  FORGE server hostname (default: 127.0.0.1)\n"
              << "  --port PORT  FORGE server port, 1-65535 (default: 8080)\n"
              << "  --help       Show this help and exit\n";
}
}

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--host") {
            if (i + 1 >= argc || argv[i + 1][0] == '\0' ||
                std::string(argv[i + 1]).rfind("--", 0) == 0) {
                std::cerr << "Error: --host requires a hostname\n";
                printUsage(argv[0]);
                return 2;
            }
            host = argv[++i];
        } else if (arg == "--port") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --port requires a number\n";
                printUsage(argv[0]);
                return 2;
            }
            try {
                const std::string value = argv[++i];
                size_t parsed = 0;
                const int parsedPort = std::stoi(value, &parsed);
                if (parsed != value.size() || parsedPort < 1 || parsedPort > 65535) {
                    throw std::out_of_range("port");
                }
                port = parsedPort;
            } catch (...) {
                std::cerr << "Error: port must be an integer from 1 to 65535\n";
                printUsage(argv[0]);
                return 2;
            }
        } else {
            std::cerr << "Error: unknown option " << arg << "\n";
            printUsage(argv[0]);
            return 2;
        }
    }

    ForgeCLI cli(host, port);
    cli.run();
    return 0;
}
