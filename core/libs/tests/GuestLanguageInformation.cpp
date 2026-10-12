#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" char* APS5_VABI nl_langinfo_nid_postfix(int);

namespace {

void Require(int item, const char* expected) {
    const char* value = nl_langinfo_nid_postfix(item);
    if (value == nullptr || std::strcmp(value, expected) != 0) {
        std::fprintf(stderr, "failed: item %d is \"%s\", expected \"%s\"\n", item, value ? value : "(null)", expected);
        std::abort();
    }
}

}

int main() {
    Require(0, "US-ASCII");
    Require(1, "%a %b %e %H:%M:%S %Y");
    Require(2, "%m/%d/%y");
    Require(3, "%H:%M:%S");
    Require(4, "%I:%M:%S %p");
    Require(5, "AM");
    Require(6, "PM");
    Require(7, "Sunday");
    Require(13, "Saturday");
    Require(14, "Sun");
    Require(20, "Sat");
    Require(21, "January");
    Require(32, "December");
    Require(33, "Jan");
    Require(44, "Dec");
    for (int item = 45; item <= 49; item++) Require(item, "");
    Require(50, ".");
    Require(51, "");
    Require(52, "^[yY]");
    Require(53, "^[nN]");
    Require(54, "yes");
    Require(55, "no");
    Require(56, "");
    Require(57, "md");
    Require(58, "January");
    Require(69, "December");
    Require(70, "");
    Require(-1, "");
    std::puts("guest language information tests passed");
    return 0;
}
