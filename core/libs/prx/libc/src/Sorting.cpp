#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "prx/libc/include/General.hpp"

extern "C" {
int* APS5_VABI __error_nid_postfix();
void* APS5_VABI malloc_nid_postfix(std::size_t size);
void APS5_VABI free_nid_postfix(void* pointer);
}

namespace {

using Compare = int (APS5_VABI *)(const void*, const void*);
using ThunkCompare = int (APS5_VABI *)(void*, const void*, const void*);

constexpr int GuestInvalidArgument = 22;

void SwapBytes(char* a, char* b, std::size_t size) {
    do {
        const char t = *a;
        *a++ = *b;
        *b++ = t;
    } while (--size > 0);
}

char* MedianOfThree(char* a, char* b, char* c, ThunkCompare compare, void* thunk) {
    return compare(thunk, a, b) < 0 ? (compare(thunk, b, c) < 0 ? b : (compare(thunk, a, c) < 0 ? c : a))
                                    : (compare(thunk, b, c) > 0 ? b : (compare(thunk, a, c) < 0 ? a : c));
}

void InsertionSort(char* a, std::size_t n, std::size_t es, ThunkCompare compare, void* thunk) {
    for (char* pm = a + es; pm < a + n * es; pm += es)
        for (char* pl = pm; pl > a && compare(thunk, pl - es, pl) > 0; pl -= es) SwapBytes(pl, pl - es, es);
}

void QuickSort(char* a, std::size_t n, std::size_t es, void* thunk, ThunkCompare compare) {
    for (;;) {
        bool swapped = false;
        if (n < 7) {
            InsertionSort(a, n, es, compare, thunk);
            return;
        }
        char* pm = a + (n / 2) * es;
        if (n > 7) {
            char* pl = a;
            char* pn = a + (n - 1) * es;
            if (n > 40) {
                const std::size_t d = (n / 8) * es;
                pl = MedianOfThree(pl, pl + d, pl + 2 * d, compare, thunk);
                pm = MedianOfThree(pm - d, pm, pm + d, compare, thunk);
                pn = MedianOfThree(pn - 2 * d, pn - d, pn, compare, thunk);
            }
            pm = MedianOfThree(pl, pm, pn, compare, thunk);
        }
        SwapBytes(a, pm, es);
        char* pa = a + es;
        char* pb = pa;
        char* pc = a + (n - 1) * es;
        char* pd = pc;
        for (;;) {
            int result;
            while (pb <= pc && (result = compare(thunk, pb, a)) <= 0) {
                if (result == 0) {
                    swapped = true;
                    SwapBytes(pa, pb, es);
                    pa += es;
                }
                pb += es;
            }
            while (pb <= pc && (result = compare(thunk, pc, a)) >= 0) {
                if (result == 0) {
                    swapped = true;
                    SwapBytes(pc, pd, es);
                    pd -= es;
                }
                pc -= es;
            }
            if (pb > pc) break;
            SwapBytes(pb, pc, es);
            swapped = true;
            pb += es;
            pc -= es;
        }
        if (!swapped) {
            InsertionSort(a, n, es, compare, thunk);
            return;
        }

        char* pn = a + n * es;
        std::size_t d1 = std::min<std::size_t>(pa - a, pb - pa);
        if (d1 > 0) SwapBytes(a, pb - d1, d1);
        d1 = std::min<std::size_t>(pd - pc, pn - pd - es);
        if (d1 > 0) SwapBytes(pb, pn - d1, d1);

        d1 = pb - pa;
        const std::size_t d2 = pd - pc;
        if (d1 <= d2) {
            if (d1 > es) QuickSort(a, d1 / es, es, thunk, compare);
            if (d2 <= es) return;
            a = pn - d2;
            n = d2 / es;
        } else {
            if (d2 > es) QuickSort(pn - d2, d2 / es, es, thunk, compare);
            if (d1 <= es) return;
            n = d1 / es;
        }
    }
}

void CopyBytes(char* destination, const char* source, std::size_t size) { std::memmove(destination, source, size); }

int HeapSort(void* vbase, std::size_t nmemb, std::size_t size, Compare compare) {
    if (nmemb <= 1) return 0;
    if (size == 0) {
        *__error_nid_postfix() = GuestInvalidArgument;
        return -1;
    }
    auto* k = static_cast<char*>(malloc_nid_postfix(size));
    if (k == nullptr) return -1;
    char* base = static_cast<char*>(vbase) - size;

    for (std::size_t l = nmemb / 2 + 1; --l;) {
        std::size_t parentIndex, childIndex;
        for (parentIndex = l; (childIndex = parentIndex * 2) <= nmemb; parentIndex = childIndex) {
            char* child = base + childIndex * size;
            if (childIndex < nmemb && compare(child, child + size) < 0) {
                child += size;
                ++childIndex;
            }
            char* parent = base + parentIndex * size;
            if (compare(child, parent) <= 0) break;
            SwapBytes(parent, child, size);
        }
    }

    while (nmemb > 1) {
        CopyBytes(k, base + nmemb * size, size);
        CopyBytes(base + nmemb * size, base + size, size);
        --nmemb;
        std::size_t parentIndex, childIndex;
        for (parentIndex = 1; (childIndex = parentIndex * 2) <= nmemb; parentIndex = childIndex) {
            char* child = base + childIndex * size;
            if (childIndex < nmemb && compare(child, child + size) < 0) {
                child += size;
                ++childIndex;
            }
            CopyBytes(base + parentIndex * size, child, size);
        }
        for (;;) {
            childIndex = parentIndex;
            parentIndex = childIndex / 2;
            char* child = base + childIndex * size;
            char* parent = base + parentIndex * size;
            if (childIndex == 1 || compare(k, parent) < 0) {
                CopyBytes(child, k, size);
                break;
            }
            CopyBytes(child, parent, size);
        }
    }
    free_nid_postfix(k);
    return 0;
}

int MergeSort(void* base, std::size_t nmemb, std::size_t size, Compare compare) {
    if (size < sizeof(void*) / 2) {
        *__error_nid_postfix() = GuestInvalidArgument;
        return -1;
    }
    if (nmemb == 0) return 0;
    auto* buffer = static_cast<char*>(malloc_nid_postfix(nmemb * size + sizeof(void*)));
    if (buffer == nullptr) return -1;
    auto* data = static_cast<char*>(base);
    for (std::size_t width = 1; width < nmemb; width *= 2) {
        for (std::size_t left = 0; left < nmemb; left += 2 * width) {
            const std::size_t middle = std::min(left + width, nmemb);
            const std::size_t right = std::min(left + 2 * width, nmemb);
            std::size_t i = left, j = middle, out = left;
            while (i < middle && j < right) {
                if (compare(data + i * size, data + j * size) <= 0) std::memcpy(buffer + out++ * size, data + i++ * size, size);
                else std::memcpy(buffer + out++ * size, data + j++ * size, size);
            }
            if (i < middle) std::memcpy(buffer + out * size, data + i * size, (middle - i) * size), out += middle - i;
            if (j < right) std::memcpy(buffer + out * size, data + j * size, (right - j) * size);
        }
        std::memcpy(data, buffer, nmemb * size);
    }
    free_nid_postfix(buffer);
    return 0;
}

using Byte = unsigned char;

struct RadixStack {
    const Byte** strings;
    int count;
    int index;
};

constexpr int RadixThreshold = 20;
constexpr int RadixStackSize = 512;

void SimpleSort(const Byte** a, int n, int b, const Byte* tr, unsigned endch) {
    for (const Byte** ak = a + 1; --n >= 1; ak++) {
        for (const Byte** ai = ak; ai > a; ai--) {
            const Byte* s = ai[0] + b;
            const Byte* t = ai[-1] + b;
            Byte ch;
            for (; (ch = tr[*s]) != endch; s++, t++)
                if (ch != tr[*t]) break;
            if (ch >= tr[*t]) break;
            std::swap(ai[0], ai[-1]);
        }
    }
}

int sortCountA[256];
int sortBinsA;
int sortMinimumA;

void RadixSortUnstable(const Byte** a, int n, int i, const Byte* tr, unsigned endch) {
    int* count = sortCountA;
    int& nc = sortBinsA;
    int& bmin = sortMinimumA;
    RadixStack s[RadixStackSize];
    RadixStack* sp = s;
    const Byte** top[256];
    *sp++ = {a, n, i};
    while (sp > s) {
        --sp;
        a = sp->strings;
        n = sp->count;
        i = sp->index;
        if (n < RadixThreshold) {
            SimpleSort(a, n, i, tr, endch);
            continue;
        }
        const Byte** an = a + n;
        const Byte** ak;
        if (nc == 0) {
            bmin = 255;
            for (ak = a; ak < an;) {
                const int c = tr[(*ak++)[i]];
                if (++count[c] == 1 && static_cast<unsigned>(c) != endch) {
                    if (c < bmin) bmin = c;
                    nc++;
                }
            }
            if (sp + nc > s + RadixStackSize) {
                RadixSortUnstable(a, n, i, tr, endch);
                continue;
            }
        }
        if (nc == 1 && count[bmin] == n) {
            *sp++ = {a, n, i + 1};
            nc = count[bmin] = 0;
            continue;
        }
        RadixStack* sp0 = sp;
        RadixStack* sp1 = sp;
        int biggest = 2;
        if (endch == 0) {
            top[0] = ak = a + count[0];
        } else {
            ak = a;
            top[255] = an;
        }
        for (int* cp = count + bmin; nc > 0; cp++) {
            while (*cp == 0) cp++;
            if (*cp > 1) {
                if (*cp > biggest) {
                    biggest = *cp;
                    sp1 = sp;
                }
                *sp++ = {ak, *cp, i + 1};
            }
            top[cp - count] = ak += *cp;
            nc--;
        }
        std::swap(*sp0, *sp1);
        int c = 0;
        const Byte* r = nullptr;
        for (const Byte** aj = a; aj < an; *aj = r, aj += count[c], count[c] = 0)
            for (r = *aj; aj < (ak = --top[c = tr[r[i]]]);) std::swap(*ak, r);
    }
}

int sortCountB[256];
int sortBinsB;
int sortMinimumB;

void RadixSortStable(const Byte** a, const Byte** ta, int n, int i, const Byte* tr, unsigned endch) {
    int* count = sortCountB;
    int& nc = sortBinsB;
    int& bmin = sortMinimumB;
    RadixStack s[RadixStackSize];
    RadixStack* sp = s;
    const Byte** top[256];
    *sp++ = {a, n, i};
    while (sp > s) {
        --sp;
        a = sp->strings;
        n = sp->count;
        i = sp->index;
        if (n < RadixThreshold) {
            SimpleSort(a, n, i, tr, endch);
            continue;
        }
        const Byte** ak;
        if (nc == 0) {
            bmin = 255;
            for (ak = a + n; --ak >= a;) {
                const int c = tr[(*ak)[i]];
                if (++count[c] == 1 && static_cast<unsigned>(c) != endch) {
                    if (c < bmin) bmin = c;
                    nc++;
                }
            }
            if (sp + nc > s + RadixStackSize) {
                RadixSortStable(a, ta, n, i, tr, endch);
                continue;
            }
        }
        RadixStack* sp0 = sp;
        RadixStack* sp1 = sp;
        int biggest = 2;
        if (endch == 0) {
            top[0] = ak = a + count[0];
            count[0] = 0;
        } else {
            ak = a;
            top[255] = a + n;
            count[255] = 0;
        }
        for (int* cp = count + bmin; nc > 0; cp++) {
            while (*cp == 0) cp++;
            const int c = *cp;
            if (c > 1) {
                if (c > biggest) {
                    biggest = c;
                    sp1 = sp;
                }
                *sp++ = {ak, c, i + 1};
            }
            top[cp - count] = ak += c;
            *cp = 0;
            nc--;
        }
        std::swap(*sp0, *sp1);
        const Byte** ai = a + n;
        for (ak = ta + n; ak > ta;) *--ak = *--ai;
        for (ak = ta + n; --ak >= ta;) *--top[tr[(*ak)[i]]] = *ak;
    }
}

bool RadixTable(const Byte* table, unsigned& endch, Byte* own, const Byte*& tr) {
    if (table == nullptr) {
        unsigned c;
        tr = own;
        for (c = 0; c < endch; c++) own[c] = static_cast<Byte>(c + 1);
        own[c] = 0;
        for (c++; c < 256; c++) own[c] = static_cast<Byte>(c);
        endch = 0;
        return true;
    }
    endch = table[endch];
    tr = table;
    if (endch != 0 && endch != 255) {
        *__error_nid_postfix() = GuestInvalidArgument;
        return false;
    }
    return true;
}

}

