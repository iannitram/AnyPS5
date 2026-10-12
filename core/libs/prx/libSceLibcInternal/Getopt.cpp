#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include "prx/libc/include/General.hpp"

struct FileStream;

extern "C" {
extern FileStream* __stderrp_nid_postfix;
const char* APS5_VABI getprogname_nid_postfix();
char* APS5_VABI getenv_nid_postfix(const char* name);
int APS5_VABI fprintf_nid_postfix(FileStream* stream, const char* format, ...);
void APS5_VABI warnx_nid_postfix(const char* format, ...);
}

namespace {

struct GuestOption {
    const char* name;
    int hasArgument;
    int* flag;
    int value;
};

constexpr int NoArgument = 0;
constexpr int RequiredArgument = 1;
constexpr int OptionalArgument = 2;

constexpr int FlagPermute = 0x01;
constexpr int FlagAllArguments = 0x02;
constexpr int FlagLongOnly = 0x04;

constexpr int BadCharacter = '?';
constexpr int InOrder = 1;

enum class DashPrefix { None, Single, Double, W };

char Empty[] = "";

char* shortPlace = Empty;
char* longPlace = Empty;
int nonOptionStart = -1;
int nonOptionEnd = -1;
DashPrefix dashPrefix = DashPrefix::None;
int posixlyCorrect = -1;

std::string Format(const char* format, va_list arguments) {
    va_list copy;
    va_copy(copy, arguments);
    const int length = std::vsnprintf(nullptr, 0, format, copy);
    va_end(copy);
    std::string text(length > 0 ? static_cast<std::size_t>(length) : 0, '\0');
    if (length > 0) std::vsnprintf(text.data(), text.size() + 1, format, arguments);
    return text;
}

void Warn(const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    const std::string message = Format(format, arguments);
    va_end(arguments);
    warnx_nid_postfix("%s", message.c_str());
}

void WarnShort(const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    const std::string message = Format(format, arguments);
    va_end(arguments);
    fprintf_nid_postfix(__stderrp_nid_postfix, "%s: %s\n", getprogname_nid_postfix(), message.c_str());
}

}

extern "C" {

int opterr_nid_postfix = 1;
int optind_nid_postfix = 1;
int optopt_nid_postfix = 0;
int optreset_nid_postfix = 0;
char* optarg_nid_postfix = nullptr;

}

