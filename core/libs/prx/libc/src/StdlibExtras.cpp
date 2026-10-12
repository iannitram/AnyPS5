#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>

#include "prx/libc/include/General.hpp"

extern "C" {
long long APS5_VABI strtoll_nid_postfix(const char* text, char** end, int base);
unsigned long long APS5_VABI strtoull_nid_postfix(const char* text, char** end, int base);
long long APS5_VABI wcstoll_nid_postfix(const char16_t* text, char16_t** end, int base);
unsigned long long APS5_VABI wcstoull_nid_postfix(const char16_t* text, char16_t** end, int base);
void* APS5_VABI realloc_nid_postfix(void* pointer, std::size_t size);
}

namespace {

constexpr int GuestOutOfMemory = 12;

template<typename T>
T Magnitude(T value) {
    using Unsigned = std::make_unsigned_t<T>;
    return value < 0 ? static_cast<T>(Unsigned{0} - static_cast<Unsigned>(value)) : value;
}

struct GuestIntegerDivision {
    std::int64_t quotient;
    std::int64_t remainder;
};

}

extern "C" {

char* suboptarg_nid_postfix = nullptr;

std::int64_t APS5_VABI labs_nid_postfix(std::int64_t value) { return Magnitude(value); }
long long APS5_VABI llabs_nid_postfix(long long value) { return Magnitude(value); }
std::int64_t APS5_VABI imaxabs_nid_postfix(std::int64_t value) { return Magnitude(value); }

GuestIntegerDivision APS5_VABI imaxdiv_nid_postfix(std::int64_t numerator, std::int64_t denominator) {
    GuestIntegerDivision result{numerator / denominator, numerator % denominator};
    if (numerator >= 0 && result.remainder < 0) {
        result.quotient++;
        result.remainder -= denominator;
    }
    return result;
}

long long APS5_VABI strtoq_nid_postfix(const char* text, char** end, int base) { return strtoll_nid_postfix(text, end, base); }
unsigned long long APS5_VABI strtouq_nid_postfix(const char* text, char** end, int base) { return strtoull_nid_postfix(text, end, base); }
std::int64_t APS5_VABI wcstoimax_nid_postfix(const char16_t* text, char16_t** end, int base) { return wcstoll_nid_postfix(text, end, base); }
std::uint64_t APS5_VABI wcstoumax_nid_postfix(const char16_t* text, char16_t** end, int base) { return wcstoull_nid_postfix(text, end, base); }

int APS5_VABI toascii_nid_postfix(int character) { return character & 0x7f; }

void* APS5_VABI reallocarray_nid_postfix(void* pointer, std::size_t count, std::size_t size) {
    constexpr std::size_t NoOverflow = std::size_t{1} << (sizeof(std::size_t) * 4);
    if ((count >= NoOverflow || size >= NoOverflow) && count > 0 && SIZE_MAX / count < size) {
        errno = GuestOutOfMemory;
        return nullptr;
    }
    return realloc_nid_postfix(pointer, size * count);
}

std::int64_t APS5_VABI a64l_nid_postfix(const char* text) {
    if (text == nullptr) throw std::invalid_argument("a64l: null string");
    std::uint32_t value = 0;
    int shift = 0;
    for (int i = 0; *text != '\0' && i < 6; i++, text++) {
        const int c = static_cast<signed char>(*text);
        int digit;
        if (c <= '/') digit = c - '/' + 1;
        else if (c <= '9') digit = c - '0' + 2;
        else if (c <= 'Z') digit = c - 'A' + 12;
        else digit = c - 'a' + 38;
        value |= static_cast<std::uint32_t>(digit) << shift;
        shift += 6;
    }
    return static_cast<std::int32_t>(value);
}

int APS5_VABI l64a_r_nid_postfix(std::int64_t value, char* buffer, int length) {
    if (buffer == nullptr && length > 0) throw std::invalid_argument("l64a_r: null buffer");
    static constexpr char Characters[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    auto bits = static_cast<std::uint32_t>(value);
    while (length-- > 0) {
        if (bits == 0) {
            *buffer = '\0';
            return 0;
        }
        *buffer++ = Characters[bits & 0x3f];
        bits >>= 6;
    }
    return -1;
}

char* APS5_VABI l64a_nid_postfix(std::int64_t value) {
    static char buffer[7];
    l64a_r_nid_postfix(value, buffer, sizeof(buffer));
    return buffer;
}

int APS5_VABI getsubopt_nid_postfix(char** option, char* const* tokens, char** value) {
    if (tokens == nullptr || value == nullptr) throw std::invalid_argument("getsubopt: null tokens or value");
    suboptarg_nid_postfix = *value = nullptr;
    if (!option || !*option) return -1;
    char* p;
    for (p = *option; *p && (*p == ',' || *p == ' ' || *p == '\t'); ++p) {}
    if (!*p) {
        *option = p;
        return -1;
    }
    for (suboptarg_nid_postfix = p; *++p && *p != ',' && *p != '=' && *p != ' ' && *p != '\t';) {}
    if (*p) {
        if (*p == '=') {
            *p = '\0';
            for (*value = ++p; *p && *p != ',' && *p != ' ' && *p != '\t'; ++p) {}
            if (*p) *p++ = '\0';
        } else {
            *p++ = '\0';
        }
        for (; *p && (*p == ',' || *p == ' ' || *p == '\t'); ++p) {}
    }
    *option = p;
    for (int count = 0; *tokens; ++tokens, ++count)
        if (std::strcmp(suboptarg_nid_postfix, *tokens) == 0) return count;
    return -1;
}

}
