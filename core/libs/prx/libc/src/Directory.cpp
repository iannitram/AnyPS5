#include "prx/libc/include/GuestDirectory.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/FilesystemError.hpp"
#include <dirent.h>
#include <cerrno>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>

extern "C" {
void* APS5_VABI malloc_nid_postfix(std::size_t size);
void* APS5_VABI realloc_nid_postfix(void* pointer, std::size_t size);
void APS5_VABI free_nid_postfix(void* pointer);
int APS5_VABI strcoll_nid_postfix(const char* first, const char* second);
void APS5_VABI qsort_r_nid_postfix(void* base, std::size_t count, std::size_t size, void* thunk, int (APS5_VABI *compare)(void*, const void*, const void*));
}

namespace {
using SelectEntry = int (APS5_VABI *)(const GuestDirectoryEntry*);
using CompareEntries = int (APS5_VABI *)(const GuestDirectoryEntry* const*, const GuestDirectoryEntry* const*);

std::size_t GuestDirectorySize(const GuestDirectoryEntry& entry) {
    return offsetof(GuestDirectoryEntry, name) + ((entry.nameLength + 1 + 3) & ~std::size_t{3});
}

int APS5_VABI CompareThunk(void* thunk, const void* first, const void* second) {
    const CompareEntries compare = *static_cast<CompareEntries*>(thunk);
    return compare(static_cast<const GuestDirectoryEntry* const*>(first), static_cast<const GuestDirectoryEntry* const*>(second));
}

int DirectoryError(int error) {
    switch (error) {
    case ENOENT: return 2;
    case EACCES: return 13;
    case ENOTDIR: return 20;
    case EINVAL: return 22;
    case EMFILE: return 24;
    case ENFILE: return 23;
    case ENOMEM: return 12;
    case ENAMETOOLONG: return 63;
#ifdef ELOOP
    case ELOOP: return 62;
#endif
    default: return 5;
    }
}
#ifdef _WIN32
using NativeDirectory = _WDIR;
NativeDirectory* OpenNative(const std::filesystem::path& path) { return ::_wopendir(path.c_str()); }
int CloseNative(NativeDirectory* native) { return ::_wclosedir(native); }
void RewindNative(NativeDirectory* native) { ::_wrewinddir(native); }
bool ReadNative(NativeDirectory* native, std::filesystem::path& component, std::string& name, std::uint64_t& inode) {
    auto* entry = ::_wreaddir(native);
    if (!entry) return false;
    component = entry->d_name;
    const auto utf8 = component.u8string();
    name.assign(utf8.begin(), utf8.end());
    inode = static_cast<std::uint64_t>(entry->d_ino);
    return true;
}
#else
using NativeDirectory = DIR;
NativeDirectory* OpenNative(const std::filesystem::path& path) { return ::opendir(path.c_str()); }
int CloseNative(NativeDirectory* native) { return ::closedir(native); }
void RewindNative(NativeDirectory* native) { ::rewinddir(native); }
bool ReadNative(NativeDirectory* native, std::filesystem::path& component, std::string& name, std::uint64_t& inode) {
    auto* entry = ::readdir(native);
    if (!entry) return false;
    name = entry->d_name;
    component = name;
    inode = static_cast<std::uint64_t>(entry->d_ino);
    return true;
}
#endif
struct Directory {
    NativeDirectory* native = nullptr;
    std::filesystem::path path;
    GuestDirectoryEntry entry{};
    std::int64_t position = 0;
    std::mutex mutex;
    ~Directory() { if (native) CloseNative(native); }
};
std::uint8_t DirectoryType(const std::filesystem::path& path) {
    std::error_code error;
    switch (std::filesystem::symlink_status(path, error).type()) {
    case std::filesystem::file_type::regular: return 8;
    case std::filesystem::file_type::directory: return 4;
    case std::filesystem::file_type::symlink: return 10;
    case std::filesystem::file_type::block: return 6;
    case std::filesystem::file_type::character: return 2;
    case std::filesystem::file_type::fifo: return 1;
    case std::filesystem::file_type::socket: return 12;
    default: return 0;
    }
}
}