namespace {

int& optind = optind_nid_postfix;
int& opterr = opterr_nid_postfix;
int& optopt = optopt_nid_postfix;
int& optreset = optreset_nid_postfix;
char*& optarg = optarg_nid_postfix;

int GreatestCommonDivisor(int a, int b) {
    int c = a % b;
    while (c != 0) {
        a = b;
        b = c;
        c = a % b;
    }
    return b;
}

void PermuteArguments(int start, int end, int optionsEnd, char* const* arguments) {
    char** vector = const_cast<char**>(arguments);
    const int nonOptions = end - start;
    const int options = optionsEnd - end;
    const int cycles = GreatestCommonDivisor(nonOptions, options);
    const int cycleLength = (optionsEnd - start) / cycles;
    for (int i = 0; i < cycles; i++) {
        const int cycleStart = end + i;
        int position = cycleStart;
        for (int j = 0; j < cycleLength; j++) {
            if (position >= end) position -= nonOptions;
            else position += options;
            char* swap = vector[position];
            vector[position] = vector[cycleStart];
            vector[cycleStart] = swap;
        }
    }
}

bool PrintError(const char* options) { return opterr && *options != ':'; }
int BadArgument(const char* options) { return *options == ':' ? ':' : '?'; }

const char* PrefixText() {
    switch (dashPrefix) {
    case DashPrefix::Single: return "-";
    case DashPrefix::Double: return "--";
    case DashPrefix::W: return "-W ";
    default: return "";
    }
}

int ParseLongOption(char* const* arguments, const char* options, const GuestOption* longOptions, int* index, bool shortToo, int flags) {
    char* current = longPlace;
    const char* prefix = PrefixText();
    int match = -1;
    bool exactMatch = false;
    bool secondPartialMatch = false;

    optind++;

    char* equal = std::strchr(current, '=');
    std::size_t length;
    if (equal != nullptr) {
        length = static_cast<std::size_t>(equal - current);
        equal++;
    } else {
        length = std::strlen(current);
    }

    for (int i = 0; longOptions[i].name; i++) {
        if (std::strncmp(current, longOptions[i].name, length) != 0) continue;
        if (std::strlen(longOptions[i].name) == length) {
            match = i;
            exactMatch = true;
            break;
        }
        if (shortToo && length == 1) continue;
        if (match == -1) {
            match = i;
        } else if ((flags & FlagLongOnly) || longOptions[i].hasArgument != longOptions[match].hasArgument ||
                   longOptions[i].flag != longOptions[match].flag || longOptions[i].value != longOptions[match].value) {
            secondPartialMatch = true;
        }
    }
    if (!exactMatch && secondPartialMatch) {
        if (PrintError(options)) Warn("option `%s%.*s' is ambiguous", prefix, static_cast<int>(length), current);
        optopt = 0;
        return BadCharacter;
    }
    if (match == -1) {
        if (shortToo) {
            --optind;
            return -1;
        }
        if (PrintError(options)) Warn("unrecognized option `%s%s'", prefix, current);
        optopt = 0;
        return BadCharacter;
    }
    const GuestOption& option = longOptions[match];
    if (option.hasArgument == NoArgument && equal != nullptr) {
        if (PrintError(options)) Warn("option `%s%.*s' doesn't allow an argument", prefix, static_cast<int>(length), current);
        optopt = option.flag == nullptr ? option.value : 0;
        return BadCharacter;
    }
    if (option.hasArgument == RequiredArgument || option.hasArgument == OptionalArgument) {
        if (equal != nullptr) optarg = equal;
        else if (option.hasArgument == RequiredArgument) optarg = arguments[optind++];
    }
    if (option.hasArgument == RequiredArgument && optarg == nullptr) {
        if (PrintError(options)) Warn("option `%s%s' requires an argument", prefix, current);
        optopt = option.flag == nullptr ? option.value : 0;
        --optind;
        return BadArgument(options);
    }
    if (index != nullptr) *index = match;
    if (option.flag != nullptr) {
        *option.flag = option.value;
        return 0;
    }
    return option.value;
}

int GetoptInternal(int count, char* const* arguments, const char* options, const GuestOption* longOptions, int* index, int flags) {
    if (options == nullptr) return -1;

    if (optind == 0) optind = optreset = 1;

    if (posixlyCorrect == -1 || optreset) posixlyCorrect = getenv_nid_postfix("POSIXLY_CORRECT") != nullptr;
    if (*options == '-') flags |= FlagAllArguments;
    else if (posixlyCorrect || *options == '+') flags &= ~FlagPermute;
    if (*options == '+' || *options == '-') options++;

    optarg = nullptr;
    if (optreset) nonOptionStart = nonOptionEnd = -1;

    for (;;) {
        if (optreset || !*longPlace) {
            optreset = 0;
            if (optind >= count) {
                longPlace = Empty;
                if (nonOptionEnd != -1) {
                    PermuteArguments(nonOptionStart, nonOptionEnd, optind, arguments);
                    optind -= nonOptionEnd - nonOptionStart;
                } else if (nonOptionStart != -1) {
                    optind = nonOptionStart;
                }
                nonOptionStart = nonOptionEnd = -1;
                return -1;
            }
            longPlace = arguments[optind];
            if (*longPlace != '-' || longPlace[1] == '\0') {
                longPlace = Empty;
                if (flags & FlagAllArguments) {
                    optarg = arguments[optind++];
                    return InOrder;
                }
                if (!(flags & FlagPermute)) return -1;
                if (nonOptionStart == -1) {
                    nonOptionStart = optind;
                } else if (nonOptionEnd != -1) {
                    PermuteArguments(nonOptionStart, nonOptionEnd, optind, arguments);
                    nonOptionStart = optind - (nonOptionEnd - nonOptionStart);
                    nonOptionEnd = -1;
                }
                optind++;
                continue;
            }
            if (nonOptionStart != -1 && nonOptionEnd == -1) nonOptionEnd = optind;

            if (longPlace[1] != '\0' && *++longPlace == '-' && longPlace[1] == '\0') {
                optind++;
                longPlace = Empty;
                if (nonOptionEnd != -1) {
                    PermuteArguments(nonOptionStart, nonOptionEnd, optind, arguments);
                    optind -= nonOptionEnd - nonOptionStart;
                }
                nonOptionStart = nonOptionEnd = -1;
                return -1;
            }
        }
        break;
    }

    int optionCharacter;
    if (longOptions != nullptr && longPlace != arguments[optind] && (*longPlace == '-' || (flags & FlagLongOnly))) {
        bool shortToo = false;
        dashPrefix = DashPrefix::Single;
        if (*longPlace == '-') {
            longPlace++;
            dashPrefix = DashPrefix::Double;
        } else if (*longPlace != ':' && std::strchr(options, *longPlace) != nullptr) {
            shortToo = true;
        }
        optionCharacter = ParseLongOption(arguments, options, longOptions, index, shortToo, flags);
        if (optionCharacter != -1) {
            longPlace = Empty;
            return optionCharacter;
        }
    }

    const char* letter = nullptr;
    optionCharacter = *longPlace++;
    if (optionCharacter == ':' || (optionCharacter == '-' && *longPlace != '\0') ||
        (letter = std::strchr(options, optionCharacter)) == nullptr) {
        if (optionCharacter == '-' && *longPlace == '\0') return -1;
        if (!*longPlace) ++optind;
        if (PrintError(options)) Warn(posixlyCorrect ? "illegal option -- %c" : "invalid option -- %c", optionCharacter);
        optopt = optionCharacter;
        return BadCharacter;
    }
    if (longOptions != nullptr && optionCharacter == 'W' && letter[1] == ';') {
        if (*longPlace) {
        } else if (++optind >= count) {
            longPlace = Empty;
            if (PrintError(options)) Warn("option requires an argument -- %c", optionCharacter);
            optopt = optionCharacter;
            return BadArgument(options);
        } else {
            longPlace = arguments[optind];
        }
        dashPrefix = DashPrefix::W;
        optionCharacter = ParseLongOption(arguments, options, longOptions, index, false, flags);
        longPlace = Empty;
        return optionCharacter;
    }
    if (*++letter != ':') {
        if (!*longPlace) ++optind;
    } else {
        optarg = nullptr;
        if (*longPlace) {
            optarg = longPlace;
        } else if (letter[1] != ':') {
            if (++optind >= count) {
                longPlace = Empty;
                if (PrintError(options)) Warn("option requires an argument -- %c", optionCharacter);
                optopt = optionCharacter;
                return BadArgument(options);
            }
            optarg = arguments[optind];
        }
        longPlace = Empty;
        ++optind;
    }
    return optionCharacter;
}

}

