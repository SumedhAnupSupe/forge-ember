#include "forge/cli/cli.h"
#include <iostream>

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) host = argv[++i];
        else if (arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
    }
    ForgeCLI cli(host, port);
    cli.run();
    return 0;
}
