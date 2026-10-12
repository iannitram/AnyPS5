#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

namespace {
struct Option {
    const char* name;
    int hasArgument;
    int* flag;
    int value;
};
}

extern "C" {
extern int optind_nid_postfix;
extern int opterr_nid_postfix;
extern int optopt_nid_postfix;
extern int optreset_nid_postfix;
extern char* optarg_nid_postfix;
int APS5_VABI getopt_nid_postfix(int, char* const*, const char*);
int APS5_VABI getopt_long_nid_postfix(int, char* const*, const char*, const Option*, int*);
int APS5_VABI getopt_long_only_nid_postfix(int, char* const*, const char*, const Option*, int*);
int APS5_VABI setenv_nid_postfix(const char*, const char*, int);
int APS5_VABI unsetenv_nid_postfix(const char*);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

struct Arguments {
    std::vector<std::string> storage;
    std::vector<char*> pointers;
    explicit Arguments(std::initializer_list<const char*> values) : storage(values.begin(), values.end()) {
        for (auto& value : storage) pointers.push_back(value.data());
        pointers.push_back(nullptr);
    }
    int count() const { return static_cast<int>(storage.size()); }
    char* const* data() { return pointers.data(); }
    std::string at(int index) const { return pointers[index]; }
};

void Reset(bool errors = false) {
    optind_nid_postfix = 1;
    optreset_nid_postfix = 1;
    opterr_nid_postfix = errors ? 1 : 0;
}

bool Argument(const char* expected) {
    if (expected == nullptr) return optarg_nid_postfix == nullptr;
    return optarg_nid_postfix != nullptr && std::strcmp(optarg_nid_postfix, expected) == 0;
}

void CheckShort() {
    Arguments a {"prog", "-a", "-bval", "-b", "x", "-c", "-cy", "-ab", "z", "file", "-a"};
    Reset();
    Require(getopt_nid_postfix(a.count(), a.data(), "ab:c::") == 'a' && Argument(nullptr), "flag option");
    Require(getopt_nid_postfix(a.count(), a.data(), "ab:c::") == 'b' && Argument("val"), "attached argument");
    Require(getopt_nid_postfix(a.count(), a.data(), "ab:c::") == 'b' && Argument("x"), "separate argument");
    Require(getopt_nid_postfix(a.count(), a.data(), "ab:c::") == 'c' && Argument(nullptr), "optional argument absent");
    Require(getopt_nid_postfix(a.count(), a.data(), "ab:c::") == 'c' && Argument("y"), "optional argument attached");
    Require(getopt_nid_postfix(a.count(), a.data(), "ab:c::") == 'a', "grouped flag");
    Require(getopt_nid_postfix(a.count(), a.data(), "ab:c::") == 'b' && Argument("z"), "grouped option with argument");
    Require(getopt_nid_postfix(a.count(), a.data(), "ab:c::") == -1 && optind_nid_postfix == 9, "stops at the first operand");
    Require(a.at(9) == "file", "getopt does not permute");

    Arguments unknown {"prog", "-xa", "-b"};
    Reset();
    Require(getopt_nid_postfix(unknown.count(), unknown.data(), "ab:") == '?' && optopt_nid_postfix == 'x' && optind_nid_postfix == 1, "unknown option");
    Require(getopt_nid_postfix(unknown.count(), unknown.data(), "ab:") == 'a' && optind_nid_postfix == 2, "rest of the group");
    Require(getopt_nid_postfix(unknown.count(), unknown.data(), "ab:") == '?' && optopt_nid_postfix == 'b' && optind_nid_postfix == 3, "missing argument");
    Reset();
    getopt_nid_postfix(unknown.count(), unknown.data(), ":ab:");
    getopt_nid_postfix(unknown.count(), unknown.data(), ":ab:");
    Require(getopt_nid_postfix(unknown.count(), unknown.data(), ":ab:") == ':', "missing argument with leading colon");

    Arguments ends {"prog", "--", "-a"};
    Reset();
    Require(getopt_nid_postfix(ends.count(), ends.data(), "a") == -1 && optind_nid_postfix == 2, "double dash ends options");
    Arguments dash {"prog", "-", "-a"};
    Reset();
    Require(getopt_nid_postfix(dash.count(), dash.data(), "a") == -1 && optind_nid_postfix == 1, "lone dash is an operand");
    Reset();
    Require(getopt_nid_postfix(dash.count(), dash.data(), "a-") == '-' && optind_nid_postfix == 2, "lone dash as an option");
    Arguments colon {"prog", "-:"};
    Reset();
    Require(getopt_nid_postfix(colon.count(), colon.data(), "a:") == '?' && optopt_nid_postfix == ':', "colon is never an option");
}

void CheckLong() {
    int flag = 0;
    const Option options[] = {
        {"verbose", 0, &flag, 7},
        {"version", 0, nullptr, 'V'},
        {"name", 1, nullptr, 'N'},
        {"color", 2, nullptr, 'C'},
        {nullptr, 0, nullptr, 0},
    };
    Arguments a {"prog", "file1", "--verbose", "file2", "-n", "5", "--name=bob", "--", "-x"};
    Reset();
    int index = -1;
    Require(getopt_long_nid_postfix(a.count(), a.data(), "n:", options, &index) == 0 && flag == 7 && index == 0, "flag option");
    Require(getopt_long_nid_postfix(a.count(), a.data(), "n:", options, &index) == 'n' && Argument("5"), "short option between operands");
    Require(getopt_long_nid_postfix(a.count(), a.data(), "n:", options, &index) == 'N' && Argument("bob") && index == 2, "long option with equals");
    Require(getopt_long_nid_postfix(a.count(), a.data(), "n:", options, &index) == -1, "end");
    const char* permuted[] = {"prog", "--verbose", "-n", "5", "--name=bob", "--", "file1", "file2", "-x"};
    for (int i = 0; i < a.count(); i++) Require(a.at(i) == permuted[i], "operands permuted after the options");
    Require(optind_nid_postfix == 6, "optind at the first operand");

    Arguments b {"prog", "--name", "alice", "--color", "--color=red", "--verb", "--ver", "--version=1", "--nope", "--name"};
    Reset();
    Require(getopt_long_nid_postfix(b.count(), b.data(), "", options, nullptr) == 'N' && Argument("alice"), "separate long argument");
    Require(getopt_long_nid_postfix(b.count(), b.data(), "", options, nullptr) == 'C' && Argument(nullptr), "optional long argument absent");
    Require(getopt_long_nid_postfix(b.count(), b.data(), "", options, nullptr) == 'C' && Argument("red"), "optional long argument present");
    flag = 0;
    Require(getopt_long_nid_postfix(b.count(), b.data(), "", options, nullptr) == 0 && flag == 7, "unique abbreviation");
    Require(getopt_long_nid_postfix(b.count(), b.data(), "", options, nullptr) == '?' && optopt_nid_postfix == 0, "ambiguous abbreviation");
    Require(getopt_long_nid_postfix(b.count(), b.data(), "", options, nullptr) == '?' && optopt_nid_postfix == 'V', "argument to a no-argument option");
    Require(getopt_long_nid_postfix(b.count(), b.data(), "", options, nullptr) == '?' && optopt_nid_postfix == 0, "unknown long option");
    Require(getopt_long_nid_postfix(b.count(), b.data(), "", options, nullptr) == '?' && optopt_nid_postfix == 'N', "missing long argument");
    Reset();
    optind_nid_postfix = 9;
    Require(getopt_long_nid_postfix(b.count(), b.data(), ":", options, nullptr) == ':', "missing long argument with leading colon");

    Arguments c {"prog", "-W", "name=w", "-Wversion"};
    Reset();
    Require(getopt_long_nid_postfix(c.count(), c.data(), "W;", options, nullptr) == 'N' && Argument("w"), "-W long option");
    Require(getopt_long_nid_postfix(c.count(), c.data(), "W;", options, nullptr) == 'V', "-W attached long option");
}

void CheckOrdering() {
    const Option options[] = {{"all", 0, nullptr, 'A'}, {nullptr, 0, nullptr, 0}};
    Arguments a {"prog", "file", "-a"};
    Reset();
    Require(getopt_long_nid_postfix(a.count(), a.data(), "+a", options, nullptr) == -1 && optind_nid_postfix == 1, "plus stops at the first operand");
    Reset();
    Require(getopt_long_nid_postfix(a.count(), a.data(), "-a", options, nullptr) == 1 && Argument("file"), "minus returns operands in order");
    Require(getopt_long_nid_postfix(a.count(), a.data(), "-a", options, nullptr) == 'a', "minus then option");
    setenv_nid_postfix("POSIXLY_CORRECT", "1", 1);
    Reset();
    Require(getopt_long_nid_postfix(a.count(), a.data(), "a", options, nullptr) == -1 && optind_nid_postfix == 1, "POSIXLY_CORRECT disables permutation");
    unsetenv_nid_postfix("POSIXLY_CORRECT");
    Reset();
    Require(getopt_long_nid_postfix(a.count(), a.data(), "a", options, nullptr) == 'a', "permutation again");
}

void CheckLongOnly() {
    const Option options[] = {{"name", 1, nullptr, 'N'}, {"all", 0, nullptr, 'A'}, {nullptr, 0, nullptr, 0}};
    Arguments a {"prog", "-name=x", "-n", "5", "-al", "-all"};
    Reset();
    Require(getopt_long_only_nid_postfix(a.count(), a.data(), "n:al", options, nullptr) == 'N' && Argument("x"), "single dash long option");
    Require(getopt_long_only_nid_postfix(a.count(), a.data(), "n:al", options, nullptr) == 'n' && Argument("5"), "single letter stays short");
    Require(getopt_long_only_nid_postfix(a.count(), a.data(), "n:al", options, nullptr) == 'A', "-al abbreviates all");
    Require(getopt_long_only_nid_postfix(a.count(), a.data(), "n:al", options, nullptr) == 'A', "-all is all");
    Require(getopt_long_only_nid_postfix(a.count(), a.data(), "n:al", options, nullptr) == -1, "end");
}

void CheckReset() {
    Arguments a {"prog", "-ab"};
    Reset();
    Require(getopt_nid_postfix(a.count(), a.data(), "ab") == 'a', "first");
    Reset();
    Require(getopt_nid_postfix(a.count(), a.data(), "ab") == 'a', "optreset restarts the group");
    optind_nid_postfix = 0;
    Require(getopt_long_nid_postfix(a.count(), a.data(), "ab", nullptr, nullptr) == 'a', "optind 0 restarts");
}

}

int main() {
    CheckShort();
    CheckLong();
    CheckOrdering();
    CheckLongOnly();
    CheckReset();
    std::puts("guest getopt tests passed");
    return 0;
}
