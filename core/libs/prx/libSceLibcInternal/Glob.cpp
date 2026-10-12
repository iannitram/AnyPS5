#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestDirectory.hpp"

extern "C" {
void* APS5_VABI opendir_nid_postfix(const char* path);
GuestDirectoryEntry* APS5_VABI readdir_nid_postfix(void* directory);
int APS5_VABI closedir_nid_postfix(void* directory);
int APS5_VABI stat_nid_postfix(const char* path, FileStat* status);
int APS5_VABI lstat_nid_postfix(const char* path, FileStat* status);
char* APS5_VABI getenv_nid_postfix(const char* name);
int APS5_VABI issetugid_nid_postfix(void);
int APS5_VABI strcoll_nid_postfix(const char* first, const char* second);
void* APS5_VABI malloc_nid_postfix(std::size_t size);
void* APS5_VABI realloc_nid_postfix(void* pointer, std::size_t size);
void APS5_VABI free_nid_postfix(void* pointer);
}

namespace {

struct GuestGlob {
    std::size_t pathCount;
    std::size_t matchCount;
    std::size_t offsets;
    int flags;
    char** paths;
    int (APS5_VABI *errorFunction)(const char*, int);
    void (APS5_VABI *closeDirectory)(void*);
    GuestDirectoryEntry* (APS5_VABI *readDirectory)(void*);
    void* (APS5_VABI *openDirectory)(const char*);
    int (APS5_VABI *linkStatus)(const char*, FileStat*);
    int (APS5_VABI *status)(const char*, FileStat*);
};
static_assert(offsetof(GuestGlob, paths) == 32 && sizeof(GuestGlob) == 88);

constexpr int Append = 0x0001;
constexpr int DoOffsets = 0x0002;
constexpr int ErrorFlag = 0x0004;
constexpr int Mark = 0x0008;
constexpr int NoCheck = 0x0010;
constexpr int NoSort = 0x0020;
constexpr int AltDirectoryFunctions = 0x0040;
constexpr int Brace = 0x0080;
constexpr int MagicCharacter = 0x0100;
constexpr int NoMagic = 0x0200;
constexpr int Tilde = 0x0800;
constexpr int Limit = 0x1000;
constexpr int NoEscape = 0x2000;

constexpr int NoSpace = -1;
constexpr int Aborted = -2;
constexpr int NoMatch = -3;

constexpr int GuestNoEntry = 2;
constexpr int GuestTooBig = 7;
constexpr int GuestNotDirectory = 20;
constexpr int GuestNameTooLong = 63;

constexpr std::size_t MaxPath = 1024;
constexpr std::size_t MultibyteMax = 6;
constexpr std::size_t LimitBrace = 128;
constexpr std::size_t LimitPath = 65536;
constexpr std::size_t LimitReaddir = 16384;
constexpr std::size_t LimitStat = 1024;
constexpr std::size_t LimitString = 262144;

constexpr std::uint16_t TypeMask = 0170000;
constexpr std::uint16_t TypeDirectory = 0040000;
constexpr std::uint16_t TypeLink = 0120000;

using Char = std::uint64_t;

constexpr Char Quote = 0x8000000000ULL;
constexpr Char Protect = 0x4000000000ULL;
constexpr Char MaskAll = 0xffffffffffULL;
constexpr Char MaskChar = 0x00ffffffffULL;

constexpr Char Dot = '.';
constexpr Char End = 0;
constexpr Char LeftBracket = '[';
constexpr Char Not = '!';
constexpr Char Question = '?';
constexpr Char Backslash = '\\';
constexpr Char Range = '-';
constexpr Char RightBracket = ']';
constexpr Char Separator = '/';
constexpr Char Star = '*';
constexpr Char TildeCharacter = '~';
constexpr Char LeftBrace = '{';
constexpr Char RightBrace = '}';
constexpr Char Comma = ',';

constexpr Char Character(Char c) { return c & MaskChar; }
constexpr Char Meta(Char c) { return c | Quote; }
constexpr Char Unprotect(Char c) { return c & ~Protect; }
constexpr bool IsMeta(Char c) { return (c & Quote) != 0; }

constexpr Char MetaAll = Meta('*');
constexpr Char MetaEnd = Meta(']');
constexpr Char MetaNot = Meta('!');
constexpr Char MetaOne = Meta('?');
constexpr Char MetaRange = Meta('-');
constexpr Char MetaSet = Meta('[');

struct Limits {
    std::size_t brace = 0;
    std::size_t path = 0;
    std::size_t readdir = 0;
    std::size_t stat = 0;
    std::size_t string = 0;
};

bool ToBytes(const Char* text, char* buffer, std::size_t length) {
    while (length >= 1) {
        *buffer = static_cast<char>(Character(*text));
        if (Character(*text) == End) return false;
        ++text;
        ++buffer;
        --length;
    }
    return true;
}

const Char* FindChar(const Char* text, Char c) {
    do {
        if (*text == c) return text;
    } while (*text++);
    return nullptr;
}

int Extend(const Char* path, GuestGlob* glob, Limits* limits, const char* original) {
    if ((glob->flags & Limit) && glob->matchCount > limits->path) {
        errno = GuestTooBig;
        return NoSpace;
    }
    const std::size_t count = 2 + glob->pathCount + glob->offsets;
    if (count > SIZE_MAX / sizeof(char*)) {
        errno = 12;
        return NoSpace;
    }
    auto** paths = static_cast<char**>(realloc_nid_postfix(glob->paths, count * sizeof(char*)));
    if (paths == nullptr) return NoSpace;
    if (glob->paths == nullptr && glob->offsets > 0) {
        for (std::size_t i = 0; i < glob->offsets; ++i) paths[i] = nullptr;
    }
    glob->paths = paths;

    char* copy;
    if (original != nullptr) {
        const std::size_t length = std::strlen(original) + 1;
        copy = static_cast<char*>(malloc_nid_postfix(length));
        if (copy != nullptr) std::memcpy(copy, original, length);
    } else {
        const Char* p = path;
        while (*p++ != End) {}
        const auto length = static_cast<std::size_t>(p - path);
        copy = static_cast<char*>(malloc_nid_postfix(length));
        if (copy != nullptr && ToBytes(path, copy, length)) {
            free_nid_postfix(copy);
            errno = GuestTooBig;
            return NoSpace;
        }
    }
    if (copy != nullptr) {
        limits->string += std::strlen(copy) + 1;
        if ((glob->flags & Limit) && limits->string >= LimitString) {
            free_nid_postfix(copy);
            errno = GuestTooBig;
            return NoSpace;
        }
        paths[glob->offsets + glob->pathCount++] = copy;
    }
    paths[glob->offsets + glob->pathCount] = nullptr;
    return copy == nullptr ? NoSpace : 0;
}

int NoMatchResult(GuestGlob* glob, Limits* limits, const char* original) {
    if ((glob->flags & NoCheck) || ((glob->flags & NoMagic) && !(glob->flags & MagicCharacter)))
        return Extend(nullptr, glob, limits, original);
    return NoMatch;
}

int AbortResult(GuestGlob* glob, int error, char* buffer) {
    if ((glob->errorFunction != nullptr && glob->errorFunction(buffer, error)) || (glob->flags & ErrorFlag)) return Aborted;
    return 0;
}

bool Match(Char* name, Char* pattern, Char* patternEnd) {
    while (pattern < patternEnd) {
        Char c = *pattern++;
        switch (c & MaskAll) {
        case MetaAll:
            if (pattern == patternEnd) return true;
            do {
                if (Match(name, pattern, patternEnd)) return true;
            } while (*name++ != End);
            return false;
        case MetaOne:
            if (*name++ == End) return false;
            break;
        case MetaSet: {
            bool ok = false;
            const Char k = *name++;
            if (k == End) return false;
            const bool negate = (*pattern & MaskAll) == MetaNot;
            if (negate) ++pattern;
            while (((c = *pattern++) & MaskAll) != MetaEnd) {
                if ((*pattern & MaskAll) == MetaRange) {
                    if (Character(c) <= Character(k) && Character(k) <= Character(pattern[1])) ok = true;
                    pattern += 2;
                } else if (c == k) {
                    ok = true;
                }
            }
            if (ok == negate) return false;
            break;
        }
        default:
            if (*name++ != c) return false;
            break;
        }
    }
    return *name == End;
}

void* OpenDirectory(Char* path, GuestGlob* glob) {
    char buffer[MaxPath + MultibyteMax - 1];
    if (*path == End) {
        std::strcpy(buffer, ".");
    } else if (ToBytes(path, buffer, sizeof(buffer))) {
        errno = GuestNameTooLong;
        return nullptr;
    }
    if (glob->flags & AltDirectoryFunctions) return glob->openDirectory(buffer);
    return opendir_nid_postfix(buffer);
}

int LinkStatus(Char* path, FileStat* status, GuestGlob* glob) {
    char buffer[MaxPath + MultibyteMax - 1];
    if (ToBytes(path, buffer, sizeof(buffer))) {
        errno = GuestNameTooLong;
        return -1;
    }
    if (glob->flags & AltDirectoryFunctions) return glob->linkStatus(buffer, status);
    return lstat_nid_postfix(buffer, status);
}

int Status(Char* path, FileStat* status, GuestGlob* glob) {
    char buffer[MaxPath + MultibyteMax - 1];
    if (ToBytes(path, buffer, sizeof(buffer))) {
        errno = GuestNameTooLong;
        return -1;
    }
    if (glob->flags & AltDirectoryFunctions) return glob->status(buffer, status);
    return stat_nid_postfix(buffer, status);
}

int Glob2(Char* pathStart, Char* pathEnd, Char* pathLast, Char* pattern, GuestGlob* glob, Limits* limits);

int Glob3(Char* pathStart, Char* pathEnd, Char* pathLast, Char* pattern, Char* rest, GuestGlob* glob, Limits* limits) {
    char buffer[MaxPath + MultibyteMax - 1];
    if (pathEnd > pathLast) {
        errno = GuestTooBig;
        return NoSpace;
    }
    *pathEnd = End;
    if (glob->errorFunction != nullptr && ToBytes(pathStart, buffer, sizeof(buffer))) {
        errno = GuestTooBig;
        return NoSpace;
    }

    const int savedError = errno;
    errno = 0;
    void* directory = OpenDirectory(pathStart, glob);
    if (directory == nullptr) {
        if (errno == GuestNoEntry || errno == GuestNotDirectory) return 0;
        const int result = AbortResult(glob, errno, buffer);
        if (errno == 0) errno = savedError;
        return result;
    }

    int result = 0;
    errno = 0;
    GuestDirectoryEntry* entry;
    while ((entry = (glob->flags & AltDirectoryFunctions) ? glob->readDirectory(directory) : readdir_nid_postfix(directory)) != nullptr) {
        if ((glob->flags & Limit) && limits->readdir++ >= LimitReaddir) {
            errno = GuestTooBig;
            result = NoSpace;
            break;
        }
        if (entry->name[0] == '.' && Unprotect(*pattern) != Dot) {
            errno = 0;
            continue;
        }
        Char* destination = pathEnd;
        const char* source = entry->name;
        bool tooLong = true;
        while (destination <= pathLast) {
            if ((*destination++ = static_cast<unsigned char>(*source)) == End) {
                tooLong = false;
                break;
            }
            ++source;
        }
        if (tooLong && (result = AbortResult(glob, GuestNameTooLong, buffer))) {
            errno = GuestNameTooLong;
            break;
        }
        if (tooLong || !Match(pathEnd, pattern, rest)) {
            *pathEnd = End;
            errno = 0;
            continue;
        }
        if (errno == 0) errno = savedError;
        result = Glob2(pathStart, --destination, pathLast, rest, glob, limits);
        if (result) break;
        errno = 0;
    }

    const int closeError = errno;
    if (glob->flags & AltDirectoryFunctions) glob->closeDirectory(directory);
    else closedir_nid_postfix(directory);
    errno = closeError;

    if (result) return result;
    if (entry == nullptr && errno != 0 && (result = AbortResult(glob, errno, buffer))) return result;
    if (errno == 0) errno = savedError;
    return 0;
}

int Glob2(Char* pathStart, Char* pathEnd, Char* pathLast, Char* pattern, GuestGlob* glob, Limits* limits) {
    for (bool anyMeta = false;;) {
        if (*pattern == End) {
            *pathEnd = End;
            FileStat status{};
            if (LinkStatus(pathStart, &status, glob)) return 0;
            if ((glob->flags & Limit) && limits->stat++ >= LimitStat) {
                errno = GuestTooBig;
                return NoSpace;
            }
            if ((glob->flags & Mark) && Unprotect(pathEnd[-1]) != Separator &&
                ((status.st_mode & TypeMask) == TypeDirectory ||
                 ((status.st_mode & TypeMask) == TypeLink && Status(pathStart, &status, glob) == 0 && (status.st_mode & TypeMask) == TypeDirectory))) {
                if (pathEnd + 1 > pathLast) {
                    errno = GuestTooBig;
                    return NoSpace;
                }
                *pathEnd++ = Separator;
                *pathEnd = End;
            }
            ++glob->matchCount;
            return Extend(pathStart, glob, limits, nullptr);
        }

        Char* q = pathEnd;
        Char* p = pattern;
        while (*p != End && Unprotect(*p) != Separator) {
            if (IsMeta(*p)) anyMeta = true;
            if (q + 1 > pathLast) {
                errno = GuestTooBig;
                return NoSpace;
            }
            *q++ = *p++;
        }

        if (anyMeta) return Glob3(pathStart, pathEnd, pathLast, pattern, p, glob, limits);
        pathEnd = q;
        pattern = p;
        while (Unprotect(*pattern) == Separator) {
            if (pathEnd + 1 > pathLast) {
                errno = GuestTooBig;
                return NoSpace;
            }
            *pathEnd++ = *pattern++;
        }
    }
}

int Glob1(Char* pattern, GuestGlob* glob, Limits* limits) {
    Char path[MaxPath];
    if (*pattern == End) return 0;
    return Glob2(path, path, path + MaxPath - 1, pattern, glob, limits);
}

const Char* ExpandTilde(const Char* pattern, Char* buffer, std::size_t length, GuestGlob* glob) {
    if (*pattern != TildeCharacter || !(glob->flags & Tilde)) return pattern;
    Char* last = &buffer[length - 1];
    const Char* p = pattern + 1;
    Char* b = buffer;
    for (; b < last && *p != End && Unprotect(*p) != Separator; *b++ = *p++) {}
    if (*p != End && Unprotect(*p) != Separator) return nullptr;
    *b = End;

    const char* home = nullptr;
    if (buffer[0] == End) {
        if (issetugid_nid_postfix() != 0 || (home = getenv_nid_postfix("HOME")) == nullptr) return pattern;
    } else {
        return pattern;
    }

    b = buffer;
    const char* source = home;
    for (; b < last && *source != '\0'; ++b, ++source) *b = static_cast<unsigned char>(*source) | Protect;
    if (*source != '\0') return nullptr;

    if (*p != End) {
        bool tooLong = true;
        while (b <= last) {
            if ((*b++ = *p++) == End) {
                tooLong = false;
                break;
            }
        }
        if (tooLong) return nullptr;
    } else {
        *b = End;
    }
    return buffer;
}

int Finish(GuestGlob* glob, Limits* limits, std::size_t oldCount, const char* original) {
    if (glob->pathCount == oldCount) return NoMatchResult(glob, limits, original);
    if (!(glob->flags & NoSort)) {
        char** first = glob->paths + glob->offsets + oldCount;
        std::sort(first, glob->paths + glob->offsets + glob->pathCount,
                  [](const char* a, const char* b) { return strcoll_nid_postfix(a, b) < 0; });
    }
    return 0;
}

int Glob0(const Char* pattern, GuestGlob* glob, Limits* limits, const char* original) {
    Char buffer[MaxPath];
    const Char* next = ExpandTilde(pattern, buffer, MaxPath, glob);
    if (next == nullptr) {
        errno = GuestTooBig;
        return NoSpace;
    }
    const std::size_t oldCount = glob->pathCount;
    Char compiled[MaxPath];
    Char* out = compiled;
    Char c;
    while ((c = *next++) != End) {
        switch (c) {
        case LeftBracket:
            c = *next;
            if (c == Not) ++next;
            if (*next == End || FindChar(next + 1, RightBracket) == nullptr) {
                *out++ = LeftBracket;
                if (c == Not) --next;
                break;
            }
            *out++ = MetaSet;
            if (c == Not) *out++ = MetaNot;
            c = *next++;
            do {
                *out++ = Character(c);
                if (*next == Range && (c = next[1]) != RightBracket) {
                    *out++ = MetaRange;
                    *out++ = Character(c);
                    next += 2;
                }
            } while ((c = *next++) != RightBracket);
            glob->flags |= MagicCharacter;
            *out++ = MetaEnd;
            break;
        case Question:
            glob->flags |= MagicCharacter;
            *out++ = MetaOne;
            break;
        case Star:
            glob->flags |= MagicCharacter;
            if (out == compiled || out[-1] != MetaAll) *out++ = MetaAll;
            break;
        default:
            *out++ = Character(c);
            break;
        }
    }
    *out = End;

    if (const int error = Glob1(compiled, glob, limits)) return error;
    if (original != nullptr) return Finish(glob, limits, oldCount, original);
    return 0;
}

int ExpandBraces1(const Char* pattern, GuestGlob* glob, Limits* limits);

int ExpandBraces2(const Char* brace, const Char* pattern, GuestGlob* glob, Limits* limits) {
    Char buffer[MaxPath];
    Char* lm = buffer;
    const Char* pm = pattern;
    for (; pm != brace; *lm++ = *pm++) {}
    *lm = End;
    Char* ls = lm;

    int depth = 0;
    const Char* pe = ++brace;
    for (; *pe != End; pe++) {
        if (*pe == LeftBracket) {
            for (pm = pe++; *pe != RightBracket && *pe != End; pe++) {}
            if (*pe == End) pe = pm;
        } else if (*pe == LeftBrace) {
            depth++;
        } else if (*pe == RightBrace) {
            if (depth == 0) break;
            depth--;
        }
    }
    if (depth != 0 || *pe == End) return Glob0(pattern, glob, limits, nullptr);

    depth = 0;
    const Char* pl = brace;
    for (pm = brace; pm <= pe; pm++) {
        switch (*pm) {
        case LeftBracket: {
            const Char* start = pm++;
            for (; *pm != RightBracket && *pm != End; pm++) {}
            if (*pm == End) pm = start;
            break;
        }
        case LeftBrace:
            depth++;
            break;
        case RightBrace:
            if (depth) {
                depth--;
                break;
            }
            [[fallthrough]];
        case Comma:
            if (depth && *pm == Comma) break;
            for (lm = ls; pl < pm; *lm++ = *pl++) {}
            for (pl = pe + 1; (*lm++ = *pl++) != End;) {}
            if (const int result = ExpandBraces1(buffer, glob, limits)) return result;
            pl = pm + 1;
            break;
        default:
            break;
        }
    }
    return 0;
}

int ExpandBraces1(const Char* pattern, GuestGlob* glob, Limits* limits) {
    if (const Char* brace = FindChar(pattern, LeftBrace)) {
        if ((glob->flags & Limit) && limits->brace++ >= LimitBrace) {
            errno = GuestTooBig;
            return NoSpace;
        }
        return ExpandBraces2(brace, pattern, glob, limits);
    }
    return Glob0(pattern, glob, limits, nullptr);
}

int ExpandBraces0(const Char* pattern, GuestGlob* glob, Limits* limits, const char* original) {
    if (pattern[0] == LeftBrace && pattern[1] == RightBrace && pattern[2] == End) {
        if ((glob->flags & Limit) && limits->brace++ >= LimitBrace) {
            errno = GuestTooBig;
            return NoSpace;
        }
        return Glob0(pattern, glob, limits, original);
    }
    const std::size_t oldCount = glob->pathCount;
    if (const int result = ExpandBraces1(pattern, glob, limits)) return result;
    return Finish(glob, limits, oldCount, original);
}

}

