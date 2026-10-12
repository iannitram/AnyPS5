#include "prx/libc/include/General.hpp"

namespace {

constexpr const char* Days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
constexpr const char* ShortDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
constexpr const char* Months[] = {"January", "February", "March", "April", "May", "June",
                                  "July", "August", "September", "October", "November", "December"};
constexpr const char* ShortMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

constexpr int Codeset = 0;
constexpr int DateTimeFormat = 1;
constexpr int DateFormat = 2;
constexpr int TimeFormat = 3;
constexpr int TimeFormatAmPm = 4;
constexpr int AmString = 5;
constexpr int PmString = 6;
constexpr int FirstDay = 7;
constexpr int FirstShortDay = 14;
constexpr int FirstMonth = 21;
constexpr int FirstShortMonth = 33;
constexpr int RadixCharacter = 50;
constexpr int ThousandsSeparator = 51;
constexpr int YesExpression = 52;
constexpr int NoExpression = 53;
constexpr int YesString = 54;
constexpr int NoString = 55;
constexpr int MonthDayOrder = 57;
constexpr int FirstAlternateMonth = 58;

}

extern "C" {

char* APS5_VABI nl_langinfo_nid_postfix(int item) {
    const char* result = "";
    if (item >= FirstDay && item < FirstDay + 7) result = Days[item - FirstDay];
    else if (item >= FirstShortDay && item < FirstShortDay + 7) result = ShortDays[item - FirstShortDay];
    else if (item >= FirstMonth && item < FirstMonth + 12) result = Months[item - FirstMonth];
    else if (item >= FirstShortMonth && item < FirstShortMonth + 12) result = ShortMonths[item - FirstShortMonth];
    else if (item >= FirstAlternateMonth && item < FirstAlternateMonth + 12) result = Months[item - FirstAlternateMonth];
    else {
        switch (item) {
        case Codeset: result = "US-ASCII"; break;
        case DateTimeFormat: result = "%a %b %e %H:%M:%S %Y"; break;
        case DateFormat: result = "%m/%d/%y"; break;
        case TimeFormat: result = "%H:%M:%S"; break;
        case TimeFormatAmPm: result = "%I:%M:%S %p"; break;
        case AmString: result = "AM"; break;
        case PmString: result = "PM"; break;
        case RadixCharacter: result = "."; break;
        case ThousandsSeparator: result = ""; break;
        case YesExpression: result = "^[yY]"; break;
        case NoExpression: result = "^[nN]"; break;
        case YesString: result = "yes"; break;
        case NoString: result = "no"; break;
        case MonthDayOrder: result = "md"; break;
        default: break;
        }
    }
    return const_cast<char*>(result);
}

}
