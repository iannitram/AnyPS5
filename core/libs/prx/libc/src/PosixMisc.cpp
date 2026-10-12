#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/GuestTime.hpp"

extern "C" {
GuestTm* APS5_VABI localtime_s_nid_postfix(const std::int64_t* timer, GuestTm* result);
GuestTm* APS5_VABI gmtime_s_nid_postfix(const std::int64_t* timer, GuestTm* result);
std::size_t APS5_VABI fread_nid_postfix(void* buffer, std::size_t size, std::size_t count, FileStream* stream);
std::size_t APS5_VABI fwrite_nid_postfix(const void* buffer, std::size_t size, std::size_t count, FileStream* stream);
void* APS5_VABI memalign_nid_postfix(std::size_t alignment, std::size_t size);
}

namespace {

constexpr int GuestOverflow = 84;
constexpr std::size_t AsctimeSize = 26;
constexpr std::size_t PageSize = 16384;
constexpr int TimeUtc = 1;

}

extern "C" {

GuestTm* APS5_VABI localtime_r_nid_postfix(const std::int64_t* timer, GuestTm* result) {
    if (localtime_s_nid_postfix(timer, result) != nullptr) return result;
    errno = GuestOverflow;
    return nullptr;
}

GuestTm* APS5_VABI gmtime_r_nid_postfix(const std::int64_t* timer, GuestTm* result) {
    if (gmtime_s_nid_postfix(timer, result) != nullptr) return result;
    errno = GuestOverflow;
    return nullptr;
}

// tzcode's asctime_r as FreeBSD 11 ships it: years wider than four digits use the longer
// format and fail with EOVERFLOW, which no longer fits the 26-byte buffer.
char* APS5_VABI asctime_r_nid_postfix(const GuestTm* time, char* buffer) {
    static constexpr const char* Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static constexpr const char* Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (time == nullptr) {
        errno = EINVAL;
        return std::strcpy(buffer, "??? ??? ?? ??:??:?? ????\n");
    }
    const char* day = time->tm_wday >= 0 && time->tm_wday < 7 ? Days[time->tm_wday] : "???";
    const char* month = time->tm_mon >= 0 && time->tm_mon < 12 ? Months[time->tm_mon] : "???";
    const std::string year = std::to_string(static_cast<long long>(time->tm_year) + 1900);
    char result[3 * 2 + 5 * 11 + 3 + 32];
    const char* format = year.size() <= 4 ? "%.3s %.3s%3d %.2d:%.2d:%.2d %-4s\n" : "%.3s %.3s%3d %.2d:%.2d:%.2d     %s\n";
    const int length = std::snprintf(result, sizeof(result), format, day, month, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec, year.c_str());
    if (length < 0 || static_cast<std::size_t>(length) >= AsctimeSize) {
        errno = GuestOverflow;
        return nullptr;
    }
    return std::strcpy(buffer, result);
}

char* APS5_VABI ctime_r_nid_postfix(const std::int64_t* timer, char* buffer) {
    GuestTm local{};
    return asctime_r_nid_postfix(localtime_r_nid_postfix(timer, &local), buffer);
}

int APS5_VABI isascii_nid_postfix(int character) { return (character & ~0x7f) == 0; }

int APS5_VABI getw_nid_postfix(FileStream* stream) {
    int value = 0;
    return fread_nid_postfix(&value, sizeof(value), 1, stream) == 1 ? value : EOF;
}

int APS5_VABI putw_nid_postfix(int value, FileStream* stream) {
    return fwrite_nid_postfix(&value, sizeof(value), 1, stream) == 1 ? 0 : EOF;
}

void* APS5_VABI valloc_nid_postfix(std::size_t size) { return memalign_nid_postfix(PageSize, size); }

int APS5_VABI timespec_get_nid_postfix(KernelTimespec* time, int base) {
    if (base != TimeUtc) return 0;
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now);
    time->tv_sec = seconds.count();
    time->tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now - seconds).count();
    return base;
}

}
