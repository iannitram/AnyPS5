#include "prx/libc/include/GuestTime.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
char16_t* APS5_VABI wcscat_nid_postfix(char16_t*, const char16_t*);
char16_t* APS5_VABI wcsncat_nid_postfix(char16_t*, const char16_t*, std::size_t);
std::size_t APS5_VABI wcscspn_nid_postfix(const char16_t*, const char16_t*);
int APS5_VABI wctomb_nid_postfix(char*, char16_t);
int APS5_VABI wctrans_nid_postfix(const char*);
std::uint32_t APS5_VABI towctrans_nid_postfix(std::uint32_t, int);
std::uint32_t APS5_VABI _Towctrans_nid_postfix(std::uint32_t, std::uint64_t);
std::size_t APS5_VABI wcsftime_nid_postfix(char16_t*, std::size_t, const char16_t*, const GuestTm*);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

bool Equal(const char16_t* a, const char16_t* b) {
    while (*a && *a == *b) ++a, ++b;
    return *a == *b;
}

}

int main() {
    char16_t text[16] = u"ab";
    Require(wcscat_nid_postfix(text, u"cd") == text && Equal(text, u"abcd"), "wcscat");
    Require(wcsncat_nid_postfix(text, u"efgh", 2) == text && Equal(text, u"abcdef"), "wcsncat limits");
    Require(Equal(wcsncat_nid_postfix(text, u"g", 9), u"abcdefg"), "wcsncat stops at the terminator");
    Require(Equal(wcsncat_nid_postfix(text, u"xyz", 0), u"abcdefg"), "wcsncat of nothing");
    Require(wcscspn_nid_postfix(u"hello world", u" o") == 4 && wcscspn_nid_postfix(u"abc", u"xyz") == 3 && wcscspn_nid_postfix(u"", u"a") == 0, "wcscspn");
    Require(wcscspn_nid_postfix(u"€x", u"x") == 1, "wcscspn compares full units");

    char byte = 0;
    Require(wctomb_nid_postfix(nullptr, u'a') == 0, "no state-dependent encodings");
    Require(wctomb_nid_postfix(&byte, u'Z') == 1 && byte == 'Z', "wctomb");
    errno = 0;
    Require(wctomb_nid_postfix(&byte, u'€') == -1 && errno == 86, "wctomb of an unrepresentable character");

    const int lower = wctrans_nid_postfix("tolower");
    const int upper = wctrans_nid_postfix("toupper");
    Require(lower == 1 && upper == 2, "wctrans values");
    Require(towctrans_nid_postfix('Q', lower) == 'q' && towctrans_nid_postfix('q', upper) == 'Q' && towctrans_nid_postfix('5', upper) == '5', "towctrans");
    Require(_Towctrans_nid_postfix('Q', lower) == 'q' && _Towctrans_nid_postfix('q', upper) == 'Q', "_Towctrans takes the wctrans descriptors");
    errno = 0;
    Require(wctrans_nid_postfix("totitle") == 0 && errno == 22, "unknown transformation");
    errno = 0;
    Require(towctrans_nid_postfix('a', 7) == 'a' && errno == 22, "invalid transformation");

    GuestTm time{};
    time.tm_year = 124;
    time.tm_mon = 2;
    time.tm_mday = 5;
    time.tm_hour = 9;
    time.tm_min = 7;
    time.tm_wday = 2;
    char16_t buffer[32];
    Require(wcsftime_nid_postfix(buffer, 32, u"%Y-%m-%d %H:%M", &time) == 16 && Equal(buffer, u"2024-03-05 09:07"), "wcsftime");
    Require(wcsftime_nid_postfix(buffer, 16, u"%Y-%m-%d %H:%M", &time) == 0, "wcsftime needs room for the terminator");
    Require(wcsftime_nid_postfix(buffer, 32, u"%a %%", &time) == 5 && Equal(buffer, u"Tue %"), "wcsftime conversions");
    Require(wcsftime_nid_postfix(buffer, 32, u"€%Y", &time) == 0, "unrepresentable format character");
    std::puts("guest wide extras tests passed");
    return 0;
}
