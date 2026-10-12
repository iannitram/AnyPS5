#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

struct GuestRegex {
    int magic;
    std::size_t subexpressions;
    const char* end;
    void* guts;
};

struct GuestMatch {
    std::int64_t start;
    std::int64_t end;
};

extern "C" {
int APS5_VABI regcomp_nid_postfix(GuestRegex*, const char*, int);
int APS5_VABI regexec_nid_postfix(const GuestRegex*, const char*, std::size_t, GuestMatch*, int);
std::size_t APS5_VABI regerror_nid_postfix(int, const GuestRegex*, char*, std::size_t);
void APS5_VABI regfree_nid_postfix(GuestRegex*);
}

namespace {
constexpr int Extended = 1;
constexpr int NoSub = 4;
constexpr int NoSpec = 0x10;
constexpr int PendFlag = 0x20;
constexpr int StartEnd = 4;
constexpr int NotBol = 1;
constexpr int NotEol = 2;

void Require(bool value) {
    if (!value) std::abort();
}

struct CompileCase {
    const char* pattern;
    int flags;
    int result;
    long subexpressions;
};

struct ExecCase {
    const char* pattern;
    int flags;
    const char* subject;
    int execFlags;
    std::size_t wanted;
    int result;
    GuestMatch expected[4];
};

struct ErrorCase {
    int code;
    const char* text;
    std::size_t length;
};

const CompileCase CompileCases[] = {
    {"a**", 1, 13, -1},
    {"(", 1, 8, -1},
    {")", 1, 8, -1},
    {"a{1", 1, 9, -1},
    {"a{2,1}", 1, 10, -1},
    {"[a", 1, 7, -1},
    {"[[:foo:]]", 1, 4, -1},
    {"[z-a]", 1, 11, -1},
    {"\\", 1, 5, -1},
    {"\\(a", 0, 8, -1},
    {"\\1", 0, 6, -1},
    {"", 1, 14, -1},
    {"", 0, 14, -1},
    {"a||b", 1, 14, -1},
    {"a|", 1, 14, -1},
    {"(a|)", 1, 14, -1},
    {"[[.xyz.]]", 1, 3, -1},
    {"[[.space.]]", 1, 0, 0},
    {"[[..]]", 1, 17, -1},
    {"a{256}", 1, 10, -1},
    {"a{1,256}", 1, 10, -1},
    {"*a", 1, 13, -1},
    {"+", 1, 13, -1},
    {"^*", 1, 13, -1},
    {"a{", 1, 0, 0},
    {"a{x", 1, 0, 0},
    {"{1}", 1, 13, -1},
    {"a\\{1", 0, 9, -1},
    {"a\\{2,1\\}", 0, 10, -1},
    {"\\(\\)", 0, 0, 1},
    {"()", 1, 0, 1},
    {"(a", 1, 8, -1},
    {"a)", 1, 8, -1},
    {"\\)", 0, 8, -1},
    {"[[:alpha:]", 1, 7, -1},
    {"[[:alpha", 1, 7, -1},
    {"[a-]", 1, 0, 0},
    {"[]a]", 1, 0, 0},
    {"[^]a]", 1, 0, 0},
    {"[--a]", 1, 11, -1},
    {"[a--]", 1, 11, -1},
    {"a\\{0\\}", 0, 14, -1},
    {"a{0}", 1, 14, -1},
    {"(a{0})", 1, 14, -1},
    {"(a)\\1", 1, 0, 1},
    {"\\(a\\)\\2", 0, 6, -1},
    {"\\(a\\1\\)", 0, 6, -1},
    {"[[=a=]]", 1, 0, 0},
    {"[[=-=]]", 1, 3, -1},
    {"[[:-:]]", 1, 4, -1},
    {"[[:", 1, 7, -1},
    {"[a", 16, 0, 0},
    {"a", 17, 16, -1},
};

const ExecCase ExecCases[] = {
    {"abc", 1, "xxabcxx", 0, 1, 0, {{2, 5}}},
    {"abc", 1, "xxabxx", 0, 1, 1, {}},
    {"a*", 1, "", 0, 1, 0, {{0, 0}}},
    {"a*", 1, "baaa", 0, 1, 0, {{0, 0}}},
    {"a+", 1, "baaa", 0, 1, 0, {{1, 4}}},
    {"(a|ab)(c|bcd)(d*)", 1, "abcd", 0, 4, 0, {{0, 4}, {0, 2}, {2, 3}, {3, 4}}},
    {"(a|ab)(bc|c)?", 1, "abc", 0, 3, 0, {{0, 3}, {0, 2}, {2, 3}}},
    {"(a*)*", 1, "b", 0, 2, 0, {{0, 0}, {0, 0}}},
    {"(a*)+", 1, "b", 0, 2, 0, {{0, 0}, {0, 0}}},
    {"(a|b)*", 1, "abab", 0, 2, 0, {{0, 4}, {3, 4}}},
    {"((a)|(b))+", 1, "ab", 0, 4, 0, {{0, 2}, {1, 2}, {-1, -1}, {1, 2}}},
    {"(a)|b", 1, "b", 0, 2, 0, {{0, 1}, {-1, -1}}},
    {"x*", 1, "aaa", 0, 1, 0, {{0, 0}}},
    {"^abc$", 1, "abc", 0, 1, 0, {{0, 3}}},
    {"^abc$", 1, "abc", 1, 1, 1, {}},
    {"^abc$", 1, "abc", 2, 1, 1, {}},
    {"^b", 1, "a\nb", 0, 1, 1, {}},
    {"^b", 9, "a\nb", 0, 1, 0, {{2, 3}}},
    {"a$", 9, "a\nb", 0, 1, 0, {{0, 1}}},
    {"a$", 1, "a\nb", 0, 1, 1, {}},
    {"a.b", 1, "a\nb", 0, 1, 0, {{0, 3}}},
    {"a.b", 9, "a\nb", 0, 1, 1, {}},
    {"a[^x]b", 9, "a\nb", 0, 1, 1, {}},
    {"ABC", 3, "xabcx", 0, 1, 0, {{1, 4}}},
    {"[a-c]+", 3, "xxABCx", 0, 1, 0, {{2, 5}}},
    {"[[:upper:]]+", 3, "abc", 0, 1, 0, {{0, 3}}},
    {"[[:upper:]]+", 1, "abc", 0, 1, 1, {}},
    {"[[:digit:]]+", 1, "ab123c", 0, 1, 0, {{2, 5}}},
    {"[[:alpha:][:digit:]_]+", 1, "-- a_1 --", 0, 1, 0, {{3, 6}}},
    {"\\<b", 1, "a b", 0, 1, 0, {{2, 3}}},
    {"b\\>", 1, "ab b", 0, 1, 0, {{1, 2}}},
    {"[[:<:]]b", 1, "ab b", 0, 1, 0, {{3, 4}}},
    {"b[[:>:]]", 1, "ba b", 0, 1, 0, {{3, 4}}},
    {"\\<a\\>", 1, "ba a", 0, 1, 0, {{3, 4}}},
    {"a{2}", 1, "aaaa", 0, 1, 0, {{0, 2}}},
    {"a{2,3}", 1, "aaaa", 0, 1, 0, {{0, 3}}},
    {"a{2,}", 1, "aaaa", 0, 1, 0, {{0, 4}}},
    {"a{0,1}b", 1, "aab", 0, 1, 0, {{1, 3}}},
    {"(ab){2}", 1, "ababab", 0, 2, 0, {{0, 4}, {2, 4}}},
    {"(a|b){1,2}c", 1, "abbc", 0, 2, 0, {{1, 4}, {2, 3}}},
    {"a\\{2\\}", 0, "aaaa", 0, 1, 0, {{0, 2}}},
    {"\\(a\\)\\(b\\)\\2\\1", 0, "abba", 0, 3, 0, {{0, 4}, {0, 1}, {1, 2}}},
    {"\\(a*\\)b\\1", 0, "aabaa", 0, 2, 0, {{0, 5}, {0, 2}}},
    {"\\(.\\)\\1", 0, "abccd", 0, 2, 0, {{2, 4}, {2, 3}}},
    {"\\(a\\)\\{2\\}", 0, "aa", 0, 2, 0, {{0, 2}, {1, 2}}},
    {"a*", 0, "aaa", 0, 1, 0, {{0, 3}}},
    {"*a", 0, "*a", 0, 1, 0, {{0, 2}}},
    {"a\\|b", 0, "a|b", 0, 1, 0, {{0, 3}}},
    {"a+", 0, "a+", 0, 1, 0, {{0, 2}}},
    {"a?", 0, "a?", 0, 1, 0, {{0, 2}}},
    {"^*", 0, "*", 0, 1, 0, {{0, 1}}},
    {"a^b", 0, "a^b", 0, 1, 0, {{0, 3}}},
    {"a$b", 0, "a$b", 0, 1, 0, {{0, 3}}},
    {"^a$", 0, "a", 0, 1, 0, {{0, 1}}},
    {"a.c", 16, "a.c", 0, 1, 0, {{0, 3}}},
    {"a.c", 16, "abc", 0, 1, 1, {}},
    {"a|b", 16, "xa|b", 0, 1, 0, {{1, 4}}},
    {"(a)(b)?", 1, "a", 0, 3, 0, {{0, 1}, {0, 1}, {-1, -1}}},
    {"(a)(b)?", 1, "a", 0, 0, 0, {}},
    
    {"(foo|foobar)(bar)?", 1, "foobar", 0, 3, 0, {{0, 6}, {0, 6}, {-1, -1}}},
    {"(a+)(a*)", 1, "aaa", 0, 3, 0, {{0, 3}, {0, 3}, {3, 3}}},
    {"(a*)(a+)", 1, "aaa", 0, 3, 0, {{0, 3}, {0, 2}, {2, 3}}},
    {"(a*)(b*)(c*)", 1, "aabbcc", 0, 4, 0, {{0, 6}, {0, 2}, {2, 4}, {4, 6}}},
    {"((a)(b))*", 1, "abab", 0, 4, 0, {{0, 4}, {2, 4}, {2, 3}, {3, 4}}},
    {"\\<\\<a", 1, "a", 0, 1, 1, {}},
    {"a\\>$", 1, "a", 0, 1, 1, {}},
    {"a$\\>", 1, "a", 0, 1, 0, {{0, 1}}},
    {"${2}", 1, "a", 0, 1, 1, {}},
    {"^\\<a", 1, "a", 0, 1, 0, {{0, 1}}},
    {"\\<^a", 1, "a", 0, 1, 1, {}},
    {"x*$", 1, "axx", 0, 1, 0, {{1, 3}}},
    {"$", 1, "abc", 0, 1, 0, {{3, 3}}},
    {"^", 1, "abc", 0, 1, 0, {{0, 0}}},
    {"(^|x)a", 1, "xa", 0, 2, 0, {{0, 2}, {0, 1}}},
    {"(^|x)a", 1, "ba", 0, 2, 1, {}},
    {"a($|x)", 1, "ax", 0, 2, 0, {{0, 2}, {1, 2}}},
    {"[]]", 1, "a]", 0, 1, 0, {{1, 2}}},
    {"[^]]", 1, "]a", 0, 1, 0, {{1, 2}}},
    {"[a-]", 1, "-", 0, 1, 0, {{0, 1}}},
    {"[[.hyphen.]]", 1, "-", 0, 1, 0, {{0, 1}}},
    {"[[=a=]]", 1, "a", 0, 1, 0, {{0, 1}}},
    {"[[.a.]-c]+", 1, "abcd", 0, 1, 0, {{0, 3}}},
    {"a\\.b", 1, "axb a.b", 0, 1, 0, {{4, 7}}},
    {"\\*", 1, "a*", 0, 1, 0, {{1, 2}}},
    {"\\(", 1, "a(", 0, 1, 0, {{1, 2}}},
};

const ErrorCase ErrorCases[] = {
    {1, "regexec() failed to match", 26},
    {2, "invalid regular expression", 27},
    {3, "invalid collating element", 26},
    {4, "invalid character class", 24},
    {5, "trailing backslash (\\)", 23},
    {6, "invalid backreference number", 29},
    {7, "brackets ([ ]) not balanced", 28},
    {8, "parentheses not balanced", 25},
    {9, "braces not balanced", 20},
    {10, "invalid repetition count(s)", 28},
    {11, "invalid character range", 24},
    {12, "out of memory", 14},
    {13, "repetition-operator operand invalid", 36},
    {14, "empty (sub)expression", 22},
    {15, "\"can't happen\" -- you found a bug", 34},
    {16, "invalid argument to regex routine", 34},
    {17, "illegal byte sequence", 22},
    {0, "*** unknown regexp error code ***", 34},
    {18, "*** unknown regexp error code ***", 34},
    {99, "*** unknown regexp error code ***", 34},
    {257, "REG_NOMATCH", 12},
    {273, "REG_ILLSEQ", 11},
    {274, "REG_0x12", 9},
    {256, "REG_0x0", 8},
    {511, "REG_0xff", 9},
};

void CheckCompile() {
    for (const auto& item : CompileCases) {
        GuestRegex regex{};
        const int result = regcomp_nid_postfix(&regex, item.pattern, item.flags);
        Require(result == item.result);
        if (result == 0) {
            Require(regex.magic != 0);
            Require(static_cast<long>(regex.subexpressions) == item.subexpressions);
            regfree_nid_postfix(&regex);
            Require(regex.magic == 0);
        } else {
            Require(regex.magic == 0);
        }
    }
}

void CheckExec() {
    for (const auto& item : ExecCases) {
        GuestRegex regex{};
        Require(regcomp_nid_postfix(&regex, item.pattern, item.flags) == 0);
        std::array<GuestMatch, 4> found;
        found.fill(GuestMatch{-7, -7});
        const int result = regexec_nid_postfix(&regex, item.subject, item.wanted, found.data(), item.execFlags);
        Require(result == item.result);
        if (result == 0) {
            for (std::size_t i = 0; i < item.wanted; ++i) {
                Require(found[i].start == item.expected[i].start);
                Require(found[i].end == item.expected[i].end);
            }
        }
        for (std::size_t i = item.result == 0 ? item.wanted : 0; i < found.size(); ++i) {
            Require(found[i].start == -7 && found[i].end == -7);
        }
        regfree_nid_postfix(&regex);
    }
}

bool Range(const char* pattern, int flags, const char* subject, std::int64_t start, std::int64_t end, int execFlags,
           int expectedResult, std::int64_t expectedStart, std::int64_t expectedEnd) {
    GuestRegex regex{};
    Require(regcomp_nid_postfix(&regex, pattern, flags) == 0);
    GuestMatch found{start, end};
    const int result = regexec_nid_postfix(&regex, subject, 1, &found, execFlags | StartEnd);
    regfree_nid_postfix(&regex);
    return result == expectedResult && found.start == expectedStart && found.end == expectedEnd;
}

void CheckStartEnd() {
    Require(Range("^b", Extended, "abc", 1, 3, 0, 0, 1, 2));
    Require(Range("^b", Extended, "abc", 1, 3, NotBol, 1, 1, 3));
    Require(Range("^b", Extended | 8, "a\nbc", 2, 4, NotBol, 0, 2, 3));
    Require(Range("b$", Extended, "abc", 0, 2, 0, 0, 1, 2));
    Require(Range("b$", Extended, "abc", 0, 2, NotEol, 1, 0, 2));
    Require(Range("b", Extended, "abc", 2, 2, 0, 1, 2, 2));
    Require(Range("x*", Extended, "abc", 3, 3, 0, 0, 3, 3));
    Require(Range("b", Extended, "abc", 2, 1, 0, 16, 2, 1));
    Require(Range("c\\>", Extended, "abc", 0, 3, 0, 0, 2, 3));
    Require(Range("\\<b", Extended, "abc", 1, 3, 0, 0, 1, 2));
    Require(Range("\\<b", Extended, "abc", 1, 3, NotBol, 1, 1, 3));

    GuestRegex regex{};
    Require(regcomp_nid_postfix(&regex, "(b)(c)", Extended) == 0);
    std::array<GuestMatch, 3> found{GuestMatch{1, 4}, GuestMatch{-7, -7}, GuestMatch{-7, -7}};
    Require(regexec_nid_postfix(&regex, "abcd", 3, found.data(), StartEnd) == 0);
    Require(found[0].start == 1 && found[0].end == 3);
    Require(found[1].start == 1 && found[1].end == 2);
    Require(found[2].start == 2 && found[2].end == 3);
    regfree_nid_postfix(&regex);
}

void CheckPatternEnd() {
    const char* pattern = "abczz";
    GuestRegex regex{};
    regex.end = pattern + 3;
    Require(regcomp_nid_postfix(&regex, pattern, Extended | PendFlag) == 0);
    Require(regex.subexpressions == 0);
    GuestMatch found{};
    Require(regexec_nid_postfix(&regex, "zabcz", 1, &found, 0) == 0);
    Require(found.start == 1 && found.end == 4);
    regfree_nid_postfix(&regex);

    GuestRegex backwards{};
    backwards.end = pattern;
    Require(regcomp_nid_postfix(&backwards, pattern + 2, Extended | PendFlag) == 16);
}

void CheckLifetime() {
    GuestRegex failed{};
    Require(regcomp_nid_postfix(&failed, "(", Extended) == 8);
    Require(failed.magic == 0);
    Require(regexec_nid_postfix(&failed, "a", 0, nullptr, 0) == 2);

    GuestRegex released{};
    Require(regcomp_nid_postfix(&released, "a", Extended) == 0);
    regfree_nid_postfix(&released);
    Require(released.magic == 0);
    Require(regexec_nid_postfix(&released, "a", 0, nullptr, 0) == 2);
    regfree_nid_postfix(&released);

    GuestRegex quiet{};
    Require(regcomp_nid_postfix(&quiet, "a(b)", Extended | NoSub) == 0);
    Require(quiet.subexpressions == 1);
    std::array<GuestMatch, 2> found{GuestMatch{-7, -7}, GuestMatch{-7, -7}};
    Require(regexec_nid_postfix(&quiet, "ab", 2, found.data(), 0) == 0);
    Require(found[0].start == -7 && found[0].end == -7 && found[1].start == -7 && found[1].end == -7);
    regfree_nid_postfix(&quiet);

    GuestRegex conflicting{};
    Require(regcomp_nid_postfix(&conflicting, "a", Extended | NoSpec) == 16);
}

void CheckErrors() {
    for (const auto& item : ErrorCases) {
        char buffer[100];
        Require(regerror_nid_postfix(item.code, nullptr, buffer, sizeof(buffer)) == item.length);
        Require(std::strcmp(buffer, item.text) == 0);
    }
    char buffer[64];
    std::memset(buffer, 'x', sizeof(buffer));
    Require(regerror_nid_postfix(1, nullptr, buffer, 5) == 26);
    Require(std::strcmp(buffer, "rege") == 0 && buffer[5] == 'x');
    std::memset(buffer, 'x', sizeof(buffer));
    Require(regerror_nid_postfix(1, nullptr, buffer, 0) == 26);
    Require(buffer[0] == 'x');
    std::memset(buffer, 'x', sizeof(buffer));
    Require(regerror_nid_postfix(1, nullptr, buffer, 26) == 26);
    Require(std::strcmp(buffer, "regexec() failed to match") == 0);
    std::memset(buffer, 'x', sizeof(buffer));
    Require(regerror_nid_postfix(1, nullptr, buffer, 27) == 26);
    Require(std::strcmp(buffer, "regexec() failed to match") == 0);

    GuestRegex named{};
    named.end = "REG_EMPTY";
    Require(regerror_nid_postfix(255, &named, buffer, sizeof(buffer)) == 3);
    Require(std::strcmp(buffer, "14") == 0);
    named.end = "REG_BOGUS";
    Require(regerror_nid_postfix(255, &named, buffer, sizeof(buffer)) == 2);
    Require(std::strcmp(buffer, "0") == 0);
}

void CheckLargeInput() {
    GuestRegex regex{};
    Require(regcomp_nid_postfix(&regex, "(a|b)*c", Extended) == 0);
    std::string subject(200000, 'a');
    subject += "c";
    std::array<GuestMatch, 2> found{};
    Require(regexec_nid_postfix(&regex, subject.c_str(), 2, found.data(), 0) == 0);
    Require(found[0].start == 0 && found[0].end == 200001);
    Require(found[1].start == 199999 && found[1].end == 200000);
    regfree_nid_postfix(&regex);
}
}

int main() {
    CheckCompile();
    CheckExec();
    CheckStartEnd();
    CheckPatternEnd();
    CheckLifetime();
    CheckErrors();
    CheckLargeInput();
    return 0;
}
