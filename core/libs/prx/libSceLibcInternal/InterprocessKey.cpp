#include <cstdint>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {
int APS5_VABI stat_nid_postfix(const char* path, FileStat* status);

std::int64_t APS5_VABI ftok_nid_postfix(const char* path, int id) {
    FileStat status{};
    if (stat_nid_postfix(path, &status) < 0) return -1;
    return static_cast<std::int64_t>(static_cast<int>(static_cast<unsigned>(id) << 24) | static_cast<int>((status.st_dev & 0xff) << 16) |
                                     static_cast<int>(status.st_ino & 0xffff));
}
}
