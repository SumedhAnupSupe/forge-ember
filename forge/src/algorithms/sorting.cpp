#include "algorithms/sorting.h"
#include <algorithm>

namespace forge {
namespace algorithms {

void Sorting::bubbleSort(std::vector<int>& arr) {
    if (arr.size() <= 1) return;
    size_t n = arr.size();
    for (size_t i = 0; i < n - 1; ++i) {
        for (size_t j = 0; j < n - i - 1; ++j) {
            if (arr[j] > arr[j + 1]) {
                std::swap(arr[j], arr[j + 1]);
            }
        }
    }
}

void Sorting::selectionSort(std::vector<int>& arr) {
    if (arr.size() <= 1) return;
    size_t n = arr.size();
    for (size_t i = 0; i < n - 1; ++i) {
        size_t minIdx = i;
        for (size_t j = i + 1; j < n; ++j) {
            if (arr[j] < arr[minIdx]) {
                minIdx = j;
            }
        }
        if (minIdx != i) {
            std::swap(arr[i], arr[minIdx]);
        }
    }
}

void Sorting::insertionSort(std::vector<int>& arr) {
    if (arr.size() <= 1) return;
    size_t n = arr.size();
    for (size_t i = 1; i < n; ++i) {
        int key = arr[i];
        long j = (long)i - 1;
        while (j >= 0 && arr[(size_t)j] > key) {
            arr[(size_t)j + 1] = arr[(size_t)j];
            --j;
        }
        arr[(size_t)j + 1] = key;
    }
}

void Sorting::merge(std::vector<int>& arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    
    std::vector<int> L(n1), R(n2);
    for (int i = 0; i < n1; ++i) L[(size_t)i] = arr[(size_t)(left + i)];
    for (int j = 0; j < n2; ++j) R[(size_t)j] = arr[(size_t)(mid + 1 + j)];
    
    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[(size_t)i] <= R[(size_t)j]) {
            arr[(size_t)k++] = L[(size_t)i++];
        } else {
            arr[(size_t)k++] = R[(size_t)j++];
        }
    }
    while (i < n1) arr[(size_t)k++] = L[(size_t)i++];
    while (j < n2) arr[(size_t)k++] = R[(size_t)j++];
}

void Sorting::mergeSortHelper(std::vector<int>& arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSortHelper(arr, left, mid);
        mergeSortHelper(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

void Sorting::mergeSort(std::vector<int>& arr) {
    if (arr.empty()) return;
    if (arr.size() == 1) return;
    mergeSortHelper(arr, 0, (int)arr.size() - 1);
}

int Sorting::partition(std::vector<int>& arr, int low, int high) {
    int pivot = arr[(size_t)high];
    int i = (low - 1);
    for (int j = low; j <= high - 1; ++j) {
        if (arr[(size_t)j] <= pivot) {
            ++i;
            std::swap(arr[(size_t)i], arr[(size_t)j]);
        }
    }
    std::swap(arr[(size_t)(i + 1)], arr[(size_t)high]);
    return (i + 1);
}

void Sorting::quickSortHelper(std::vector<int>& arr, int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSortHelper(arr, low, pi - 1);
        quickSortHelper(arr, pi + 1, high);
    }
}

void Sorting::quickSort(std::vector<int>& arr) {
    if (arr.empty()) return;
    if (arr.size() == 1) return;
    quickSortHelper(arr, 0, (int)arr.size() - 1);
}

} // namespace algorithms
} // namespace forge