extern "C" {

int APS5_VABI glob_nid_postfix(const char* pattern, int flags, int (APS5_VABI *errorFunction)(const char*, int), GuestGlob* glob) {
    if (pattern == nullptr || glob == nullptr) throw std::invalid_argument("glob: null pattern or result");
    Limits limits;
    if (!(flags & Append)) {
        glob->pathCount = 0;
        glob->paths = nullptr;
        if (!(flags & DoOffsets)) glob->offsets = 0;
    }
    if (flags & Limit) {
        limits.path = glob->matchCount;
        if (limits.path == 0) limits.path = LimitPath;
    }
    glob->flags = flags & ~MagicCharacter;
    glob->errorFunction = errorFunction;
    glob->matchCount = 0;

    Char buffer[MaxPath];
    Char* next = buffer;
    Char* last = next + MaxPath - 1;
    const char* source = pattern;
    bool tooLong = true;
    if (flags & NoEscape) {
        while (next <= last) {
            if (*source == '\0') {
                tooLong = false;
                break;
            }
            *next++ = static_cast<unsigned char>(*source++);
        }
    } else {
        while (next <= last) {
            Char protect = 0;
            if (*source == '\\') {
                if (*++source == '\0') {
                    *next++ = Backslash;
                    continue;
                }
                protect = Protect;
            }
            if (*source == '\0') {
                tooLong = false;
                break;
            }
            *next++ = static_cast<unsigned char>(*source++) | protect;
        }
    }
    if (tooLong) return NoMatchResult(glob, &limits, pattern);
    *next = End;

    if (flags & Brace) return ExpandBraces0(buffer, glob, &limits, pattern);
    return Glob0(buffer, glob, &limits, pattern);
}

void APS5_VABI globfree_nid_postfix(GuestGlob* glob) {
    if (glob == nullptr) throw std::invalid_argument("globfree: null result");
    if (glob->paths == nullptr) return;
    char** path = glob->paths + glob->offsets;
    for (std::size_t i = glob->pathCount; i--; ++path)
        if (*path) free_nid_postfix(*path);
    free_nid_postfix(glob->paths);
    glob->paths = nullptr;
}

}
