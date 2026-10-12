#include "prx/libc/include/GuestTime.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>

extern "C" {
char* APS5_VABI strptime_nid_postfix(const char*, const char*, GuestTm*);
std::int64_t APS5_VABI timegm_nid_postfix(GuestTm*);
std::int64_t APS5_VABI timeoff_nid_postfix(GuestTm*, std::int64_t);
std::int64_t APS5_VABI timelocal_nid_postfix(GuestTm*);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

struct Case {
    const char* text;
    const char* format;
    long consumed;
    int fields[8];
};

constexpr Case Cases[] = {
    {"2024-03-15 13:45:09", "%Y-%m-%d %H:%M:%S", 19, {9, 45, 13, 15, 2, 124, 4, 74}},
    {"Fri Mar 15 13:45:09 2024", "%c", 24, {9, 45, 13, 15, 2, 124, 5, 74}},
    {"2024-075", "%Y-%j", 8, {0, 0, 0, 15, 2, 124, 4, 74}},
    {"2023 10 1", "%Y %U %w", 9, {0, 0, 0, 6, 2, 123, 1, 64}},
    {"2024-03-15T13:45:09+0530", "%Y-%m-%dT%H:%M:%S%z", 24, {9, 15, 8, 15, 2, 124, 5, 74}},
    {"1710510309", "%s", 10, {9, 45, 13, 15, 2, 124, 5, 74}},
    {"12:30:00 am", "%I:%M:%S %p", 11, {0, 30, 0, 0, 0, 0, 0, 0}},
    {"2023-366", "%Y-%j", 8, {0, 0, 0, 1, 0, 124, 0, 0}},
    {"2023 0 1", "%Y %U %w", -1, {}},
    {"68", "%y", 2, {0, 0, 0, 0, 0, 168, 0, 0}},
};

bool Fields(const GuestTm& time, const int (&expected)[8]) {
    const int actual[8] = {time.tm_sec, time.tm_min, time.tm_hour, time.tm_mday, time.tm_mon, time.tm_year, time.tm_wday, time.tm_yday};
    return std::memcmp(actual, expected, sizeof(actual)) == 0;
}

void CheckParsing() {
    for (const Case& test : Cases) {
        GuestTm time{};
        char* end = strptime_nid_postfix(test.text, test.format, &time);
        if (test.consumed < 0) {
            Require(end == nullptr, test.text);
            continue;
        }
        Require(end == test.text + test.consumed, test.text);
        Require(Fields(time, test.fields), test.text);
    }
    GuestTm time{};
    Require(strptime_nid_postfix("13:61", "%H:%M", &time) == nullptr, "minute out of range");
    Require(strptime_nid_postfix("2024 XYZ", "%Y %Z", &time) == nullptr, "unknown zone name");
    Require(strptime_nid_postfix("x", "%Q", &time) == nullptr, "unknown conversion");
    Require(strptime_nid_postfix("17", "%C", &time) == nullptr, "century before 19");
    time = {};
    Require(strptime_nid_postfix("SEPTEMBER 3", "%b %e", &time) != nullptr && time.tm_mon == 8 && time.tm_mday == 3, "case-insensitive month names");
    time = {};
    Require(strptime_nid_postfix("2024-03-15 UTC", "%F %Z", &time) != nullptr && time.tm_zone != nullptr && std::strcmp(time.tm_zone, "UTC") == 0, "UTC zone converts back");
    try {
        strptime_nid_postfix(nullptr, "%Y", &time);
        Require(false, "null text");
    } catch (const std::invalid_argument&) {
    }
}

void CheckConversions() {
    GuestTm time{};
    time.tm_year = 124;
    time.tm_mon = 0;
    time.tm_mday = 32;
    time.tm_hour = 25;
    time.tm_isdst = 1;
    Require(timegm_nid_postfix(&time) == 1706835600, "timegm normalizes");
    Require(time.tm_mon == 1 && time.tm_mday == 2 && time.tm_hour == 1 && time.tm_wday == 5 && time.tm_yday == 32, "timegm fills the fields");
    Require(time.tm_isdst == 0 && time.tm_gmtoff == 0 && std::strcmp(time.tm_zone, "UTC") == 0, "timegm zone");

    time = {};
    time.tm_year = 70;
    time.tm_mon = 14;
    time.tm_mday = 1;
    time.tm_sec = -1;
    Require(timegm_nid_postfix(&time) == 36633599 && time.tm_year == 71 && time.tm_mon == 1 && time.tm_mday == 28 && time.tm_sec == 59, "negative seconds and month overflow");

    time = {};
    time.tm_year = 69;
    time.tm_mon = 11;
    time.tm_mday = 31;
    time.tm_hour = 23;
    Require(timegm_nid_postfix(&time) == -3600, "before the epoch");

    time = {};
    time.tm_year = 124;
    time.tm_mon = 2;
    time.tm_mday = 15;
    time.tm_hour = 12;
    Require(timeoff_nid_postfix(&time, 3600) == 1710500400, "timeoff subtracts the offset");
    Require(time.tm_hour == 12 && time.tm_gmtoff == 3600 && std::strcmp(time.tm_zone, "   ") == 0, "timeoff keeps the offset time");

    errno = 0;
    Require(timegm_nid_postfix(nullptr) == -1 && errno == EINVAL, "null time");

    time = {};
    time.tm_year = 124;
    time.tm_mon = 2;
    time.tm_mday = 15;
    time.tm_isdst = 1;
    Require(timelocal_nid_postfix(&time) == 1710460800 && time.tm_isdst == 0, "timelocal ignores the given isdst");
}

}

int main() {
#ifdef _WIN32
    _putenv_s("TZ", "UTC");
    _tzset();
#else
    setenv("TZ", "UTC", 1);
    tzset();
#endif
    CheckParsing();
    CheckConversions();
    std::puts("guest time parsing tests passed");
    return 0;
}
