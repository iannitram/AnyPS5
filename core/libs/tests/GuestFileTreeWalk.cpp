#include "SceTypes.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

struct Entry;

struct Stream {
    Entry* current;
    Entry* children;
    Entry** array;
    std::uint32_t device;
    char* path;
    int rootDescriptor;
    std::size_t pathLength;
    std::size_t arrayItems;
    void* compare;
    int options;
    void* clientPointer;
};

struct Entry {
    Entry* cycle;
    Entry* parent;
    Entry* link;
    long long number;
    void* pointer;
    char* accessPath;
    char* path;
    int error;
    int symbolicDescriptor;
    std::size_t pathLength;
    std::size_t nameLength;
    std::uint32_t inode;
    std::uint32_t device;
    std::uint16_t linkCount;
    long level;
    int info;
    unsigned flags;
    int instruction;
    FileStat* status;
    char* name;
    Stream* fts;
};

struct Position {
    int base;
    int level;
};

using Compare = int (APS5_VABI *)(const Entry* const*, const Entry* const*);

}

extern "C" {
Stream* APS5_VABI fts_open_nid_postfix(char* const*, int, Compare);
Entry* APS5_VABI fts_read_nid_postfix(Stream*);
Entry* APS5_VABI fts_children_nid_postfix(Stream*, int);
int APS5_VABI fts_set_nid_postfix(Stream*, Entry*, int);
int APS5_VABI fts_close_nid_postfix(Stream*);
void APS5_VABI fts_set_clientptr_nid_postfix(Stream*, void*);
void* APS5_VABI fts_get_clientptr_nid_postfix(Stream*);
Stream* APS5_VABI fts_get_stream_nid_postfix(Entry*);
int APS5_VABI nftw_nid_postfix(const char*, int (APS5_VABI *)(const char*, const FileStat*, int, Position*), int, int);
int APS5_VABI ftw_nid_postfix(const char*, int (APS5_VABI *)(const char*, const FileStat*, int), int);
}

