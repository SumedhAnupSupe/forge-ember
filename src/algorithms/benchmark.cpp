#include "algorithms/benchmark.h"
#include "algorithms/sorting.h"
#include <iostream>
#include <algorithm>
#include <numeric>

namespace forge {
namespace algorithms {

Benchmark::Result Benchmark::benchmarkSort(const std::string& name,
                                         std::function<void(std::vector<int>&)> sortFunc,
                                         std::vector<int> baseInput) {
    Result result;
    result.algorithmName = name;
    result.correct = true;
    
    // Warm-up run
    {
        std::vector<int> warmupInput = baseInput;
        sortFunc(warmupInput);
    }
    
    // 100 measured runs
    double totalTime = 0.0;
    for (int run = 0; run < 100; ++run) {
        std::vector<int> testInput = baseInput; // fresh copy for each run
        auto start = std::chrono::steady_clock::now();
        sortFunc(testInput);
        auto end = std::chrono::steady_clock::now();
        
        // Check correctness (optional but good practice)
        if (run == 0) {
            for (size_t i = 1; i < testInput.size(); ++i) {
                if (testInput[i - 1] > testInput[i]) {
                    result.correct = false;
                    break;
                }
            }
        }
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        totalTime += duration.count();
    }
    
    result.averageTimeMicroseconds = totalTime / 100.0;
    return result;
}

void Benchmark::runAllBenchmarks() {
    // Create test data
    std::vector<int> testData(1000);
    for (int i = 0; i < 1000; ++i) {
        testData[i] = 1000 - i; // reverse sorted
    }
    
    std::cout << "=== Sorting Benchmarks ===" << std::endl;
    
    auto r1 = benchmarkSort("Bubble Sort", Sorting::bubbleSort, testData);
    std::cout << r1.algorithmName << ": " << r1.averageTimeMicroseconds << " μs (correct: " << r1.correct << ")" << std::endl;
    
    auto r2 = benchmarkSort("Selection Sort", Sorting::selectionSort, testData);
    std::cout << r2.algorithmName << ": " << r2.averageTimeMicroseconds << " μs (correct: " << r2.correct << ")" << std::endl;
    
    auto r3 = benchmarkSort("Insertion Sort", Sorting::insertionSort, testData);
    std::cout << r3.algorithmName << ": " << r3.averageTimeMicroseconds << " μs (correct: " << r3.correct << ")" << std::endl;
    
    auto r4 = benchmarkSort("Merge Sort", Sorting::mergeSort, testData);
    std::cout << r4.algorithmName << ": " << r4.averageTimeMicroseconds << " μs (correct: " << r4.correct << ")" << std::endl;
    
    auto r5 = benchmarkSort("Quick Sort", Sorting::quickSort, testData);
    std::cout << r5.algorithmName << ": " << r5.averageTimeMicroseconds << " μs (correct: " << r5.correct << ")" << std::endl;
}

} // namespace algorithms
} // namespace forge
