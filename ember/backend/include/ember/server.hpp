#pragma once

#include "ember/forge_client.hpp"
#include <httplib.h>
#include <memory>
#include <string>

namespace ember {
class Server {
public:
    Server(std::string host, int port, std::string forge_url);
    bool start();
private:
    std::string host_;
    int port_;
    ForgeClient forge_;
    httplib::Server server_;
    void register_routes();
};
}
