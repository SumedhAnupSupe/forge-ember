#ifndef SORTING_H
#define SORTING_H

#include <vector>

namespace forge {
namespace algorithms {

class Sorting {
public:
    static void bubbleSort(std::vector<int>& arr);
    static void selectionSort(std::vector<int>& arr);
    static void insertionSort(std::vector<int>& arr);
    static void mergeSort(std::vector<int>& arr);
    static void quickSort(std::vector<int>& arr);
    
private:
    static void merge(std::vector<int>& arr, int left, int mid, int right);
    static void mergeSortHelper(std::vector<int>& arr, int left, int right);
    static int partition(std::vector<int>& arr, int low, int high);
    static void quickSortHelper(std::vector<int>& arr, int low, int high);
};

} // namespace algorithms
} // namespace forge

#endif // SORTING_H
