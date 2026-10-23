#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <vector>
#include <chrono>
#include <string>
#include <functional>

namespace forge {
namespace algorithms {

class Benchmark {
public:
    struct Result {
        std::string algorithmName;
        double averageTimeMicroseconds;
        bool correct;
    };
    
    static Result benchmarkSort(const std::string& name, 
                               std::function<void(std::vector<int>&)> sortFunc,
                               std::vector<int> baseInput);
    
    static void runAllBenchmarks();
};

} // namespace algorithms
} // namespace forge

#endif // BENCHMARK_H
