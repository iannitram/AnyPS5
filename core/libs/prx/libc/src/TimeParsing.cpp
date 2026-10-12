#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>

#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestTime.hpp"

extern "C" {
GuestTm* APS5_VABI gmtime_s_nid_postfix(const std::int64_t* timer, GuestTm* result);
GuestTm* APS5_VABI localtime_s_nid_postfix(const std::int64_t* timer, GuestTm* result);
std::int64_t APS5_VABI mktime_nid_postfix(GuestTm* time);
void APS5_VABI tzset_nid_postfix(void);
}

namespace {

constexpr int YearBase = 1900;
constexpr int Sunday = 0;
constexpr int Monday = 1;

constexpr int FlagYear = 1 << 1;
constexpr int FlagMonth = 1 << 2;
constexpr int FlagYearDay = 1 << 3;
constexpr int FlagMonthDay = 1 << 4;
constexpr int FlagWeekDay = 1 << 5;

constexpr const char* MonthAbbreviations[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr const char* MonthNames[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
constexpr const char* DayAbbreviations[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
constexpr const char* DayNames[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
constexpr const char* TimeFormat = "%H:%M:%S";
constexpr const char* DateFormat = "%m/%d/%y";
constexpr const char* DateTimeFormat = "%a %b %e %H:%M:%S %Y";
constexpr const char* Morning = "AM";
constexpr const char* Afternoon = "PM";
constexpr const char* DateCommandFormat = "%a %b %e %H:%M:%S %Z %Y";
constexpr const char* TwelveHourFormat = "%I:%M:%S %p";

constexpr int StartOfMonthTable[26] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365,
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366,
};

char Wildcard[] = "   ";

bool IsLeap(int year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }
int StartOfMonth(int year, int month) { return StartOfMonthTable[(IsLeap(year) ? 13 : 0) + month]; }
bool Space(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }
bool Digit(char c) { return c >= '0' && c <= '9'; }
bool Upper(char c) { return c >= 'A' && c <= 'Z'; }

bool PrefixIgnoringCase(const char* text, const char* prefix, std::size_t length) {
    for (std::size_t i = 0; i < length; i++) {
        if (std::tolower(static_cast<unsigned char>(text[i])) != std::tolower(static_cast<unsigned char>(prefix[i]))) return false;
        if (text[i] == '\0') return false;
    }
    return true;
}

int FirstWeekDayOf(int year) {
    return ((2 * (3 - (year / 100) % 4)) + (year % 100) + ((year % 100) / 4) + (IsLeap(year) ? 6 : 0) + 1) % 7;
}

int ReadNumber(const char*& text, int digits) {
    int value = 0;
    for (; digits && *text != 0 && Digit(*text); text++, digits--) value = value * 10 + (*text - '0');
    return value;
}

std::int64_t DaysFromCivil(std::int64_t year, unsigned month, unsigned day) {
    year -= month <= 2;
    const std::int64_t era = (year >= 0 ? year : year - 399) / 400;
    const auto yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + static_cast<std::int64_t>(dayOfEra) - 719468;
}

std::int64_t FloorDivide(std::int64_t value, std::int64_t divisor) {
    return value / divisor - ((value % divisor != 0) && ((value < 0) != (divisor < 0)));
}

std::int64_t UniversalTime(GuestTm* time, std::int64_t offset) {
    if (time == nullptr) {
        errno = EINVAL;
        return -1;
    }
    time->tm_isdst = 0;
    const std::int64_t months = static_cast<std::int64_t>(time->tm_mon);
    const std::int64_t year = static_cast<std::int64_t>(time->tm_year) + YearBase + FloorDivide(months, 12);
    const auto month = static_cast<unsigned>(months - FloorDivide(months, 12) * 12);
    const std::int64_t days = DaysFromCivil(year, month + 1, 1) + time->tm_mday - 1;
    const std::int64_t seconds = days * 86400 + static_cast<std::int64_t>(time->tm_hour) * 3600 + static_cast<std::int64_t>(time->tm_min) * 60 + time->tm_sec - offset;
    const std::int64_t local = seconds + offset;
    GuestTm result{};
    if (gmtime_s_nid_postfix(&local, &result) == nullptr) return -1;
    result.tm_gmtoff = offset;
    result.tm_zone = offset != 0 ? Wildcard : "UTC";
    *time = result;
    return seconds;
}

const char* Parse(const char* text, const char* format, GuestTm* time, bool* universal) {
    int dayOffset = -1;
    int weekOffset = 0;
    int flags = 0;
    int i;
    std::size_t length;

    const char* pointer = format;
    while (*pointer != 0) {
        char c = *pointer++;
        if (c != '%') {
            if (Space(c)) {
                while (*text != 0 && Space(*text)) text++;
            } else if (c != *text++) {
                return nullptr;
            }
            continue;
        }

        bool alternative = false;
        bool other = false;
    label:
        c = *pointer++;
        switch (c) {
        case '%':
            if (*text++ != '%') return nullptr;
            break;
        case '+':
            text = Parse(text, DateCommandFormat, time, universal);
            if (text == nullptr) return nullptr;
            flags |= FlagWeekDay | FlagMonth | FlagMonthDay | FlagYear;
            break;
        case 'C':
            if (!Digit(*text)) return nullptr;
            i = ReadNumber(text, 2);
            if (i < 19) return nullptr;
            time->tm_year = i * 100 - YearBase;
            flags |= FlagYear;
            break;
        case 'c':
            text = Parse(text, DateTimeFormat, time, universal);
            if (text == nullptr) return nullptr;
            flags |= FlagWeekDay | FlagMonth | FlagMonthDay | FlagYear;
            break;
        case 'D':
            text = Parse(text, "%m/%d/%y", time, universal);
            if (text == nullptr) return nullptr;
            flags |= FlagMonth | FlagMonthDay | FlagYear;
            break;
        case 'E':
            if (alternative || other) break;
            alternative = true;
            goto label;
        case 'O':
            if (alternative || other) break;
            other = true;
            goto label;
        case 'F':
            text = Parse(text, "%Y-%m-%d", time, universal);
            if (text == nullptr) return nullptr;
            flags |= FlagMonth | FlagMonthDay | FlagYear;
            break;
        case 'R':
            text = Parse(text, "%H:%M", time, universal);
            if (text == nullptr) return nullptr;
            break;
        case 'r':
            text = Parse(text, TwelveHourFormat, time, universal);
            if (text == nullptr) return nullptr;
            break;
        case 'T':
            text = Parse(text, "%H:%M:%S", time, universal);
            if (text == nullptr) return nullptr;
            break;
        case 'X':
            text = Parse(text, TimeFormat, time, universal);
            if (text == nullptr) return nullptr;
            break;
        case 'x':
            text = Parse(text, DateFormat, time, universal);
            if (text == nullptr) return nullptr;
            flags |= FlagMonth | FlagMonthDay | FlagYear;
            break;
        case 'j':
            if (!Digit(*text)) return nullptr;
            i = ReadNumber(text, 3);
            if (i < 1 || i > 366) return nullptr;
            time->tm_yday = i - 1;
            flags |= FlagYearDay;
            break;
        case 'M':
        case 'S':
            if (*text == 0 || Space(*text)) break;
            if (!Digit(*text)) return nullptr;
            i = ReadNumber(text, 2);
            if (c == 'M') {
                if (i > 59) return nullptr;
                time->tm_min = i;
            } else {
                if (i > 60) return nullptr;
                time->tm_sec = i;
            }
            break;
        case 'H':
        case 'I':
        case 'k':
        case 'l':
            if (!Digit(*text)) return nullptr;
            i = ReadNumber(text, 2);
            if (c == 'H' || c == 'k') {
                if (i > 23) return nullptr;
            } else if (i > 12) {
                return nullptr;
            }
            time->tm_hour = i;
            break;
        case 'p':
            if (time->tm_hour > 12) return nullptr;
            length = std::strlen(Morning);
            if (PrefixIgnoringCase(text, Morning, length)) {
                if (time->tm_hour == 12) time->tm_hour = 0;
                text += length;
                break;
            }
            length = std::strlen(Afternoon);
            if (PrefixIgnoringCase(text, Afternoon, length)) {
                if (time->tm_hour != 12) time->tm_hour += 12;
                text += length;
                break;
            }
            return nullptr;
        case 'A':
        case 'a':
            for (i = 0; i < 7; i++) {
                length = std::strlen(DayNames[i]);
                if (PrefixIgnoringCase(text, DayNames[i], length)) break;
                length = std::strlen(DayAbbreviations[i]);
                if (PrefixIgnoringCase(text, DayAbbreviations[i], length)) break;
            }
            if (i == 7) return nullptr;
            text += length;
            time->tm_wday = i;
            flags |= FlagWeekDay;
            break;
        case 'U':
        case 'W':
            if (!Digit(*text)) return nullptr;
            i = ReadNumber(text, 2);
            if (i > 53) return nullptr;
            dayOffset = c == 'U' ? Sunday : Monday;
            weekOffset = i;
            break;
        case 'u':
        case 'w':
            if (!Digit(*text)) return nullptr;
            i = *text++ - '0';
            if (i < 0 || i > 7 || (c == 'u' && i < 1) || (c == 'w' && i > 6)) return nullptr;
            time->tm_wday = i % 7;
            flags |= FlagWeekDay;
            break;
        case 'e':
            if (*text != 0 && Space(*text)) text++;
            [[fallthrough]];
        case 'd':
            if (!Digit(*text)) return nullptr;
            i = ReadNumber(text, 2);
            if (i == 0 || i > 31) return nullptr;
            time->tm_mday = i;
            flags |= FlagMonthDay;
            break;
        case 'B':
        case 'b':
        case 'h':
            length = 0;
            for (i = 0; i < 12; i++) {
                if (other) {
                    if (c == 'B') {
                        length = std::strlen(MonthNames[i]);
                        if (PrefixIgnoringCase(text, MonthNames[i], length)) break;
                    }
                } else {
                    length = std::strlen(MonthNames[i]);
                    if (PrefixIgnoringCase(text, MonthNames[i], length)) break;
                }
            }
            if (i == 12 && !other) {
                for (i = 0; i < 12; i++) {
                    length = std::strlen(MonthAbbreviations[i]);
                    if (PrefixIgnoringCase(text, MonthAbbreviations[i], length)) break;
                }
            }
            if (i == 12) return nullptr;
            time->tm_mon = i;
            text += length;
            flags |= FlagMonth;
            break;
        case 'm':
            if (!Digit(*text)) return nullptr;
            i = ReadNumber(text, 2);
            if (i < 1 || i > 12) return nullptr;
            time->tm_mon = i - 1;
            flags |= FlagMonth;
            break;
        case 's': {
            const int saved = errno;
            errno = 0;
            char* end = nullptr;
            const long long value = std::strtoll(text, &end, 10);
            if (errno == ERANGE) {
                errno = saved;
                return nullptr;
            }
            errno = saved;
            text = end;
            const std::int64_t seconds = value;
            if (gmtime_s_nid_postfix(&seconds, time) == nullptr) return nullptr;
            *universal = true;
            flags |= FlagYearDay | FlagWeekDay | FlagMonth | FlagMonthDay | FlagYear;
            break;
        }
        case 'Y':
        case 'y':
            if (*text == 0 || Space(*text)) break;
            if (!Digit(*text)) return nullptr;
            i = ReadNumber(text, c == 'Y' ? 4 : 2);
            if (c == 'Y') i -= YearBase;
            if (c == 'y' && i < 69) i += 100;
            if (i < 0) return nullptr;
            time->tm_year = i;
            flags |= FlagYear;
            break;
        case 'Z': {
            const char* end = text;
            while (*end && Upper(*end)) ++end;
            if (end != text) {
                const std::string zone(text, end);
                tzset_nid_postfix();
#ifdef _WIN32
                char** names = _tzname;
#else
                char** names = ::tzname;
#endif
                if (zone == "GMT" || zone == "UTC") *universal = true;
                else if (zone == names[0]) time->tm_isdst = 0;
                else if (zone == names[1]) time->tm_isdst = 1;
                else return nullptr;
                text = end;
            }
            break;
        }
        case 'z': {
            int sign = 1;
            if (*text != '+') {
                if (*text == '-') sign = -1;
                else return nullptr;
            }
            text++;
            i = 0;
            for (int digits = 4; digits > 0; digits--) {
                if (Digit(*text)) {
                    i = i * 10 + (*text - '0');
                    text++;
                } else if (digits == 2) {
                    i *= 100;
                    break;
                } else {
                    return nullptr;
                }
            }
            if (i > 1400 || (sign == -1 && i > 1200) || (i % 100) >= 60) return nullptr;
            time->tm_hour -= sign * (i / 100);
            time->tm_min -= sign * (i % 100);
            *universal = true;
            break;
        }
        case 'n':
        case 't':
            while (Space(*text)) text++;
            break;
        default:
            return nullptr;
        }
    }

    if (!(flags & FlagYearDay) && (flags & FlagYear)) {
        if ((flags & (FlagMonth | FlagMonthDay)) == (FlagMonth | FlagMonthDay)) {
            time->tm_yday = StartOfMonth(time->tm_year + YearBase, time->tm_mon) + (time->tm_mday - 1);
            flags |= FlagYearDay;
        } else if (dayOffset != -1) {
            const int firstWeekDay = FirstWeekDayOf(time->tm_year + YearBase);
            if (weekOffset == 0 && firstWeekDay == dayOffset) return nullptr;
            const int weekDay = (flags & FlagWeekDay) ? time->tm_wday : dayOffset;
            int yearDay = (7 - firstWeekDay + dayOffset) % 7 + (weekOffset - 1) * 7 + (weekDay - dayOffset + 7) % 7;
            if (yearDay < 0) {
                if (flags & FlagWeekDay) return nullptr;
                yearDay = 0;
            }
            time->tm_yday = yearDay;
            flags |= FlagYearDay;
        }
    }

    if ((flags & (FlagYear | FlagYearDay)) == (FlagYear | FlagYearDay)) {
        if (!(flags & FlagMonth)) {
            i = 0;
            while (i <= 12 && time->tm_yday >= StartOfMonth(time->tm_year + YearBase, i)) i++;
            if (i > 12) {
                i = 1;
                time->tm_yday -= StartOfMonth(time->tm_year + YearBase, 12);
                time->tm_year++;
            }
            time->tm_mon = i - 1;
            flags |= FlagMonth;
        }
        if (!(flags & FlagMonthDay)) {
            time->tm_mday = time->tm_yday - StartOfMonth(time->tm_year + YearBase, time->tm_mon) + 1;
            flags |= FlagMonthDay;
        }
        if (!(flags & FlagWeekDay)) {
            i = 0;
            int weekDay = FirstWeekDayOf(time->tm_year);
            while (i++ <= time->tm_yday) {
                if (weekDay++ >= 6) weekDay = 0;
            }
            time->tm_wday = weekDay;
            flags |= FlagWeekDay;
        }
    }

    return text;
}

}

extern "C" {

std::int64_t APS5_VABI timegm_nid_postfix(GuestTm* time) {
    return UniversalTime(time, 0);
}

std::int64_t APS5_VABI timeoff_nid_postfix(GuestTm* time, std::int64_t offset) {
    return UniversalTime(time, offset);
}

std::int64_t APS5_VABI timelocal_nid_postfix(GuestTm* time) {
    if (time != nullptr) time->tm_isdst = -1;
    return mktime_nid_postfix(time);
}

char* APS5_VABI strptime_nid_postfix(const char* text, const char* format, GuestTm* time) {
    if (text == nullptr || format == nullptr || time == nullptr) throw std::invalid_argument("strptime: null argument");
    bool universal = false;
    const char* result = Parse(text, format, time, &universal);
    if (result != nullptr && universal) {
        const std::int64_t seconds = timegm_nid_postfix(time);
        localtime_s_nid_postfix(&seconds, time);
    }
    return const_cast<char*>(result);
}

}
