#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

struct GuestGlob {
    std::size_t pathCount;
    std::size_t matchCount;
    std::size_t offsets;
    int flags;
    char** paths;
    void* functions[6];
};

}

extern "C" {
int APS5_VABI glob_nid_postfix(const char*, int, int (APS5_VABI *)(const char*, int), GuestGlob*);
void APS5_VABI globfree_nid_postfix(GuestGlob*);
int APS5_VABI setenv_nid_postfix(const char*, const char*, int);
}

namespace {

constexpr int Append = 0x0001;
constexpr int DoOffsets = 0x0002;
constexpr int Mark = 0x0008;
constexpr int NoCheck = 0x0010;
constexpr int NoSort = 0x0020;
constexpr int Brace = 0x0080;
constexpr int MagicCharacter = 0x0100;
constexpr int NoMagic = 0x0200;
constexpr int Tilde = 0x0800;
constexpr int NoEscape = 0x2000;
constexpr int NoMatch = -3;

std::string root;

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

std::vector<std::string> Paths(const GuestGlob& result) {
    std::vector<std::string> paths;
    for (std::size_t i = 0; i < result.pathCount; i++) paths.emplace_back(result.paths[result.offsets + i]);
    return paths;
}

std::vector<std::string> Under(std::vector<std::string> names) {
    for (auto& name : names) name = root + name;
    return names;
}

std::vector<std::string> Glob(const std::string& pattern, int flags = 0) {
    GuestGlob result{};
    const int status = glob_nid_postfix(pattern.c_str(), flags, nullptr, &result);
    if (status != 0) std::fprintf(stderr, "glob %s returned %d\n", pattern.c_str(), status);
    Require(status == 0, pattern.c_str());
    auto paths = Paths(result);
    globfree_nid_postfix(&result);
    return paths;
}

void Touch(const std::string& name) { std::ofstream(root + name).put('x'); }

void CheckMatching() {
    Require(Glob(root + "*") == Under({"a.c", "b.c", "b.h", "dir", "x[1]"}), "star");
    Require(Glob(root + "?.c") == Under({"a.c", "b.c"}), "question");
    Require(Glob(root + "[a-b].[!c]") == Under({"b.h"}), "range and negation");
    Require(Glob(root + "*/*") == Under({"dir/inner.c"}), "nested directory");
    Require(Glob(root + "*", Mark) == Under({"a.c", "b.c", "b.h", "dir/", "x[1]"}), "mark directories");
    Require(Glob(root + "x\\[1]") == Under({"x[1]"}), "escaped bracket");
    Require(Glob(root + "*", 0).size() == 5, "hidden files are skipped");
    Require(Glob(root + ".*") == Under({".", "..", ".hidden"}), "explicit dot");
    Require(Glob(root + "{a,b}.c", Brace) == Under({"a.c", "b.c"}), "brace expansion");
    Require(Glob(root + "{b.h,a.c}", Brace) == Under({"a.c", "b.h"}), "brace results are sorted together");
    Require(Glob("{}", Brace | NoCheck) == std::vector<std::string>{"{}"}, "empty brace is literal");
}

void CheckNoMatch() {
    GuestGlob result{};
    Require(glob_nid_postfix((root + "*.z").c_str(), 0, nullptr, &result) == NoMatch && result.pathCount == 0, "no match");
    globfree_nid_postfix(&result);
    Require(Glob(root + "*.z", NoCheck) == Under({"*.z"}), "nocheck returns the pattern");
    Require(Glob(root + "\\*.z", NoCheck) == Under({"\\*.z"}), "nocheck returns the original pattern");
    Require(Glob(root + "\\a.c", 0) == Under({"a.c"}), "escaped literal matches");
    Require(Glob(root + "\\a.c", NoCheck | NoEscape) == Under({"\\a.c"}), "noescape treats backslash literally");
    Require(Glob(root + "plain", NoMagic) == Under({"plain"}), "nomagic without metacharacters");
    result = {};
    Require(glob_nid_postfix((root + "*.z").c_str(), NoMagic, nullptr, &result) == NoMatch, "nomagic with metacharacters");
    globfree_nid_postfix(&result);
}

void CheckAppendAndOffsets() {
    GuestGlob result{};
    result.offsets = 2;
    Require(glob_nid_postfix((root + "a.c").c_str(), DoOffsets, nullptr, &result) == 0, "first glob");
    Require(result.paths[0] == nullptr && result.paths[1] == nullptr, "reserved slots");
    Require(glob_nid_postfix((root + "b.*").c_str(), DoOffsets | Append, nullptr, &result) == 0, "appended glob");
    Require(result.pathCount == 3 && result.matchCount == 2, "matchCount covers the last call");
    Require(Paths(result) == Under({"a.c", "b.c", "b.h"}), "appended paths");
    Require(result.paths[5] == nullptr, "terminated vector");
    Require((result.flags & MagicCharacter) != 0, "magic flag reported");
    globfree_nid_postfix(&result);
    Require(result.paths == nullptr, "globfree clears the vector");

    result = {};
    Require(glob_nid_postfix((root + "{b.h,b.c,a.c}").c_str(), Brace | NoSort, nullptr, &result) == 0, "unsorted glob");
    Require(Paths(result) == Under({"b.h", "b.c", "a.c"}), "nosort keeps order");
    globfree_nid_postfix(&result);
}

void CheckTilde() {
    Require(setenv_nid_postfix("HOME", (root + "dir").c_str(), 1) == 0, "set HOME");
    Require(Glob("~/*.c", Tilde) == Under({"dir/inner.c"}), "tilde expands HOME");
    Require(Glob("~", Tilde) == Under({"dir"}), "bare tilde");
}

}

int main() {
    std::array<void*, 10> api{};
    ApplicationHeapRegister_nid_no_patch(api.data());
    const auto directory = std::filesystem::path("anyps5-glob-test-" + std::to_string(std::rand()));
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory / "dir");
    root = directory.generic_string() + "/";
    for (const char* name : {"a.c", "b.c", "b.h", "x[1]", ".hidden", "dir/inner.c"}) Touch(name);

    CheckMatching();
    CheckNoMatch();
    CheckAppendAndOffsets();
    CheckTilde();

    std::filesystem::remove_all(directory);
    std::puts("guest glob tests passed");
    return 0;
}
