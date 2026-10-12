#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

extern "C" {
int* APS5_VABI __error_nid_postfix();
void APS5_VABI qsort_r_nid_postfix(void*, std::size_t, std::size_t, void*, int (APS5_VABI *)(void*, const void*, const void*));
int APS5_VABI heapsort_nid_postfix(void*, std::size_t, std::size_t, int (APS5_VABI *)(const void*, const void*));
int APS5_VABI mergesort_nid_postfix(void*, std::size_t, std::size_t, int (APS5_VABI *)(const void*, const void*));
int APS5_VABI radixsort_nid_postfix(const unsigned char**, int, const unsigned char*, unsigned);
int APS5_VABI sradixsort_nid_postfix(const unsigned char**, int, const unsigned char*, unsigned);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

struct Record {
    int key;
    int id;
};

long calls = 0;

int APS5_VABI CompareRecords(const void* a, const void* b) {
    ++calls;
    const int x = static_cast<const Record*>(a)->key, y = static_cast<const Record*>(b)->key;
    return (x > y) - (x < y);
}

int APS5_VABI CompareWithThunk(void* thunk, const void* a, const void* b) {
    ++*static_cast<long*>(thunk);
    return CompareRecords(a, b);
}

constexpr int Keys[] = {0, 0, 5, 5, 2, 1, 2, 6, 3, 5, 4, 4, 0, 3, 3, 0, 5, 2, 0, 1, 2, 0, 0, 1, 2, 2, 1, 3, 6, 3,
                        2, 6, 5, 1, 6, 6, 4, 6, 5, 1, 3, 5, 4, 1, 5, 4, 1, 2, 2, 4, 6, 1, 0, 5, 3, 0, 4, 0, 3, 2};
constexpr int QuickSortIds[] = {22, 21, 12, 0, 55, 15, 52, 57, 18, 1, 51, 23, 5, 19, 46, 43, 39, 33, 26, 59, 4, 6, 17, 20, 24, 25, 30, 47, 48, 58,
                                27, 13, 40, 14, 54, 29, 8, 11, 45, 10, 56, 49, 36, 42, 44, 3, 32, 38, 41, 2, 9, 53, 16, 34, 35, 7, 31, 50, 28, 37};
constexpr int HeapSortIds[] = {57, 21, 52, 18, 15, 0, 55, 22, 12, 1, 5, 19, 39, 23, 43, 26, 46, 51, 33, 30, 25, 47, 24, 48, 4, 59, 17, 20, 6, 29,
                               14, 13, 27, 54, 40, 58, 8, 56, 49, 11, 45, 10, 42, 36, 53, 2, 44, 41, 9, 38, 16, 32, 3, 28, 50, 37, 35, 34, 31, 7};

std::vector<Record> Input() {
    std::vector<Record> records;
    for (int i = 0; i < 60; i++) records.push_back({Keys[i], i});
    return records;
}

bool Ids(const std::vector<Record>& records, const int* expected) {
    for (std::size_t i = 0; i < records.size(); i++)
        if (records[i].id != expected[i]) return false;
    return true;
}

void CheckQuickSort() {
    auto records = Input();
    long thunk = 0;
    qsort_r_nid_postfix(records.data(), records.size(), sizeof(Record), &thunk, CompareWithThunk);
    Require(Ids(records, QuickSortIds) && thunk == 168, "FreeBSD qsort order and comparison count");
    std::mt19937 random(3);
    for (int round = 0; round < 50; round++) {
        std::vector<Record> values(random() % 500);
        for (auto& value : values) value = {static_cast<int>(random() % 1000), 0};
        qsort_r_nid_postfix(values.data(), values.size(), sizeof(Record), &thunk, CompareWithThunk);
        Require(std::is_sorted(values.begin(), values.end(), [](Record a, Record b) { return a.key < b.key; }), "qsort_r sorts");
    }
    qsort_r_nid_postfix(nullptr, 0, sizeof(Record), &thunk, CompareWithThunk);
}

void CheckHeapSort() {
    auto records = Input();
    calls = 0;
    Require(heapsort_nid_postfix(records.data(), records.size(), sizeof(Record), CompareRecords) == 0, "heapsort");
    Require(Ids(records, HeapSortIds) && calls == 408, "FreeBSD heapsort order and comparison count");
    Require(heapsort_nid_postfix(records.data(), 1, 0, CompareRecords) == 0, "one element needs no size");
    *__error_nid_postfix() = 0;
    Require(heapsort_nid_postfix(records.data(), 2, 0, CompareRecords) == -1 && *__error_nid_postfix() == 22, "zero size is EINVAL");
}

void CheckMergeSort() {
    std::mt19937 random(9);
    for (int round = 0; round < 50; round++) {
        std::vector<Record> values(random() % 700);
        for (std::size_t i = 0; i < values.size(); i++) values[i] = {static_cast<int>(random() % 20), static_cast<int>(i)};
        auto expected = values;
        std::stable_sort(expected.begin(), expected.end(), [](Record a, Record b) { return a.key < b.key; });
        Require(mergesort_nid_postfix(values.data(), values.size(), sizeof(Record), CompareRecords) == 0, "mergesort");
        Require(std::memcmp(values.data(), expected.data(), values.size() * sizeof(Record)) == 0, "mergesort is stable");
    }
    char bytes[4] = {3, 1, 2, 0};
    *__error_nid_postfix() = 0;
    Require(mergesort_nid_postfix(bytes, 4, 1, CompareRecords) == -1 && *__error_nid_postfix() == 22, "elements smaller than half a pointer are EINVAL");
    Require(mergesort_nid_postfix(nullptr, 0, sizeof(Record), CompareRecords) == 0, "empty");
}

void CheckRadixSort() {
    const char* words[] = {"ab", "a", "b", "ab", "", "ba", "a", "abc", "ab", "b", "c", "ca", "", "ab", "bb", "a", "cab", "ab", "b", "a", "ba", "ab", "c", "a", "ab"};
    constexpr int Expected[] = {4, 12, 15, 6, 23, 1, 19, 3, 21, 8, 17, 24, 13, 0, 7, 2, 18, 9, 5, 20, 14, 22, 10, 11, 16};
    constexpr int count = 25;
    std::vector<std::string> storage(words, words + count);
    std::vector<const unsigned char*> pointers;
    for (auto& word : storage) pointers.push_back(reinterpret_cast<const unsigned char*>(word.c_str()));
    auto sorted = pointers;
    Require(radixsort_nid_postfix(sorted.data(), count, nullptr, 0) == 0, "radixsort");
    for (int i = 0; i < count; i++) Require(sorted[i] == pointers[Expected[i]], "FreeBSD radixsort order");

    sorted = pointers;
    Require(sradixsort_nid_postfix(sorted.data(), count, nullptr, 0) == 0, "sradixsort");
    std::vector<int> order(count);
    for (int i = 0; i < count; i++) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return storage[a] < storage[b]; });
    for (int i = 0; i < count; i++) Require(sorted[i] == pointers[order[i]], "sradixsort is stable");

    unsigned char reverse[256];
    for (int c = 0; c < 256; c++) reverse[c] = static_cast<unsigned char>(255 - c);
    sorted = pointers;
    Require(sradixsort_nid_postfix(sorted.data(), count, reverse, 0) == 0, "table with end byte 255");
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        const std::string& x = storage[a];
        const std::string& y = storage[b];
        for (std::size_t i = 0;; i++) {
            const int cx = i < x.size() ? 255 - static_cast<unsigned char>(x[i]) : 255;
            const int cy = i < y.size() ? 255 - static_cast<unsigned char>(y[i]) : 255;
            if (cx != cy) return cx < cy;
            if (cx == 255) return false;
        }
    });
    for (int i = 0; i < count; i++) Require(sorted[i] == pointers[order[i]], "translated order");

    unsigned char identity[256];
    for (int c = 0; c < 256; c++) identity[c] = static_cast<unsigned char>(c);
    *__error_nid_postfix() = 0;
    Require(radixsort_nid_postfix(sorted.data(), count, identity, 'x') == -1 && *__error_nid_postfix() == 22, "end byte that maps to neither 0 nor 255");

    std::vector<std::string> many;
    std::mt19937 random(4);
    for (int i = 0; i < 3000; i++) {
        std::string word;
        for (int length = random() % 8; length > 0; length--) word += static_cast<char>('a' + random() % 4);
        many.push_back(word + '/');
    }
    std::vector<const unsigned char*> manyPointers;
    for (auto& word : many) manyPointers.push_back(reinterpret_cast<const unsigned char*>(word.c_str()));
    Require(radixsort_nid_postfix(manyPointers.data(), static_cast<int>(manyPointers.size()), nullptr, '/') == 0, "end byte without table");
    for (std::size_t i = 1; i < manyPointers.size(); i++) {
        const std::string previous(reinterpret_cast<const char*>(manyPointers[i - 1]), std::strchr(reinterpret_cast<const char*>(manyPointers[i - 1]), '/'));
        const std::string current(reinterpret_cast<const char*>(manyPointers[i]), std::strchr(reinterpret_cast<const char*>(manyPointers[i]), '/'));
        Require(previous <= current, "radixsort sorts large inputs");
    }
}

}

int main() {
    std::array<void*, 10> api{};
    ApplicationHeapRegister_nid_no_patch(api.data());
    CheckQuickSort();
    CheckHeapSort();
    CheckMergeSort();
    CheckRadixSort();
    std::puts("guest sorting tests passed");
    return 0;
}
