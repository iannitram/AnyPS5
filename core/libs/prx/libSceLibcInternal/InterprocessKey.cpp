#include "prx/libSceLibcInternal/include/Api.hpp"
#include <cstdint>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "core/libs/prx/libkernel/include/Api.hpp"

extern "C" {

std::int64_t APS5_VABI ftok_nid_postfix(const char* path, int id) {
    FileStat status{};
    if (stat_nid_postfix(path, &status) < 0) return -1;
    return static_cast<std::int64_t>((static_cast<std::uint32_t>(id) << 24) | ((static_cast<std::uint32_t>(status.st_dev) & 0xff) << 16) |
                                     (static_cast<std::uint32_t>(status.st_ino) & 0xffff));
}
}
