#pragma once
#include <string>
#include <httplib.h>
#include "ember/forge_client.hpp"

namespace ember {
class EmberServer {
public:
    EmberServer(std::string host, int port, std::string forge_origin);
    bool start();
private:
    std::string host_;
    int port_;
    ForgeClient forge_;
    httplib::Server server_;
    void configureRoutes();
};
}
