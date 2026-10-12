#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {
struct Division {
    std::int64_t quotient;
    std::int64_t remainder;
};
}

extern "C" {
extern char* suboptarg_nid_postfix;
std::int64_t APS5_VABI labs_nid_postfix(std::int64_t);
long long APS5_VABI llabs_nid_postfix(long long);
std::int64_t APS5_VABI imaxabs_nid_postfix(std::int64_t);
Division APS5_VABI imaxdiv_nid_postfix(std::int64_t, std::int64_t);
long long APS5_VABI strtoq_nid_postfix(const char*, char**, int);
unsigned long long APS5_VABI strtouq_nid_postfix(const char*, char**, int);
std::int64_t APS5_VABI wcstoimax_nid_postfix(const char16_t*, char16_t**, int);
std::uint64_t APS5_VABI wcstoumax_nid_postfix(const char16_t*, char16_t**, int);
int APS5_VABI toascii_nid_postfix(int);
void* APS5_VABI reallocarray_nid_postfix(void*, std::size_t, std::size_t);
void APS5_VABI free_nid_postfix(void*);
std::int64_t APS5_VABI a64l_nid_postfix(const char*);
char* APS5_VABI l64a_nid_postfix(std::int64_t);
int APS5_VABI l64a_r_nid_postfix(std::int64_t, char*, int);
int APS5_VABI getsubopt_nid_postfix(char**, char* const*, char**);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

void CheckIntegers() {
    Require(labs_nid_postfix(-5) == 5 && labs_nid_postfix(INT64_MIN) == INT64_MIN && llabs_nid_postfix(-7) == 7 && imaxabs_nid_postfix(9) == 9, "absolute values");
    const Division division = imaxdiv_nid_postfix(-7, 2);
    Require(division.quotient == -3 && division.remainder == -1, "imaxdiv truncates");
    const Division positive = imaxdiv_nid_postfix(INT64_MAX, -10);
    Require(positive.quotient == INT64_MAX / -10 && positive.remainder == 7, "imaxdiv with a negative divisor");
    char* end = nullptr;
    const char text[] = "  -0x1fz";
    Require(strtoq_nid_postfix(text, &end, 0) == -31 && end == text + 7, "strtoq");
    Require(strtouq_nid_postfix("18446744073709551615", nullptr, 10) == UINT64_MAX, "strtouq");
    const char16_t wide[] = u" 123x";
    char16_t* wideEnd = nullptr;
    Require(wcstoimax_nid_postfix(wide, &wideEnd, 10) == 123 && wideEnd == wide + 4, "wcstoimax");
    Require(wcstoumax_nid_postfix(u"ff", nullptr, 16) == 255, "wcstoumax");
    Require(toascii_nid_postfix(0xe9) == 0x69 && toascii_nid_postfix(-1) == 0x7f, "toascii");
}

void CheckReallocarray() {
    auto* values = static_cast<int*>(reallocarray_nid_postfix(nullptr, 4, sizeof(int)));
    Require(values != nullptr, "reallocarray allocates");
    values[3] = 42;
    values = static_cast<int*>(reallocarray_nid_postfix(values, 1000, sizeof(int)));
    Require(values != nullptr && values[3] == 42, "reallocarray grows");
    errno = 0;
    Require(reallocarray_nid_postfix(values, SIZE_MAX / 2, 3) == nullptr && errno == 12, "overflow is ENOMEM");
    free_nid_postfix(values);
}

void CheckBase64() {
    Require(a64l_nid_postfix("") == 0 && a64l_nid_postfix("/") == 1 && a64l_nid_postfix("./") == 64, "a64l digits");
    Require(a64l_nid_postfix("zzzzzz") == -1 && a64l_nid_postfix("zzzzzzzz") == -1, "a64l wraps in 32 bits and reads six characters");
    Require(a64l_nid_postfix("A") == 12 && a64l_nid_postfix("a") == 38 && a64l_nid_postfix("9") == 11, "a64l alphabet");
    Require(std::strcmp(l64a_nid_postfix(0), "") == 0 && std::strcmp(l64a_nid_postfix(64), "./") == 0, "l64a");
    Require(std::strcmp(l64a_nid_postfix(0x100000000ll + 1), "/") == 0, "l64a uses the low 32 bits");
    for (std::int64_t value : {1ll, 12345ll, 0x7fffffffll, 0x80000000ll}) Require(static_cast<std::uint32_t>(a64l_nid_postfix(l64a_nid_postfix(value))) == static_cast<std::uint32_t>(value), "round trip");
    char buffer[3];
    Require(l64a_r_nid_postfix(4095, buffer, 3) == 0 && std::strcmp(buffer, "zz") == 0, "l64a_r fits");
    Require(l64a_r_nid_postfix(4096, buffer, 3) == -1, "l64a_r too small");
}

void CheckSubOptions() {
    char options[] = " ,ro, rw=yes,size=12\tbad ";
    char* const tokens[] = {const_cast<char*>("ro"), const_cast<char*>("rw"), const_cast<char*>("size"), nullptr};
    char* cursor = options;
    char* value = reinterpret_cast<char*>(1);
    Require(getsubopt_nid_postfix(&cursor, tokens, &value) == 0 && value == nullptr && std::strcmp(suboptarg_nid_postfix, "ro") == 0, "flag option");
    Require(getsubopt_nid_postfix(&cursor, tokens, &value) == 1 && std::strcmp(value, "yes") == 0, "option with a value");
    Require(getsubopt_nid_postfix(&cursor, tokens, &value) == 2 && std::strcmp(value, "12") == 0, "tab ends the value");
    Require(getsubopt_nid_postfix(&cursor, tokens, &value) == -1 && std::strcmp(suboptarg_nid_postfix, "bad") == 0, "unknown option");
    Require(getsubopt_nid_postfix(&cursor, tokens, &value) == -1 && suboptarg_nid_postfix == nullptr && *cursor == '\0', "end");
    char* empty = nullptr;
    Require(getsubopt_nid_postfix(&empty, tokens, &value) == -1, "null option string");
    Require(getsubopt_nid_postfix(&empty, nullptr, &value) == -1 && value == nullptr, "a null option string returns before reading the tokens");
}

}

int main() {
    std::array<void*, 10> api{};
    ApplicationHeapRegister_nid_no_patch(api.data());
    CheckIntegers();
    CheckReallocarray();
    CheckBase64();
    CheckSubOptions();
    std::puts("guest stdlib extras tests passed");
    return 0;
}
