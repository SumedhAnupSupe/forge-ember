#include <iostream>
#include "storage/storage.h"
#include "services/user_service.h"
#include "services/message_service.h"
#include "index/indexes.h"
#include "algorithms/benchmark.h"
#include "auth/auth.h"
#include "query/query.h"

using namespace forge;

int main() {
    std::cout << "FORGE Database Engine" << std::endl;
    std::cout << "=====================" << std::endl;
    
    auto storage = std::make_shared<Storage>("forge/data");
    if (!storage->open()) {
        std::cerr << "Failed to open storage" << std::endl;
        return 1;
    }
    
    auto indexManager = std::make_shared<IndexManager>(storage);
    indexManager->buildIndexes();
    
    auto userService = std::make_shared<UserService>(storage, indexManager);
    auto messageService = std::make_shared<MessageService>(storage, indexManager);
    auto authManager = std::make_shared<AuthManager>();
    
    algorithms::Benchmark::runAllBenchmarks();
    
    storage->close();
    std::cout << "\nFORGE initialized successfully!" << std::endl;
    return 0;
}
