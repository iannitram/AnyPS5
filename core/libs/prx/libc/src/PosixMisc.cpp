#include "prx/libc/include/Api.hpp"
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/GuestTime.hpp"
#include "prx/libc/include/SonyCrt.hpp"

extern "C" {
extern FileStream* __stdinp_nid_postfix;
}

namespace {

constexpr int GuestOverflow = 84;
constexpr std::size_t AsctimeSize = 26;
constexpr std::size_t PageSize = 16384;
constexpr int TimeUtc = 1;
constexpr std::size_t MaxPath = 1024;

std::string Year(int year) {
    int trail = year % 100 + 1900 % 100;
    int lead = year / 100 + 1900 / 100 + trail / 100;
    trail %= 100;
    if (trail < 0 && lead > 0) {
        trail += 100;
        --lead;
    } else if (lead < 0 && trail > 0) {
        trail -= 100;
        ++lead;
    }
    char text[32];
    if (lead == 0 && trail < 0) std::snprintf(text, sizeof(text), "-0%02d", -trail);
    else std::snprintf(text, sizeof(text), "%02d%02d", lead, trail < 0 ? -trail : trail);
    return text;
}

}

extern "C" {



char* APS5_VABI ctime_r_nid_postfix(const std::int64_t* timer, char* buffer) {
    GuestTm local{};
    return asctime_r_nid_postfix(localtime_r_nid_postfix(timer, &local), buffer);
}




void* APS5_VABI valloc_nid_postfix(std::size_t size) { return memalign_nid_postfix(PageSize, size); }

int APS5_VABI timespec_get_nid_postfix(KernelTimespec* time, int base) {
    if (base != TimeUtc) return 0;
    if (time == nullptr) {
        errno = 14;
        return 0;
    }
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now);
    time->tv_sec = seconds.count();
    time->tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now - seconds).count();
    return base;
}

std::size_t APS5_VABI confstr_nid_postfix(int name, char* buffer, std::size_t length) {
    static constexpr const char* Unsupported = "unsupported programming environment";
    const char* value;
    switch (name) {
    case 1: value = "/usr/bin:/bin:/usr/sbin:/sbin"; break;
    case 2: case 3: case 4: case 5: case 6: case 7: case 11: case 12: case 13: value = Unsupported; break;
    case 8: case 9: case 10: value = ""; break;
    case 14: value = "_POSIX_V6_LP64_OFF64"; break;
    default:
        errno = EINVAL;
        return 0;
    }
    const std::size_t size = std::strlen(value) + 1;
    if (length != 0 && buffer != nullptr) {
        const std::size_t copied = size < length ? size - 1 : length - 1;
        std::memcpy(buffer, value, copied);
        buffer[copied] = '\0';
    }
    return size;
}

char* APS5_VABI getwd_nid_postfix(char* buffer) {
    if (char* result = getcwd_nid_postfix(buffer, MaxPath)) return result;
    strerror_r_nid_postfix(errno, buffer, MaxPath);
    return nullptr;
}


}
