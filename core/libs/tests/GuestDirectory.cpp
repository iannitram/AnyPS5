#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/GuestDirectory.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <thread>
extern "C" {
void* APS5_VABI opendir_nid_postfix(const char*);
GuestDirectoryEntry* APS5_VABI readdir_nid_postfix(void*);
int APS5_VABI closedir_nid_postfix(void*);
void APS5_VABI rewinddir_nid_postfix(void*);
int* APS5_VABI __error_nid_postfix();
int APS5_VABI readdir_r_nid_postfix(void*, GuestDirectoryEntry*, GuestDirectoryEntry**);
std::int64_t APS5_VABI telldir_nid_postfix(void*);
void APS5_VABI seekdir_nid_postfix(void*, std::int64_t);
int APS5_VABI alphasort_nid_postfix(const GuestDirectoryEntry* const*, const GuestDirectoryEntry* const*);
int APS5_VABI scandir_nid_postfix(const char*, GuestDirectoryEntry***, int (APS5_VABI *)(const GuestDirectoryEntry*),
    int (APS5_VABI *)(const GuestDirectoryEntry* const*, const GuestDirectoryEntry* const*));
void APS5_VABI free_nid_postfix(void*);
}
static void Require(bool value) { if (!value) std::abort(); }
static int APS5_VABI SkipDots(const GuestDirectoryEntry* entry) { return entry->name[0] != '.'; }
static int APS5_VABI ByTypeOnly(const GuestDirectoryEntry* const* first, const GuestDirectoryEntry* const* second) {
    return (*first)->type - (*second)->type;
}
static void CheckPositionsAndScanning(const std::filesystem::path& root) {
    for (const char* name : {"b.txt", "a.txt", "c.txt", "d.txt"}) { std::ofstream file(root / name); file << name; }
    void* directory = opendir_nid_postfix(root.string().c_str());
    Require(directory != nullptr);
    std::vector<std::string> order;
    std::vector<std::int64_t> positions;
    positions.push_back(telldir_nid_postfix(directory));
    while (auto* entry = readdir_nid_postfix(directory)) {
        order.push_back(entry->name);
        positions.push_back(telldir_nid_postfix(directory));
    }
    for (std::size_t i = order.size(); i-- > 0;) {
        seekdir_nid_postfix(directory, positions[i]);
        auto* entry = readdir_nid_postfix(directory);
        Require(entry != nullptr && order[i] == entry->name);
    }
    seekdir_nid_postfix(directory, positions.back());
    Require(readdir_nid_postfix(directory) == nullptr);
    rewinddir_nid_postfix(directory);
    Require(telldir_nid_postfix(directory) == positions.front());

    GuestDirectoryEntry entry{};
    GuestDirectoryEntry* result = nullptr;
    std::size_t count = 0;
    while (readdir_r_nid_postfix(directory, &entry, &result) == 0 && result != nullptr) {
        Require(result == &entry && order[count] == entry.name);
        ++count;
    }
    Require(count == order.size() && result == nullptr);
    Require(closedir_nid_postfix(directory) == 0);

    GuestDirectoryEntry** list = nullptr;
    const int found = scandir_nid_postfix(root.string().c_str(), &list, SkipDots, alphasort_nid_postfix);
    Require(found == 5);
    const char* sorted[] = {"a.txt", "b.txt", "c.txt", "d.txt", "subdirectory"};
    for (int i = 0; i < found; i++) {
        Require(std::strcmp(list[i]->name, sorted[i]) == 0);
        Require(list[i]->nameLength == std::strlen(sorted[i]));
        free_nid_postfix(list[i]);
    }
    free_nid_postfix(list);

    Require(scandir_nid_postfix(root.string().c_str(), &list, nullptr, nullptr) == static_cast<int>(order.size()));
    for (std::size_t i = 0; i < order.size(); i++) {
        Require(order[i] == list[i]->name);
        free_nid_postfix(list[i]);
    }
    free_nid_postfix(list);

    Require(scandir_nid_postfix(root.string().c_str(), &list, SkipDots, ByTypeOnly) == 5);
    Require(std::strcmp(list[0]->name, "subdirectory") == 0 && list[0]->type == 4 && list[4]->type == 8);
    for (int i = 0; i < 5; i++) free_nid_postfix(list[i]);
    free_nid_postfix(list);

    Require(scandir_nid_postfix((root / "missing").string().c_str(), &list, nullptr, nullptr) == -1 && *__error_nid_postfix() == 2);
    for (const char* name : {"b.txt", "a.txt", "c.txt", "d.txt"}) std::filesystem::remove(root / name);
}
static void CheckConcurrentReaddirR(const std::filesystem::path& root) {
    const auto folder = root / "concurrent";
    Require(std::filesystem::create_directory(folder));
    constexpr int FileCount = 400;
    for (int i = 0; i < FileCount; i++) std::ofstream(folder / ("f" + std::to_string(i)));
    void* directory = opendir_nid_postfix(folder.string().c_str());
    Require(directory != nullptr);
    std::vector<std::vector<std::string>> seen(4);
    std::vector<std::thread> readers;
    for (auto& names : seen) {
        readers.emplace_back([directory, &names] {
            GuestDirectoryEntry entry {};
            GuestDirectoryEntry* result = nullptr;
            while (readdir_r_nid_postfix(directory, &entry, &result) == 0 && result != nullptr) {
                Require(std::strlen(entry.name) == entry.nameLength);
                names.emplace_back(entry.name);
            }
        });
    }
    for (auto& reader : readers) reader.join();
    Require(closedir_nid_postfix(directory) == 0);
    std::set<std::string> unique;
    std::size_t total = 0;
    for (const auto& names : seen) {
        total += names.size();
        unique.insert(names.begin(), names.end());
    }
    Require(total == FileCount + 2 && unique.size() == total);
    std::filesystem::remove_all(folder);
}
int main() {
    std::array<void*, 10> api{};
    ApplicationHeapRegister_nid_no_patch(api.data());
    const auto root = std::filesystem::path("anyps5-directory-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Require(std::filesystem::create_directory(root));
    Require(std::filesystem::create_directory(root / "subdirectory"));
    { std::ofstream file(root / "sample.txt"); file << "test"; }
    void* directory = opendir_nid_postfix(root.string().c_str());
    Require(directory != nullptr);
    std::map<std::string, int> entries;
    *__error_nid_postfix() = 13;
    while (auto* entry = readdir_nid_postfix(directory)) {
        Require(entry->nameLength == std::strlen(entry->name));
        Require(entry->recordLength == 8 + ((entry->nameLength + 4) & ~3));
        Require(entry->recordLength <= sizeof(*entry));
        entries[entry->name] = entry->type;
    }
    Require(*__error_nid_postfix() == 13); // EOF preserves errno.
    Require(entries.at("sample.txt") == 8);
    Require(entries.at("subdirectory") == 4);
    Require(entries.at(".") == 4 && entries.at("..") == 4);
    rewinddir_nid_postfix(directory);
    std::size_t count = 0;
    while (readdir_nid_postfix(directory)) ++count;
    Require(count == entries.size());
    Require(closedir_nid_postfix(directory) == 0);
    Require(opendir_nid_postfix((root / "missing").string().c_str()) == nullptr);
    Require(*__error_nid_postfix() == 2);
#ifndef _WIN32
    const auto symlinkLoop = root / "symlink-loop";
    std::filesystem::create_symlink("symlink-loop", symlinkLoop);
    Require(opendir_nid_postfix(symlinkLoop.string().c_str()) == nullptr);
    Require(*__error_nid_postfix() == 62);
    Require(std::filesystem::remove(symlinkLoop));
#endif
    Require(opendir_nid_postfix((root / "sample.txt").string().c_str()) == nullptr);
    Require(*__error_nid_postfix() == 20);
    Require(opendir_nid_postfix("") == nullptr && *__error_nid_postfix() == 2);
    Require(closedir_nid_postfix(nullptr) == -1 && *__error_nid_postfix() == 9);
    const std::u8string folder = u8"セーブ";
    const std::u8string file = u8"été.txt";
    Require(std::filesystem::create_directory(root / std::filesystem::path(folder)));
    { std::ofstream stream(root / std::filesystem::path(folder) / std::filesystem::path(file)); stream << "x"; }
    const std::string guestFolder = root.string() + "/" + std::string(folder.begin(), folder.end());
    void* unicode = opendir_nid_postfix(guestFolder.c_str());
    Require(unicode != nullptr);
    std::map<std::string, int> names;
    while (auto* entry = readdir_nid_postfix(unicode)) {
        Require(entry->nameLength == std::strlen(entry->name));
        names[entry->name] = entry->type;
    }
    Require(names.at(std::string(file.begin(), file.end())) == 8 && names.size() == 3);
    Require(closedir_nid_postfix(unicode) == 0);
    std::filesystem::remove_all(root / std::filesystem::path(folder));
    std::filesystem::remove(root / "sample.txt");
    CheckPositionsAndScanning(root);
    CheckConcurrentReaddirR(root);
    std::filesystem::remove(root / "subdirectory");
    std::filesystem::remove(root);
}
