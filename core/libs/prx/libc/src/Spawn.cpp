#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "prx/libc/include/General.hpp"

extern "C" {
void* APS5_VABI malloc_nid_postfix(std::size_t size);
void* APS5_VABI calloc_nid_postfix(std::size_t count, std::size_t size);
void APS5_VABI free_nid_postfix(void* pointer);
}

namespace {

struct GuestSignalSet {
    std::uint32_t bits[4];
};

struct GuestSchedulingParameter {
    int priority;
};

// FreeBSD 11 keeps both objects behind the opaque pointer that posix_spawnattr_t and
// posix_spawn_file_actions_t hold, allocated from the guest heap.
struct SpawnAttributes {
    short flags;
    int processGroup;
    GuestSchedulingParameter schedulingParameter;
    int schedulingPolicy;
    GuestSignalSet defaultSignals;
    GuestSignalSet signalMask;
};

enum class FileAction { Open, Duplicate, Close };

struct FileActionEntry {
    FileActionEntry* next;
    FileAction action;
    int descriptor;
    char* path;
    int openFlags;
    std::uint16_t mode;
    int newDescriptor;
};

struct FileActions {
    FileActionEntry* first;
    FileActionEntry** last;
};

constexpr int GuestBadDescriptor = 9;

int Append(FileActions* actions, FileActionEntry* entry) {
    entry->next = nullptr;
    *actions->last = entry;
    actions->last = &entry->next;
    return 0;
}

FileActionEntry* NewEntry(FileAction action, int descriptor) {
    auto* entry = static_cast<FileActionEntry*>(malloc_nid_postfix(sizeof(FileActionEntry)));
    if (entry == nullptr) return nullptr;
    entry->action = action;
    entry->descriptor = descriptor;
    entry->path = nullptr;
    return entry;
}

FileActions* Actions(FileActions** actions) {
    if (actions == nullptr || *actions == nullptr) throw std::invalid_argument("posix_spawn_file_actions: null object");
    return *actions;
}

SpawnAttributes* Attributes(SpawnAttributes* const* attributes) {
    if (attributes == nullptr || *attributes == nullptr) throw std::invalid_argument("posix_spawnattr: null object");
    return *attributes;
}

}

