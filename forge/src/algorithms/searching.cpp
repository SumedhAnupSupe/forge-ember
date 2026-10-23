#include "algorithms/searching.h"
#include <algorithm>

namespace forge {
namespace algorithms {

int Searching::linearSearch(const std::vector<int>& arr, int target) {
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i] == target) return (int)i;
    }
    return -1;
}

int Searching::binarySearch(const std::vector<int>& arr, int target) {
    int left = 0;
    int right = (int)arr.size() - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (arr[mid] == target) return mid;
        if (arr[mid] < target) left = mid + 1;
        else right = mid - 1;
    }
    return -1;
}

int Searching::fibonacciSearch(const std::vector<int>& arr, int target) {
    int n = (int)arr.size();
    if (n == 0) return -1;
    
    // Generate Fibonacci numbers
    int fib2 = 0;
    int fib1 = 1;
    int fibM = fib2 + fib1;
    while (fibM < n) {
        fib2 = fib1;
        fib1 = fibM;
        fibM = fib2 + fib1;
    }
    
    int offset = -1;
    while (fibM > 1) {
        int i = std::min(offset + fib2, n - 1);
        if (arr[i] < target) {
            fibM = fib1;
            fib1 = fib2;
            fib2 = fibM - fib1;
            offset = i;
        } else if (arr[i] > target) {
            fibM = fib2;
            fib1 = fib1 - fib2;
            fib2 = fibM - fib1;
        } else {
            return i;
        }
    }
    if (fib1 && offset + 1 < n && arr[offset + 1] == target) {
        return offset + 1;
    }
    return -1;
}

} // namespace algorithms
} // namespace forge
