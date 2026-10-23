#ifndef SEARCHING_H
#define SEARCHING_H

#include <vector>

namespace forge {
namespace algorithms {

class Searching {
public:
    static int linearSearch(const std::vector<int>& arr, int target);
    static int binarySearch(const std::vector<int>& arr, int target);
    static int fibonacciSearch(const std::vector<int>& arr, int target);
};

} // namespace algorithms
} // namespace forge

#endif // SEARCHING_H
