#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include "algorithms/sorting.h"

using namespace forge::algorithms;

int test_sorting() {
    int failures = 0;
    std::cout << "Testing Sorting..." << std::endl;
    std::vector<int> empty;
    std::vector<int> e2 = empty;
    Sorting::bubbleSort(e2);
    if (e2.empty()) std::cout << "  Empty sort OK" << std::endl;
    else { failures++; }
    std::vector<int> single = {5};
    Sorting::quickSort(single);
    if (single[0] == 5) std::cout << "  Singleton sort OK" << std::endl;
    else { failures++; }
    
    auto test = [&](auto fn, const std::string& name) {
        std::vector<int> v = {5, 2, 9, 1, 5, -1, 0};
        std::vector<int> exp = v;
        std::sort(exp.begin(), exp.end());
        std::vector<int> t = v;
        fn(t);
        if (t == exp) std::cout << "  " << name << " correct OK" << std::endl;
        else { std::cout << "  " << name << " correct FAILED" << std::endl; failures++; }
    };
    test(Sorting::bubbleSort, "Bubble");
    test(Sorting::selectionSort, "Selection");
    test(Sorting::insertionSort, "Insertion");
    test(Sorting::mergeSort, "Merge");
    test(Sorting::quickSort, "Quick");
    return failures;
}
