#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>

#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestTime.hpp"

extern "C" {
std::size_t APS5_VABI wcsrtombs_nid_postfix(char* destination, const std::uint16_t** source, std::size_t capacity, void* state);
std::size_t APS5_VABI mbsrtowcs_nid_postfix(std::uint16_t* destination, const char** source, std::size_t capacity, void* state);
std::size_t APS5_VABI wcrtomb_nid_postfix(char* destination, std::uint16_t value, void* state);
std::size_t APS5_VABI strftime_nid_postfix(char* destination, std::size_t capacity, const char* format, const GuestTm* time);
std::uint32_t APS5_VABI towlower_nid_postfix(std::uint32_t value);
std::uint32_t APS5_VABI towupper_nid_postfix(std::uint32_t value);
}

namespace {

using Wide = std::uint16_t;

constexpr int GuestInvalidArgument = 22;
constexpr int GuestOutOfMemory = 12;
constexpr int TransformError = 0;
constexpr int TransformLower = 1;
constexpr int TransformUpper = 2;

std::size_t Length(const Wide* text) {
    std::size_t length = 0;
    while (text[length] != 0) ++length;
    return length;
}

}

extern "C" {

Wide* APS5_VABI wcscat_nid_postfix(Wide* destination, const Wide* source) {
    Wide* end = destination + Length(destination);
    while ((*end++ = *source++) != 0) {}
    return destination;
}

Wide* APS5_VABI wcsncat_nid_postfix(Wide* destination, const Wide* source, std::size_t count) {
    Wide* end = destination + Length(destination);
    while (count != 0 && *source != 0) {
        *end++ = *source++;
        --count;
    }
    *end = 0;
    return destination;
}

std::size_t APS5_VABI wcscspn_nid_postfix(const Wide* text, const Wide* reject) {
    std::size_t length = 0;
    for (; text[length] != 0; ++length)
        for (const Wide* r = reject; *r != 0; ++r)
            if (*r == text[length]) return length;
    return length;
}

int APS5_VABI wctomb_nid_postfix(char* destination, Wide value) {
    if (destination == nullptr) return 0;
    const std::size_t result = wcrtomb_nid_postfix(destination, value, nullptr);
    return result == static_cast<std::size_t>(-1) ? -1 : static_cast<int>(result);
}

int APS5_VABI wctrans_nid_postfix(const char* name) {
    if (name == nullptr) throw std::invalid_argument("wctrans: null name");
    if (std::strcmp(name, "tolower") == 0) return TransformLower;
    if (std::strcmp(name, "toupper") == 0) return TransformUpper;
    errno = GuestInvalidArgument;
    return TransformError;
}

std::uint32_t APS5_VABI towctrans_nid_postfix(std::uint32_t value, int transform) {
    switch (transform) {
    case TransformLower: return towlower_nid_postfix(value);
    case TransformUpper: return towupper_nid_postfix(value);
    default:
        errno = GuestInvalidArgument;
        return value;
    }
}

std::size_t APS5_VABI wcsftime_nid_postfix(Wide* destination, std::size_t capacity, const Wide* format, const GuestTm* time) {
    if (format == nullptr || time == nullptr) throw std::invalid_argument("wcsftime: null format or time");
    const Wide* cursor = format;
    const std::size_t formatLength = wcsrtombs_nid_postfix(nullptr, &cursor, 0, nullptr);
    if (formatLength == static_cast<std::size_t>(-1)) return 0;
    std::unique_ptr<char[]> narrowFormat(new (std::nothrow) char[formatLength + 1]);
    if (!narrowFormat) {
        errno = GuestOutOfMemory;
        return 0;
    }
    cursor = format;
    wcsrtombs_nid_postfix(narrowFormat.get(), &cursor, formatLength + 1, nullptr);
    if (capacity >= SIZE_MAX) {
        errno = GuestInvalidArgument;
        return 0;
    }
    std::unique_ptr<char[]> narrow(new (std::nothrow) char[capacity == 0 ? 1 : capacity]);
    if (!narrow) {
        errno = GuestOutOfMemory;
        return 0;
    }
    if (strftime_nid_postfix(narrow.get(), capacity, narrowFormat.get(), time) == 0) return 0;
    const char* source = narrow.get();
    const std::size_t converted = mbsrtowcs_nid_postfix(destination, &source, capacity, nullptr);
    if (converted == static_cast<std::size_t>(-1) || converted == static_cast<std::size_t>(-2) || source != nullptr) return 0;
    return converted;
}

}