extern "C" {

// FreeBSD 11 returns -1, not an error number, when the list head cannot be allocated.
int APS5_VABI posix_spawn_file_actions_init_nid_postfix(FileActions** result) {
    if (result == nullptr) throw std::invalid_argument("posix_spawn_file_actions_init: null object");
    auto* actions = static_cast<FileActions*>(malloc_nid_postfix(sizeof(FileActions)));
    if (actions == nullptr) return -1;
    actions->first = nullptr;
    actions->last = &actions->first;
    *result = actions;
    return 0;
}

int APS5_VABI posix_spawn_file_actions_destroy_nid_postfix(FileActions** actions) {
    FileActions* list = Actions(actions);
    while (FileActionEntry* entry = list->first) {
        list->first = entry->next;
        if (entry->action == FileAction::Open) free_nid_postfix(entry->path);
        free_nid_postfix(entry);
    }
    free_nid_postfix(list);
    return 0;
}

int APS5_VABI posix_spawn_file_actions_addopen_nid_postfix(FileActions** actions, int descriptor, const char* path, int flags, std::uint16_t mode) {
    FileActions* list = Actions(actions);
    if (descriptor < 0) return GuestBadDescriptor;
    if (path == nullptr) throw std::invalid_argument("posix_spawn_file_actions_addopen: null path");
    FileActionEntry* entry = NewEntry(FileAction::Open, descriptor);
    if (entry == nullptr) return errno;
    const std::size_t length = std::strlen(path) + 1;
    entry->path = static_cast<char*>(malloc_nid_postfix(length));
    if (entry->path == nullptr) {
        const int error = errno;
        free_nid_postfix(entry);
        return error;
    }
    std::memcpy(entry->path, path, length);
    entry->openFlags = flags;
    entry->mode = mode;
    return Append(list, entry);
}

int APS5_VABI posix_spawn_file_actions_adddup2_nid_postfix(FileActions** actions, int descriptor, int newDescriptor) {
    FileActions* list = Actions(actions);
    if (descriptor < 0 || newDescriptor < 0) return GuestBadDescriptor;
    FileActionEntry* entry = NewEntry(FileAction::Duplicate, descriptor);
    if (entry == nullptr) return errno;
    entry->newDescriptor = newDescriptor;
    return Append(list, entry);
}

int APS5_VABI posix_spawn_file_actions_addclose_nid_postfix(FileActions** actions, int descriptor) {
    FileActions* list = Actions(actions);
    if (descriptor < 0) return GuestBadDescriptor;
    FileActionEntry* entry = NewEntry(FileAction::Close, descriptor);
    if (entry == nullptr) return errno;
    return Append(list, entry);
}

int APS5_VABI posix_spawnattr_init_nid_postfix(SpawnAttributes** result) {
    if (result == nullptr) throw std::invalid_argument("posix_spawnattr_init: null object");
    auto* attributes = static_cast<SpawnAttributes*>(calloc_nid_postfix(1, sizeof(SpawnAttributes)));
    if (attributes == nullptr) return errno;
    *result = attributes;
    return 0;
}

int APS5_VABI posix_spawnattr_destroy_nid_postfix(SpawnAttributes** attributes) {
    free_nid_postfix(Attributes(attributes));
    return 0;
}

int APS5_VABI posix_spawnattr_getflags_nid_postfix(SpawnAttributes* const* attributes, short* flags) {
    *flags = Attributes(attributes)->flags;
    return 0;
}

int APS5_VABI posix_spawnattr_getpgroup_nid_postfix(SpawnAttributes* const* attributes, int* group) {
    *group = Attributes(attributes)->processGroup;
    return 0;
}

int APS5_VABI posix_spawnattr_getschedparam_nid_postfix(SpawnAttributes* const* attributes, GuestSchedulingParameter* parameter) {
    *parameter = Attributes(attributes)->schedulingParameter;
    return 0;
}

int APS5_VABI posix_spawnattr_getschedpolicy_nid_postfix(SpawnAttributes* const* attributes, int* policy) {
    *policy = Attributes(attributes)->schedulingPolicy;
    return 0;
}

int APS5_VABI posix_spawnattr_getsigdefault_nid_postfix(SpawnAttributes* const* attributes, GuestSignalSet* signals) {
    *signals = Attributes(attributes)->defaultSignals;
    return 0;
}

int APS5_VABI posix_spawnattr_getsigmask_nid_postfix(SpawnAttributes* const* attributes, GuestSignalSet* mask) {
    *mask = Attributes(attributes)->signalMask;
    return 0;
}

int APS5_VABI posix_spawnattr_setflags_nid_postfix(SpawnAttributes** attributes, short flags) {
    Attributes(attributes)->flags = flags;
    return 0;
}

int APS5_VABI posix_spawnattr_setpgroup_nid_postfix(SpawnAttributes** attributes, int group) {
    Attributes(attributes)->processGroup = group;
    return 0;
}

int APS5_VABI posix_spawnattr_setschedparam_nid_postfix(SpawnAttributes** attributes, const GuestSchedulingParameter* parameter) {
    Attributes(attributes)->schedulingParameter = *parameter;
    return 0;
}

int APS5_VABI posix_spawnattr_setschedpolicy_nid_postfix(SpawnAttributes** attributes, int policy) {
    Attributes(attributes)->schedulingPolicy = policy;
    return 0;
}

int APS5_VABI posix_spawnattr_setsigdefault_nid_postfix(SpawnAttributes** attributes, const GuestSignalSet* signals) {
    Attributes(attributes)->defaultSignals = *signals;
    return 0;
}

int APS5_VABI posix_spawnattr_setsigmask_nid_postfix(SpawnAttributes** attributes, const GuestSignalSet* mask) {
    Attributes(attributes)->signalMask = *mask;
    return 0;
}

int APS5_VABI posix_spawn_nid_postfix(int*, const char*, FileActions* const*, SpawnAttributes* const*, char* const*, char* const*) {
    throw std::runtime_error("posix_spawn: child processes are not supported");
}

int APS5_VABI posix_spawnp_nid_postfix(int*, const char*, FileActions* const*, SpawnAttributes* const*, char* const*, char* const*) {
    throw std::runtime_error("posix_spawnp: child processes are not supported");
}

}
