#include "prx/libc/include/Api.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>

#include "prx/libc/include/General.hpp"

extern "C" {
unsigned long long APS5_VABI strtoull_nid_postfix(const char* text, char** end, int base);
long long APS5_VABI wcstoll_nid_postfix(const char16_t* text, char16_t** end, int base);
unsigned long long APS5_VABI wcstoull_nid_postfix(const char16_t* text, char16_t** end, int base);
}

namespace {

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

int APS5_VABI getsubopt_nid_postfix(char** option, char* const* tokens, char** value) {
    if (value == nullptr) throw std::invalid_argument("getsubopt: null value");
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
    if (tokens == nullptr) throw std::invalid_argument("getsubopt: null tokens");
    for (int count = 0; *tokens; ++tokens, ++count)
        if (std::strcmp(suboptarg_nid_postfix, *tokens) == 0) return count;
    return -1;
}

}
