#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <mutex>
#include <stdexcept>
#ifdef _WIN32
#include <malloc.h>
#endif

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" int APS5_VABI getpid_nid_postfix(void);

namespace {

// FreeBSD 11 struct ipc_perm and struct shmid_ds.
struct GuestIpcPermissions {
    std::uint32_t creatorUser;
    std::uint32_t creatorGroup;
    std::uint32_t user;
    std::uint32_t group;
    std::uint16_t mode;
    std::uint16_t sequence;
    std::int64_t key;
};

struct GuestSegmentStatus {
    GuestIpcPermissions permissions;
    std::size_t size;
    int lastProcess;
    int creatorProcess;
    int attachments;
    std::int64_t attachTime;
    std::int64_t detachTime;
    std::int64_t changeTime;
};
static_assert(sizeof(GuestIpcPermissions) == 32 && sizeof(GuestSegmentStatus) == 80);

constexpr int GuestNoEntry = 2;
constexpr int GuestExists = 17;
constexpr int GuestTooManyFiles = 24;
constexpr int GuestNoSpace = 28;
constexpr int GuestInvalid = 22;

constexpr std::int64_t PrivateKey = 0;
constexpr int Create = 01000;
constexpr int Exclusive = 02000;
constexpr int RemoveCommand = 0;
constexpr int SetCommand = 1;
constexpr int StatusCommand = 2;
constexpr std::uint16_t AccessPermissions = 0777;

constexpr std::size_t PageSize = 16384;
constexpr std::size_t MaximumSegmentSize = std::size_t{131072} * 4096;
constexpr std::size_t MaximumSegments = 192;
constexpr int MaximumAttachments = 128;

struct Segment {
    bool allocated = false;
    bool removed = false;
    std::uint16_t sequence = 0;
    GuestSegmentStatus status{};
    void* memory = nullptr;
};

struct Registry {
    std::mutex lock;
    std::array<Segment, MaximumSegments> segments;
    std::map<void*, std::pair<std::size_t, int>> attachments;
    int attachCount = 0;
};

Registry& Segments() {
    static auto* registry = new Registry();
    return *registry;
}

int Fail(int error) {
    errno = error;
    return -1;
}

std::int64_t Now() { return static_cast<std::int64_t>(std::time(nullptr)); }

int Identifier(std::size_t index, const Segment& segment) {
    return static_cast<int>((static_cast<unsigned>(segment.sequence) << 16) | (index & 0xffff));
}

void* Allocate(std::size_t size) {
    const std::size_t rounded = (size + PageSize - 1) & ~(PageSize - 1);
#ifdef _WIN32
    void* memory = _aligned_malloc(rounded, PageSize);
#else
    void* memory = std::aligned_alloc(PageSize, rounded);
#endif
    if (memory != nullptr) std::memset(memory, 0, rounded);
    return memory;
}

void Release(Segment& segment) {
#ifdef _WIN32
    _aligned_free(segment.memory);
#else
    std::free(segment.memory);
#endif
    segment.memory = nullptr;
    segment.allocated = false;
    segment.removed = false;
}

Segment* Lookup(Registry& registry, int identifier, std::size_t* index) {
    const std::size_t slot = static_cast<unsigned>(identifier) & 0xffff;
    if (identifier < 0 || slot >= MaximumSegments) return nullptr;
    Segment& segment = registry.segments[slot];
    if (!segment.allocated || segment.removed || segment.sequence != ((static_cast<unsigned>(identifier) >> 16) & 0x7fff)) return nullptr;
    *index = slot;
    return &segment;
}

}