extern "C" {
void* APS5_VABI opendir_nid_postfix(const char* path) {
    if (!path) { errno = 14; return nullptr; }
    if (!*path) { errno = 2; return nullptr; }
    try {
        auto directory = std::make_unique<Directory>();
        if (const int error = ResolvePathError_nid_no_patch(path, directory->path)) { errno = error; return nullptr; }
        directory->native = OpenNative(directory->path);
        if (!directory->native) { errno = DirectoryError(errno); return nullptr; }
        return directory.release();
    } catch (const std::bad_alloc&) { errno = 12; return nullptr; }
      catch (const std::filesystem::filesystem_error&) { errno = 5; return nullptr; }
}

GuestDirectoryEntry* APS5_VABI readdir_nid_postfix(void* handle) {
    if (!handle) { errno = 9; return nullptr; }
    auto& directory = *static_cast<Directory*>(handle);
    std::lock_guard lock(directory.mutex);
    const int savedError = errno;
    errno = 0;
    std::filesystem::path component;
    std::string name;
    std::uint64_t inode = 0;
    if (!ReadNative(directory.native, component, name, inode)) {
        errno = errno ? DirectoryError(errno) : savedError;
        return nullptr;
    }
    const auto length = name.size();
    if (length > 255) { errno = 63; return nullptr; }
    try {
        directory.entry = {};
        // MinGW reports no inode identity (zero); do not fabricate one.
        directory.entry.fileNumber = inode <= UINT32_MAX ? static_cast<std::uint32_t>(inode) : 0;
        directory.entry.recordLength = static_cast<std::uint16_t>(8 + ((length + 1 + 3) & ~3));
        directory.entry.nameLength = static_cast<std::uint8_t>(length);
        directory.entry.type = DirectoryType(directory.path / component);
        std::memcpy(directory.entry.name, name.c_str(), length + 1);
        ++directory.position;
        errno = savedError;
        return &directory.entry;
    } catch (const std::bad_alloc&) { errno = 12; return nullptr; }
      catch (const std::filesystem::filesystem_error&) { errno = 5; return nullptr; }
}

int APS5_VABI closedir_nid_postfix(void* handle) {
    if (!handle) { errno = 9; return -1; }
    std::unique_ptr<Directory> directory(static_cast<Directory*>(handle));
    const int result = CloseNative(directory->native);
    directory->native = nullptr;
    if (result) errno = DirectoryError(errno);
    return result;
}

void APS5_VABI rewinddir_nid_postfix(void* handle) {
    if (!handle) { errno = 9; return; }
    auto& directory = *static_cast<Directory*>(handle);
    std::lock_guard lock(directory.mutex);
    RewindNative(directory.native);
    directory.position = 0;
}

int APS5_VABI readdir_r_nid_postfix(void* handle, GuestDirectoryEntry* entry, GuestDirectoryEntry** result) {
    if (entry == nullptr || result == nullptr) throw std::invalid_argument("readdir_r: null entry or result");
    const int savedError = errno;
    errno = 0;
    GuestDirectoryEntry* next = readdir_nid_postfix(handle);
    if (next != nullptr) std::memcpy(entry, next, GuestDirectorySize(*next));
    if (errno != 0) {
        if (next == nullptr) return errno;
    } else {
        errno = savedError;
    }
    *result = next != nullptr ? entry : nullptr;
    return 0;
}

std::int64_t APS5_VABI telldir_nid_postfix(void* handle) {
    if (!handle) { errno = 9; return -1; }
    auto& directory = *static_cast<Directory*>(handle);
    std::lock_guard lock(directory.mutex);
    return directory.position;
}

void APS5_VABI seekdir_nid_postfix(void* handle, std::int64_t location) {
    if (!handle) { errno = 9; return; }
    auto& directory = *static_cast<Directory*>(handle);
    std::lock_guard lock(directory.mutex);
    if (location < 0 || location == directory.position) return;
    RewindNative(directory.native);
    directory.position = 0;
    std::filesystem::path component;
    std::string name;
    std::uint64_t inode = 0;
    while (directory.position < location && ReadNative(directory.native, component, name, inode)) ++directory.position;
}

int APS5_VABI alphasort_nid_postfix(const GuestDirectoryEntry* const* first, const GuestDirectoryEntry* const* second) {
    return strcoll_nid_postfix((*first)->name, (*second)->name);
}

int APS5_VABI scandir_nid_postfix(const char* path, GuestDirectoryEntry*** list, SelectEntry select, CompareEntries compare) {
    if (list == nullptr) throw std::invalid_argument("scandir: null list");
    void* handle = opendir_nid_postfix(path);
    if (handle == nullptr) return -1;
    std::size_t capacity = 32;
    std::size_t count = 0;
    auto** names = static_cast<GuestDirectoryEntry**>(malloc_nid_postfix(capacity * sizeof(GuestDirectoryEntry*)));
    auto fail = [&]() {
        while (count > 0) free_nid_postfix(names[--count]);
        free_nid_postfix(names);
        closedir_nid_postfix(handle);
        return -1;
    };
    if (names == nullptr) return fail();
    while (GuestDirectoryEntry* next = readdir_nid_postfix(handle)) {
        if (select != nullptr && !select(next)) continue;
        auto* copy = static_cast<GuestDirectoryEntry*>(malloc_nid_postfix(GuestDirectorySize(*next)));
        if (copy == nullptr) return fail();
        copy->fileNumber = next->fileNumber;
        copy->type = next->type;
        copy->recordLength = next->recordLength;
        copy->nameLength = next->nameLength;
        std::memcpy(copy->name, next->name, next->nameLength + 1);
        if (count >= capacity) {
            auto** grown = static_cast<GuestDirectoryEntry**>(realloc_nid_postfix(names, capacity * 2 * sizeof(GuestDirectoryEntry*)));
            if (grown == nullptr) {
                free_nid_postfix(copy);
                return fail();
            }
            names = grown;
            capacity *= 2;
        }
        names[count++] = copy;
    }
    closedir_nid_postfix(handle);
    if (count != 0 && compare != nullptr) qsort_r_nid_postfix(names, count, sizeof(GuestDirectoryEntry*), &compare, CompareThunk);
    *list = names;
    return static_cast<int>(count);
}
}