namespace {

constexpr int Logical = 0x002;
constexpr int NoStat = 0x008;
constexpr int Physical = 0x010;
constexpr int SeeDot = 0x020;
constexpr int NameOnly = 0x100;

constexpr int InfoDirectory = 1;
constexpr int InfoCycle = 2;
constexpr int InfoDot = 5;
constexpr int InfoPostorder = 6;
constexpr int InfoFile = 8;
constexpr int InfoNoStatOk = 11;
constexpr int InfoLink = 12;
constexpr int InfoLinkNone = 13;
constexpr int Skip = 4;
constexpr int Follow = 2;

constexpr int FtwFile = 0;
constexpr int FtwDirectory = 1;
constexpr int FtwPostorder = 3;
constexpr int FtwLink = 5;
constexpr int FtwLinkNone = 6;
constexpr int FtwPhysical = 0x01;
constexpr int FtwDepth = 0x04;

std::string root;

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

int APS5_VABI ByName(const Entry* const* first, const Entry* const* second) {
    return std::strcmp((*first)->name, (*second)->name);
}

int APS5_VABI ByNameReversed(const Entry* const* first, const Entry* const* second) {
    return -ByName(first, second);
}

std::vector<std::string> Read(Stream* fts) {
    std::vector<std::string> visits;
    while (Entry* entry = fts_read_nid_postfix(fts)) {
        Require(entry->fts == fts && fts_get_stream_nid_postfix(entry) == fts, "entry stream");
        Require(std::string(entry->path).size() == entry->pathLength, "path length");
        visits.push_back(std::to_string(entry->info) + ":" + entry->path);
    }
    return visits;
}

std::vector<std::string> Expect(std::initializer_list<std::pair<int, const char*>> items) {
    std::vector<std::string> visits;
    for (const auto& [info, name] : items) visits.push_back(std::to_string(info) + ":" + root + name);
    return visits;
}

void CheckSortedWalk() {
    char* paths[] = {root.data(), nullptr};
    Stream* fts = fts_open_nid_postfix(paths, Physical, ByName);
    Require(fts != nullptr, "fts_open");
    int marker = 0;
    fts_set_clientptr_nid_postfix(fts, &marker);
    Require(fts_get_clientptr_nid_postfix(fts) == &marker, "client pointer");
    std::vector<std::string> expected = Expect({
        {InfoDirectory, ""}, {InfoFile, "/a.txt"}, {InfoDirectory, "/empty"}, {InfoPostorder, "/empty"},
        {InfoDirectory, "/sub"}, {InfoFile, "/sub/b.txt"}, {InfoDirectory, "/sub/deep"}, {InfoFile, "/sub/deep/c.txt"},
        {InfoPostorder, "/sub/deep"}, {InfoPostorder, "/sub"}, {InfoPostorder, ""}});
#ifndef _WIN32
    expected.insert(expected.begin() + 2, Expect({{InfoLink, "/broken"}}).front());
    expected.insert(expected.begin() + 5, Expect({{InfoLink, "/link"}}).front());
#endif
    const auto visits = Read(fts);
    if (visits != expected)
        for (const auto& visit : visits) std::fprintf(stderr, "%s\n", visit.c_str());
    Require(visits == expected, "sorted physical walk");
    Require(errno == 0, "end of walk clears errno");
    Require(fts_close_nid_postfix(fts) == 0, "fts_close");
}

void CheckSkipAndChildren() {
    char* paths[] = {root.data(), nullptr};
    Stream* fts = fts_open_nid_postfix(paths, Physical, ByNameReversed);
    Entry* top = fts_read_nid_postfix(fts);
    Require(top->info == InfoDirectory && top->level == 0, "root first");
    Entry* children = fts_children_nid_postfix(fts, 0);
    std::vector<std::string> names;
    for (Entry* child = children; child != nullptr; child = child->link) names.emplace_back(child->name);
    std::vector<std::string> expected = {"sub", "empty", "a.txt"};
#ifndef _WIN32
    expected = {"sub", "link", "empty", "broken", "a.txt"};
#endif
    Require(names == expected, "fts_children order");
    Entry* sub = fts_read_nid_postfix(fts);
    Require(std::strcmp(sub->name, "sub") == 0 && sub->level == 1, "first child");
    Require(fts_set_nid_postfix(fts, sub, Skip) == 0, "fts_set skip");
    Require(fts_set_nid_postfix(fts, sub, 99) == 1 && errno == 22, "fts_set invalid");
    fts_set_nid_postfix(fts, sub, Skip);
    Entry* skipped = fts_read_nid_postfix(fts);
    Require(skipped == sub && skipped->info == InfoPostorder, "skipped directory is post-ordered");
    Entry* next = fts_read_nid_postfix(fts);
#ifndef _WIN32
    Require(std::strcmp(next->name, "link") == 0 && next->info == InfoLink, "symbolic link");
    Require(fts_set_nid_postfix(fts, next, Follow) == 0, "fts_set follow");
    next = fts_read_nid_postfix(fts);
    Require(std::strcmp(next->name, "link") == 0 && next->info == InfoDirectory, "followed link is a directory");
    next = fts_read_nid_postfix(fts);
    Require(std::strcmp(next->name, "deep") == 0 && next->path == root + "/link/deep", "followed link is walked");
    while (next != nullptr && !(next->info == InfoPostorder && std::strcmp(next->name, "link") == 0)) next = fts_read_nid_postfix(fts);
    next = fts_read_nid_postfix(fts);
#endif
    Require(std::strcmp(next->name, "empty") == 0, "walk continues after skip");
    fts_close_nid_postfix(fts);

    fts = fts_open_nid_postfix(paths, Physical, ByName);
    fts_read_nid_postfix(fts);
    children = fts_children_nid_postfix(fts, NameOnly);
    Require(children != nullptr && children->info == InfoNoStatOk, "name-only children are not stated");
    Require(std::strcmp(fts_read_nid_postfix(fts)->name, "a.txt") == 0, "read after name-only rebuilds");
    fts_close_nid_postfix(fts);
}

void CheckOptions() {
    char* none[] = {nullptr};
    Require(fts_open_nid_postfix(none, 0, nullptr) == nullptr && errno == 22, "no paths");
    char* paths[] = {root.data(), nullptr};
    Require(fts_open_nid_postfix(paths, 0x400, nullptr) == nullptr && errno == 22, "invalid option");

    Stream* fts = fts_open_nid_postfix(paths, Physical | NoStat, ByName);
    fts_read_nid_postfix(fts);
    Entry* file = fts_read_nid_postfix(fts);
    Require(std::strcmp(file->name, "a.txt") == 0 && file->info == InfoNoStatOk && file->status == nullptr, "nostat skips files");
    fts_close_nid_postfix(fts);

    fts = fts_open_nid_postfix(paths, Physical | SeeDot, ByName);
    fts_read_nid_postfix(fts);
    Entry* dot = fts_read_nid_postfix(fts);
    Require(std::strcmp(dot->name, ".") == 0 && dot->info == InfoDot, "seedot");
    fts_close_nid_postfix(fts);

    const std::string file_path = root + "/a.txt";
    char* files[] = {const_cast<char*>(file_path.c_str()), nullptr};
    fts = fts_open_nid_postfix(files, Physical, nullptr);
    Entry* single = fts_read_nid_postfix(fts);
    Require(single->info == InfoFile && std::strcmp(single->name, "a.txt") == 0 && single->path == file_path, "file root");
    Require(single->status->st_size == 1, "stat data");
    Require(fts_read_nid_postfix(fts) == nullptr, "single root ends");
    fts_close_nid_postfix(fts);
}

std::vector<std::string> nftwVisits;
std::size_t stopAfter = 0;

int APS5_VABI RecordNftw(const char* path, const FileStat*, int type, Position* position) {
    const std::string text(path);
    Require(position->base >= 0 && static_cast<std::size_t>(position->base) <= text.size(), "nftw base");
    nftwVisits.push_back(std::to_string(type) + ":" + std::to_string(position->level) + ":" + text.substr(position->base));
    return stopAfter != 0 && nftwVisits.size() == stopAfter ? 7 : 0;
}

int APS5_VABI RecordFtw(const char* path, const FileStat* status, int type) {
    Require(status != nullptr, "ftw status");
    nftwVisits.push_back(std::to_string(type) + ":" + std::string(path).substr(root.size()));
    return 0;
}

std::size_t IndexOf(const std::string& visit) {
    const auto found = std::find(nftwVisits.begin(), nftwVisits.end(), visit);
    Require(found != nftwVisits.end(), visit.c_str());
    return static_cast<std::size_t>(found - nftwVisits.begin());
}

void CheckNftw() {
    nftwVisits.clear();
    Require(nftw_nid_postfix(root.c_str(), RecordNftw, 4, FtwPhysical) == 0, "nftw");
    Require(IndexOf("1:1:sub") < IndexOf("0:2:b.txt") && IndexOf("1:2:deep") < IndexOf("0:3:c.txt"), "nftw preorder");
    Require(IndexOf("1:0:" + root.substr(root.find_last_of('/') + 1)) == 0, "nftw root base");
#ifndef _WIN32
    IndexOf("5:1:link");
    IndexOf("5:1:broken");
    Require(nftwVisits.size() == 9, "nftw count");
#else
    Require(nftwVisits.size() == 7, "nftw count");
#endif

    nftwVisits.clear();
    Require(nftw_nid_postfix(root.c_str(), RecordNftw, 4, FtwPhysical | FtwDepth) == 0, "nftw depth");
    Require(IndexOf("0:2:b.txt") < IndexOf("3:1:sub") && IndexOf("3:1:empty") < nftwVisits.size(), "nftw postorder");
    Require(nftwVisits.back().rfind("3:0:", 0) == 0, "root last in postorder");

    nftwVisits.clear();
    stopAfter = 3;
    Require(nftw_nid_postfix(root.c_str(), RecordNftw, 1, FtwPhysical) == 7 && nftwVisits.size() == 3, "nftw stops on nonzero");
    stopAfter = 0;
    Require(nftw_nid_postfix(root.c_str(), RecordNftw, 0, 0) == -1 && errno == 22, "nftw descriptors");
    Require(nftw_nid_postfix((root + "/missing").c_str(), RecordNftw, 1, 0) == 0, "missing root is reported");

#ifndef _WIN32
    nftwVisits.clear();
    Require(nftw_nid_postfix(root.c_str(), RecordNftw, 4, 0) == 0, "nftw logical");
    IndexOf("1:1:link");
    IndexOf("0:2:b.txt");
    IndexOf(std::to_string(FtwLinkNone) + ":1:broken");
#endif

    nftwVisits.clear();
    Require(ftw_nid_postfix(root.c_str(), RecordFtw, 2) == 0, "ftw");
    IndexOf(std::to_string(FtwDirectory) + ":/sub");
    IndexOf(std::to_string(FtwFile) + ":/sub/deep/c.txt");
#ifndef _WIN32
    IndexOf(std::to_string(FtwDirectory) + ":/link");
    IndexOf("4:/broken");
    Require(std::none_of(nftwVisits.begin(), nftwVisits.end(), [](const std::string& visit) { return visit[0] == '0' + FtwPostorder || visit[0] == '0' + FtwLink; }), "ftw preorder only");

    std::filesystem::create_directory_symlink("..", root + "/sub/deep/up");
    errno = 0;
    Require(ftw_nid_postfix(root.c_str(), RecordFtw, 2) == -1 && errno == 62, "ftw reports cycles");
    nftwVisits.clear();
    Require(nftw_nid_postfix(root.c_str(), RecordNftw, 4, 0) == 0, "nftw skips cycles");
    char* paths[] = {root.data(), nullptr};
    Stream* fts = fts_open_nid_postfix(paths, Logical, ByName);
    bool cycle = false;
    while (Entry* entry = fts_read_nid_postfix(fts))
        if (entry->info == InfoCycle) cycle = std::strcmp(entry->name, "up") == 0 && entry->cycle != nullptr && entry->cycle->level == 1;
    Require(cycle, "fts reports cycles");
    fts_close_nid_postfix(fts);
    std::filesystem::remove(root + "/sub/deep/up");
#endif
}

}

int main() {
    std::array<void*, 10> api{};
    ApplicationHeapRegister_nid_no_patch(api.data());
    const auto directory = std::filesystem::path("anyps5-fts-test-" + std::to_string(std::rand()));
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory / "sub" / "deep");
    std::filesystem::create_directories(directory / "empty");
    for (const char* name : {"a.txt", "sub/b.txt", "sub/deep/c.txt"}) std::ofstream(directory / name).put('x');
#ifndef _WIN32
    std::filesystem::create_directory_symlink("sub", directory / "link");
    std::filesystem::create_symlink("missing", directory / "broken");
#endif
    root = directory.generic_string();

    CheckSortedWalk();
    CheckSkipAndChildren();
    CheckOptions();
    CheckNftw();

    std::filesystem::remove_all(directory);
    std::puts("guest file tree walk tests passed");
    return 0;
}
