#ifndef FORGE_CLI_H
#define FORGE_CLI_H

#include <string>

class ForgeCLI {
public:
    ForgeCLI(const std::string& host, int port);
    void run();
private:
    std::string host_;
    int port_;
    std::string token_;
};

#endif
