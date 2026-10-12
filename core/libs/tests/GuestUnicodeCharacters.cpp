#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI mblen_nid_postfix(const char*, std::size_t);
std::size_t APS5_VABI mbrtoc32_nid_postfix(std::uint32_t*, const char*, std::size_t, void*);
std::size_t APS5_VABI c32rtomb_nid_postfix(char*, std::uint32_t, void*);
std::size_t APS5_VABI mbrtoc16_nid_postfix(std::uint16_t*, const char*, std::size_t, void*);
std::size_t APS5_VABI c16rtomb_nid_postfix(char*, std::uint16_t, void*);
}

namespace {

constexpr std::size_t Failed = static_cast<std::size_t>(-1);
constexpr std::size_t Incomplete = static_cast<std::size_t>(-2);

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

}

int main() {
    Require(mblen_nid_postfix(nullptr, 0) == 0, "mblen reset");
    Require(mblen_nid_postfix("a", 1) == 1 && mblen_nid_postfix("", 1) == 0 && mblen_nid_postfix("a", 0) == -1, "mblen");
    Require(mblen_nid_postfix("\xe9", 1) == 1, "mblen accepts any byte in the C locale");

    std::uint32_t wide = 7;
    Require(mbrtoc32_nid_postfix(&wide, "\xe9", 1, nullptr) == 1 && wide == 0xe9, "mbrtoc32");
    Require(mbrtoc32_nid_postfix(&wide, "", 1, nullptr) == 0 && wide == 0, "mbrtoc32 null character");
    Require(mbrtoc32_nid_postfix(&wide, "x", 0, nullptr) == Incomplete, "mbrtoc32 incomplete");
    char byte = 0;
    Require(c32rtomb_nid_postfix(&byte, 0x41, nullptr) == 1 && byte == 'A', "c32rtomb");
    errno = 0;
    Require(c32rtomb_nid_postfix(&byte, 0x100, nullptr) == Failed && errno == 86, "c32rtomb outside the C locale");
    Require(c32rtomb_nid_postfix(nullptr, 0x100, nullptr) == 1, "c32rtomb reset");

    std::uint16_t state[64] = {};
    std::uint16_t narrow = 0;
    Require(mbrtoc16_nid_postfix(&narrow, "z", 1, state) == 1 && narrow == 'z', "mbrtoc16");
    state[0] = 0xdc01;
    Require(mbrtoc16_nid_postfix(&narrow, "z", 1, state) == static_cast<std::size_t>(-3) && narrow == 0xdc01 && state[0] == 0, "pending trail surrogate");

    Require(c16rtomb_nid_postfix(&byte, u'b', state) == 1 && byte == 'b', "c16rtomb");
    Require(c16rtomb_nid_postfix(&byte, 0xd83d, state) == 0 && state[0] == 0xd83d, "lead surrogate is stored");
    errno = 0;
    Require(c16rtomb_nid_postfix(&byte, u'x', state) == Failed && errno == 86, "lead without trail");
    state[0] = 0xd83d;
    Require(c16rtomb_nid_postfix(&byte, 0xde00, state) == Failed && state[0] == 0, "supplementary character has no C locale byte");
    Require(c16rtomb_nid_postfix(&byte, 0xdc00, state) == Failed, "lone trail surrogate");
    Require(c16rtomb_nid_postfix(nullptr, u'q', state) == 1, "c16rtomb reset");
    std::puts("guest unicode character tests passed");
    return 0;
}
