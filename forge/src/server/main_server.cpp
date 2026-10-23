#include "forge/server/server.h"
#include <iostream>

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 8080;
    std::string dataDir = "./forge/data";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) host = argv[++i];
        else if (arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
        else if (arg == "--data-dir" && i + 1 < argc) dataDir = argv[++i];
    }
    forge::server::ForgeServer server(host, port, dataDir);
    if (!server.start()) {
        return 1;
    }
    return 0;
}