extern "C" {

void APS5_VABI qsort_r_nid_postfix(void* base, std::size_t count, std::size_t size, void* thunk, ThunkCompare compare) {
    if (compare == nullptr) throw std::invalid_argument("qsort_r: null comparison");
    QuickSort(static_cast<char*>(base), count, size, thunk, compare);
}

int APS5_VABI heapsort_nid_postfix(void* base, std::size_t count, std::size_t size, Compare compare) {
    if (compare == nullptr) throw std::invalid_argument("heapsort: null comparison");
    return HeapSort(base, count, size, compare);
}

int APS5_VABI mergesort_nid_postfix(void* base, std::size_t count, std::size_t size, Compare compare) {
    if (compare == nullptr) throw std::invalid_argument("mergesort: null comparison");
    return MergeSort(base, count, size, compare);
}

int APS5_VABI radixsort_nid_postfix(const Byte** strings, int count, const Byte* table, unsigned endByte) {
    if (endByte > 255) throw std::invalid_argument("radixsort: end byte out of range");
    Byte own[256];
    const Byte* tr;
    if (!RadixTable(table, endByte, own, tr)) return -1;
    RadixSortUnstable(strings, count, 0, tr, endByte);
    return 0;
}

int APS5_VABI sradixsort_nid_postfix(const Byte** strings, int count, const Byte* table, unsigned endByte) {
    if (endByte > 255) throw std::invalid_argument("sradixsort: end byte out of range");
    Byte own[256];
    const Byte* tr;
    if (!RadixTable(table, endByte, own, tr)) return -1;
    if (count < RadixThreshold) {
        SimpleSort(strings, count, 0, tr, endByte);
        return 0;
    }
    auto* temporary = static_cast<const Byte**>(malloc_nid_postfix(count * sizeof(*strings)));
    if (temporary == nullptr) return -1;
    RadixSortStable(strings, temporary, count, 0, tr, endByte);
    free_nid_postfix(temporary);
    return 0;
}

}