extern "C" {

int APS5_VABI shmget_nid_postfix(std::int64_t key, std::size_t size, int flags) {
    auto& registry = Segments();
    std::lock_guard lock(registry.lock);
    if (key != PrivateKey) {
        for (std::size_t index = 0; index < MaximumSegments; index++) {
            Segment& segment = registry.segments[index];
            if (!segment.allocated || segment.removed || segment.status.permissions.key != key) continue;
            if ((flags & (Create | Exclusive)) == (Create | Exclusive)) return Fail(GuestExists);
            if (size != 0 && size > segment.status.size) return Fail(GuestInvalid);
            return Identifier(index, segment);
        }
        if (!(flags & Create)) return Fail(GuestNoEntry);
    }
    if (size < 1 || size > MaximumSegmentSize) return Fail(GuestInvalid);
    for (std::size_t index = 0; index < MaximumSegments; index++) {
        Segment& segment = registry.segments[index];
        if (segment.allocated) continue;
        segment.memory = Allocate(size);
        if (segment.memory == nullptr) return Fail(GuestNoSpace);
        segment.allocated = true;
        segment.removed = false;
        segment.sequence = static_cast<std::uint16_t>((segment.sequence + 1) & 0x7fff);
        segment.status = {};
        segment.status.permissions.key = key;
        segment.status.permissions.mode = static_cast<std::uint16_t>(flags & AccessPermissions);
        segment.status.permissions.sequence = segment.sequence;
        segment.status.size = size;
        segment.status.creatorProcess = getpid_nid_postfix();
        segment.status.changeTime = Now();
        return Identifier(index, segment);
    }
    return Fail(GuestNoSpace);
}

void* APS5_VABI shmat_nid_postfix(int identifier, const void* address, int flags) {
    (void)flags;
    if (address != nullptr) throw std::runtime_error("shmat: attaching at a requested address is not implemented");
    auto& registry = Segments();
    std::lock_guard lock(registry.lock);
    std::size_t index = 0;
    Segment* segment = Lookup(registry, identifier, &index);
    if (segment == nullptr) {
        errno = GuestInvalid;
        return reinterpret_cast<void*>(-1);
    }
    if (registry.attachCount >= MaximumAttachments) {
        errno = GuestTooManyFiles;
        return reinterpret_cast<void*>(-1);
    }
    auto& attachment = registry.attachments[segment->memory];
    attachment.first = index;
    ++attachment.second;
    ++registry.attachCount;
    ++segment->status.attachments;
    segment->status.lastProcess = getpid_nid_postfix();
    segment->status.attachTime = Now();
    return segment->memory;
}

int APS5_VABI shmdt_nid_postfix(const void* address) {
    auto& registry = Segments();
    std::lock_guard lock(registry.lock);
    const auto found = registry.attachments.find(const_cast<void*>(address));
    if (found == registry.attachments.end()) return Fail(GuestInvalid);
    Segment& segment = registry.segments[found->second.first];
    if (--found->second.second == 0) registry.attachments.erase(found);
    --registry.attachCount;
    --segment.status.attachments;
    segment.status.lastProcess = getpid_nid_postfix();
    segment.status.detachTime = Now();
    if (segment.removed && segment.status.attachments == 0) Release(segment);
    return 0;
}

int APS5_VABI shmctl_nid_postfix(int identifier, int command, GuestSegmentStatus* status) {
    auto& registry = Segments();
    std::lock_guard lock(registry.lock);
    std::size_t index = 0;
    Segment* segment = Lookup(registry, identifier, &index);
    if (segment == nullptr) return Fail(GuestInvalid);
    switch (command) {
    case StatusCommand:
        if (status == nullptr) throw std::invalid_argument("shmctl: null status");
        *status = segment->status;
        return 0;
    case SetCommand:
        if (status == nullptr) throw std::invalid_argument("shmctl: null status");
        segment->status.permissions.user = status->permissions.user;
        segment->status.permissions.group = status->permissions.group;
        segment->status.permissions.mode = static_cast<std::uint16_t>((segment->status.permissions.mode & ~AccessPermissions) |
                                                                      (status->permissions.mode & AccessPermissions));
        segment->status.changeTime = Now();
        return 0;
    case RemoveCommand:
        segment->status.permissions.key = PrivateKey;
        segment->removed = true;
        if (segment->status.attachments == 0) Release(*segment);
        return 0;
    default:
        return Fail(GuestInvalid);
    }
}

}