extern "C" {

int APS5_VABI getopt_nid_postfix(int count, char* const* arguments, const char* options) {
    char* option = nullptr;
    if (optreset || *shortPlace == 0) {
        optreset = 0;
        shortPlace = arguments[optind];
        if (optind >= count || *shortPlace++ != '-') {
            shortPlace = Empty;
            return -1;
        }
        optopt = *shortPlace++;
        if (optopt == '-' && *shortPlace == 0) {
            ++optind;
            shortPlace = Empty;
            return -1;
        }
        if (optopt == 0) {
            shortPlace = Empty;
            if (std::strchr(options, '-') == nullptr) return -1;
            optopt = '-';
        }
    } else {
        optopt = *shortPlace++;
    }

    if (optopt == ':' || (option = const_cast<char*>(std::strchr(options, optopt))) == nullptr) {
        if (*shortPlace == 0) ++optind;
        if (opterr && *options != ':') WarnShort("illegal option -- %c", optopt);
        return BadCharacter;
    }

    if (option[1] != ':') {
        optarg = nullptr;
        if (*shortPlace == 0) ++optind;
    } else {
        if (*shortPlace) {
            optarg = shortPlace;
        } else if (option[2] == ':') {
            optarg = nullptr;
        } else if (count > ++optind) {
            optarg = arguments[optind];
        } else {
            shortPlace = Empty;
            if (*options == ':') return ':';
            if (opterr) WarnShort("option requires an argument -- %c", optopt);
            return BadCharacter;
        }
        shortPlace = Empty;
        ++optind;
    }
    return optopt;
}

int APS5_VABI getopt_long_nid_postfix(int count, char* const* arguments, const char* options, const GuestOption* longOptions, int* index) {
    return GetoptInternal(count, arguments, options, longOptions, index, FlagPermute);
}

int APS5_VABI getopt_long_only_nid_postfix(int count, char* const* arguments, const char* options, const GuestOption* longOptions, int* index) {
    return GetoptInternal(count, arguments, options, longOptions, index, FlagPermute | FlagLongOnly);
}

}
