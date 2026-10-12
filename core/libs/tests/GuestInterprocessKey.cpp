#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

extern "C" {
std::int64_t APS5_VABI ftok_nid_postfix(const char*, int);
int APS5_VABI stat_nid_postfix(const char*, FileStat*);
int* APS5_VABI __error_nid_postfix();
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

}

int main() {
    const std::string name = "anyps5-ftok-" + std::to_string(std::rand());
    std::ofstream(name).put('k');
    FileStat status{};
    Require(stat_nid_postfix(name.c_str(), &status) == 0, "stat");
    const std::int64_t expected = (std::int64_t{'P'} << 24) | ((status.st_dev & 0xff) << 16) | (status.st_ino & 0xffff);
    Require(ftok_nid_postfix(name.c_str(), 'P') == expected, "key layout");
    Require(ftok_nid_postfix(name.c_str(), 'P') == ftok_nid_postfix(name.c_str(), 'P'), "stable key");
    const std::int64_t high = (std::int64_t{0x80} << 24) | ((status.st_dev & 0xff) << 16) | (status.st_ino & 0xffff);
    Require(ftok_nid_postfix(name.c_str(), 0x80) == high && high > 0, "a high id gives a positive key, as ftok.c shifts an unsigned int");
    *__error_nid_postfix() = 0;
    Require(ftok_nid_postfix("anyps5-ftok-missing", 1) == -1 && *__error_nid_postfix() == 2, "missing path");
    std::filesystem::remove(name);
    std::puts("guest interprocess key tests passed");
    return 0;
}
