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

extern "C" {
GuestTm* APS5_VABI localtime_s_nid_postfix(const std::int64_t* timer, GuestTm* result);
GuestTm* APS5_VABI gmtime_s_nid_postfix(const std::int64_t* timer, GuestTm* result);
std::size_t APS5_VABI fread_nid_postfix(void* buffer, std::size_t size, std::size_t count, FileStream* stream);
std::size_t APS5_VABI fwrite_nid_postfix(const void* buffer, std::size_t size, std::size_t count, FileStream* stream);
void* APS5_VABI memalign_nid_postfix(std::size_t alignment, std::size_t size);
char* APS5_VABI getcwd_nid_postfix(char* buffer, std::size_t size);
int APS5_VABI strerror_r_nid_postfix(int error, char* buffer, std::size_t length);
int APS5_VABI getc_unlocked_nid_postfix(FileStream* stream);
void APS5_VABI flockfile_nid_postfix(FileStream* stream);
void APS5_VABI funlockfile_nid_postfix(FileStream* stream);
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

GuestTm* APS5_VABI localtime_r_nid_postfix(const std::int64_t* timer, GuestTm* result) {
    if (timer == nullptr || result == nullptr) throw std::invalid_argument("localtime_r: null argument");
    volatile int& hostErrno = errno;
    const int saved = hostErrno;
    GuestTm* converted = localtime_s_nid_postfix(timer, result);
    hostErrno = saved;
    return converted != nullptr ? result : nullptr;
}

GuestTm* APS5_VABI gmtime_r_nid_postfix(const std::int64_t* timer, GuestTm* result) {
    if (timer == nullptr || result == nullptr) throw std::invalid_argument("gmtime_r: null argument");
    volatile int& hostErrno = errno;
    const int saved = hostErrno;
    GuestTm* converted = gmtime_s_nid_postfix(timer, result);
    hostErrno = saved;
    return converted != nullptr ? result : nullptr;
}

char* APS5_VABI asctime_r_nid_postfix(const GuestTm* time, char* buffer) {
    if (buffer == nullptr) throw std::invalid_argument("asctime_r: null buffer");
    static constexpr const char* Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static constexpr const char* Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (time == nullptr) {
        errno = EINVAL;
        return std::strcpy(buffer, "??? ??? ?? ??:??:?? ????\n");
    }
    const char* day = time->tm_wday >= 0 && time->tm_wday < 7 ? Days[time->tm_wday] : "???";
    const char* month = time->tm_mon >= 0 && time->tm_mon < 12 ? Months[time->tm_mon] : "???";
    const std::string year = Year(time->tm_year);
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

char* APS5_VABI gets_nid_postfix(char* buffer) {
    if (buffer == nullptr) throw std::invalid_argument("gets: null buffer");
    FileStream* stream = __stdinp_nid_postfix;
    flockfile_nid_postfix(stream);
    struct Unlock {
        FileStream* stream;
        ~Unlock() { funlockfile_nid_postfix(stream); }
    } unlock{stream};
    static bool warned = false;
    if (!warned) {
        std::fputs("warning: this program uses gets(), which is unsafe.\n", stderr);
        std::fflush(stderr);
        warned = true;
    }
    char* cursor = buffer;
    for (int c; (c = getc_unlocked_nid_postfix(stream)) != '\n';) {
        if (c == EOF) {
            if (cursor == buffer) return nullptr;
            break;
        }
        *cursor++ = static_cast<char>(c);
    }
    *cursor = '\0';
    return buffer;
}

}
