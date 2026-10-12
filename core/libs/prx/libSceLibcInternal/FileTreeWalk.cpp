#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestDirectory.hpp"

namespace {

struct GuestFtsEntry;

using EntryCompare = int (APS5_VABI *)(const GuestFtsEntry* const*, const GuestFtsEntry* const*);

struct GuestFts {
    GuestFtsEntry* current;
    GuestFtsEntry* children;
    GuestFtsEntry** array;
    std::uint32_t device;
    char* path;
    int rootDescriptor;
    std::size_t pathLength;
    std::size_t arrayItems;
    EntryCompare compare;
    int options;
    void* clientPointer;
};
static_assert(offsetof(GuestFts, compare) == 64 && sizeof(GuestFts) == 88);

struct GuestFtsEntry {
    GuestFtsEntry* cycle;
    GuestFtsEntry* parent;
    GuestFtsEntry* link;
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
    GuestFts* fts;
};
static_assert(offsetof(GuestFtsEntry, level) == 96 && offsetof(GuestFtsEntry, status) == 120 && sizeof(GuestFtsEntry) == 144);

struct GuestFtw {
    int base;
    int level;
};

}

extern "C" {
void* APS5_VABI opendir_nid_postfix(const char* path);
GuestDirectoryEntry* APS5_VABI readdir_nid_postfix(void* directory);
int APS5_VABI closedir_nid_postfix(void* directory);
int APS5_VABI stat_nid_postfix(const char* path, FileStat* status);
int APS5_VABI lstat_nid_postfix(const char* path, FileStat* status);
void* APS5_VABI malloc_nid_postfix(std::size_t size);
void* APS5_VABI calloc_nid_postfix(std::size_t count, std::size_t size);
void* APS5_VABI reallocf_nid_postfix(void* pointer, std::size_t size);
void APS5_VABI free_nid_postfix(void* pointer);
void APS5_VABI qsort_nid_postfix(void* base, std::size_t count, std::size_t size, int (APS5_VABI *compare)(const void*, const void*));
}

namespace {

constexpr int ComFollow = 0x001;
constexpr int Logical = 0x002;
constexpr int NoChdir = 0x004;
constexpr int NoStat = 0x008;
constexpr int Physical = 0x010;
constexpr int SeeDot = 0x020;
constexpr int CrossDevice = 0x040;
constexpr int OptionMask = 0x0ff;
constexpr int NameOnly = 0x100;
constexpr int Stop = 0x200;

constexpr long RootParentLevel = -1;
constexpr long RootLevel = 0;

constexpr int InfoDirectory = 1;
constexpr int InfoCycle = 2;
constexpr int InfoDefault = 3;
constexpr int InfoUnreadable = 4;
constexpr int InfoDot = 5;
constexpr int InfoPostorder = 6;
constexpr int InfoError = 7;
constexpr int InfoFile = 8;
constexpr int InfoInit = 9;
constexpr int InfoNoStat = 10;
constexpr int InfoNoStatOk = 11;
constexpr int InfoLink = 12;
constexpr int InfoLinkNone = 13;


constexpr int Again = 1;
constexpr int Follow = 2;
constexpr int NoInstruction = 3;
constexpr int Skip = 4;

constexpr int BuildChild = 1;
constexpr int BuildNames = 2;
constexpr int BuildRead = 3;

constexpr int FtwFile = 0;
constexpr int FtwDirectory = 1;
constexpr int FtwUnreadable = 2;
constexpr int FtwPostorder = 3;
constexpr int FtwNoStat = 4;
constexpr int FtwLink = 5;
constexpr int FtwLinkNone = 6;
constexpr int FtwPhysical = 0x01;
constexpr int FtwMount = 0x02;
constexpr int FtwDepth = 0x04;

constexpr std::uint8_t TypeUnknown = 0;
constexpr std::uint8_t TypeDirectoryEntry = 4;

constexpr std::uint16_t TypeMask = 0170000;
constexpr std::uint16_t TypeDirectory = 0040000;
constexpr std::uint16_t TypeRegular = 0100000;
constexpr std::uint16_t TypeLink = 0120000;

constexpr int GuestInvalid = 22;
constexpr int GuestLoop = 62;
constexpr std::size_t MaxPath = 1024;

bool IsDot(const char* name) { return name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2])); }

bool IsSet(const GuestFts* fts, int option) { return (fts->options & option) != 0; }

std::size_t AppendOffset(const GuestFtsEntry* entry) {
    return entry->path[entry->pathLength - 1] == '/' ? entry->pathLength - 1 : entry->pathLength;
}

GuestFtsEntry* Allocate(GuestFts* fts, const char* name, std::size_t nameLength) {
    struct EntryWithStatus {
        GuestFtsEntry entry;
        FileStat status;
    };
    const std::size_t length = (IsSet(fts, NoStat) ? sizeof(GuestFtsEntry) : sizeof(EntryWithStatus)) + nameLength + 1;
    auto* entry = static_cast<GuestFtsEntry*>(malloc_nid_postfix(length));
    if (entry == nullptr) return nullptr;
    if (IsSet(fts, NoStat)) {
        entry->name = reinterpret_cast<char*>(entry + 1);
        entry->status = nullptr;
    } else {
        auto* withStatus = reinterpret_cast<EntryWithStatus*>(entry);
        entry->name = reinterpret_cast<char*>(withStatus + 1);
        entry->status = &withStatus->status;
    }
    std::memcpy(entry->name, name, nameLength);
    entry->name[nameLength] = '\0';
    entry->nameLength = nameLength;
    entry->path = fts->path;
    entry->error = 0;
    entry->flags = 0;
    entry->instruction = NoInstruction;
    entry->number = 0;
    entry->pointer = nullptr;
    entry->fts = fts;
    return entry;
}

void FreeList(GuestFtsEntry* head) {
    while (GuestFtsEntry* entry = head) {
        head = head->link;
        free_nid_postfix(entry);
    }
}

bool GrowPath(GuestFts* fts, std::size_t more) {
    fts->pathLength += more + 256;
    fts->path = static_cast<char*>(reallocf_nid_postfix(fts->path, fts->pathLength));
    return fts->path == nullptr;
}

void AdjustPaths(GuestFts* fts, GuestFtsEntry* head) {
    char* address = fts->path;
    auto adjust = [address](GuestFtsEntry* entry) {
        if (entry->accessPath != entry->name) entry->accessPath = address + (entry->accessPath - entry->path);
        entry->path = address;
    };
    for (GuestFtsEntry* entry = fts->children; entry != nullptr; entry = entry->link) adjust(entry);
    for (GuestFtsEntry* entry = head; entry->level >= RootLevel;) {
        adjust(entry);
        entry = entry->link != nullptr ? entry->link : entry->parent;
    }
}

int APS5_VABI CompareTrampoline(const void* first, const void* second) {
    const GuestFts* fts = (*static_cast<const GuestFtsEntry* const*>(first))->fts;
    return fts->compare(static_cast<const GuestFtsEntry* const*>(first), static_cast<const GuestFtsEntry* const*>(second));
}

GuestFtsEntry* Sort(GuestFts* fts, GuestFtsEntry* head, std::size_t count) {
    if (count > fts->arrayItems) {
        fts->arrayItems = count + 40;
        fts->array = static_cast<GuestFtsEntry**>(reallocf_nid_postfix(fts->array, fts->arrayItems * sizeof(GuestFtsEntry*)));
        if (fts->array == nullptr) {
            fts->arrayItems = 0;
            return head;
        }
    }
    GuestFtsEntry** item = fts->array;
    for (GuestFtsEntry* entry = head; entry != nullptr; entry = entry->link) *item++ = entry;
    qsort_nid_postfix(fts->array, count, sizeof(GuestFtsEntry*), CompareTrampoline);
    item = fts->array;
    head = *item;
    for (; --count; ++item) item[0]->link = item[1];
    item[0]->link = nullptr;
    return head;
}

int StatEntry(GuestFts* fts, GuestFtsEntry* entry, bool follow) {
    FileStat local{};
    FileStat* status = IsSet(fts, NoStat) ? &local : entry->status;
    const char* path = entry->accessPath;
    bool failed = false;
    if (IsSet(fts, Logical) || follow) {
        if (stat_nid_postfix(path, status)) {
            const int savedError = errno;
            if (lstat_nid_postfix(path, status)) {
                entry->error = savedError;
                failed = true;
            } else {
                errno = 0;
                if ((status->st_mode & TypeMask) == TypeLink) return InfoLinkNone;
            }
        }
    } else if (lstat_nid_postfix(path, status)) {
        entry->error = errno;
        failed = true;
    }
    if (failed) {
        std::memset(status, 0, sizeof(FileStat));
        return InfoNoStat;
    }

    const std::uint16_t type = status->st_mode & TypeMask;
    if (type == TypeDirectory) {
        entry->device = status->st_dev;
        entry->inode = status->st_ino;
        entry->linkCount = status->st_nlink;
        if (IsDot(entry->name)) return InfoDot;
        for (GuestFtsEntry* ancestor = entry->parent; ancestor->level >= RootLevel; ancestor = ancestor->parent) {
            if (entry->inode == ancestor->inode && entry->device == ancestor->device) {
                entry->cycle = ancestor;
                return InfoCycle;
            }
        }
        return InfoDirectory;
    }
    if (type == TypeLink) return InfoLink;
    if (type == TypeRegular) return InfoFile;
    return InfoDefault;
}

void Load(GuestFts* fts, GuestFtsEntry* entry) {
    std::size_t length = entry->pathLength = entry->nameLength;
    std::memmove(fts->path, entry->name, length + 1);
    char* slash = std::strrchr(entry->name, '/');
    if (slash != nullptr && (slash != entry->name || slash[1])) {
        length = std::strlen(++slash);
        std::memmove(entry->name, slash, length + 1);
        entry->nameLength = length;
    }
    entry->accessPath = entry->path = fts->path;
    fts->device = entry->device;
}

GuestFtsEntry* Build(GuestFts* fts, int type) {
    GuestFtsEntry* current = fts->current;
    void* directory = opendir_nid_postfix(current->accessPath);
    if (directory == nullptr) {
        if (type == BuildRead) {
            current->info = InfoUnreadable;
            current->error = errno;
        }
        return nullptr;
    }

    long links;
    bool noStat;
    if (type == BuildNames) {
        links = 0;
        noStat = false;
    } else if (IsSet(fts, NoStat) && IsSet(fts, Physical)) {
        links = -1;
        noStat = true;
    } else {
        links = -1;
        noStat = false;
    }

    std::size_t length = AppendOffset(current);
    char* cursor = fts->path + length;
    *cursor++ = '/';
    length++;
    std::size_t maxLength = fts->pathLength - length;
    const long level = current->level + 1;

    bool adjust = false;
    GuestFtsEntry* head = nullptr;
    GuestFtsEntry* tail = nullptr;
    std::size_t count = 0;
    while (GuestDirectoryEntry* item = readdir_nid_postfix(directory)) {
        const std::size_t nameLength = item->nameLength;
        if (!IsSet(fts, SeeDot) && IsDot(item->name)) continue;

        GuestFtsEntry* entry = Allocate(fts, item->name, nameLength);
        bool outOfMemory = entry == nullptr;
        if (!outOfMemory && nameLength >= maxLength) {
            char* oldAddress = fts->path;
            if (GrowPath(fts, nameLength + length + 1)) {
                outOfMemory = true;
            } else {
                if (oldAddress != fts->path) {
                    adjust = true;
                    cursor = fts->path + length;
                }
                maxLength = fts->pathLength - length;
            }
        }
        if (outOfMemory) {
            const int savedError = errno;
            free_nid_postfix(entry);
            FreeList(head);
            closedir_nid_postfix(directory);
            current->info = InfoError;
            fts->options |= Stop;
            errno = savedError;
            return nullptr;
        }

        entry->level = level;
        entry->parent = fts->current;
        entry->path = fts->path;
        entry->pathLength = length + nameLength;
        if (links == 0 || (noStat && item->type != TypeDirectoryEntry && item->type != TypeUnknown)) {
            entry->accessPath = entry->path;
            entry->info = InfoNoStatOk;
        } else {
            entry->accessPath = entry->path;
            std::memmove(cursor, entry->name, entry->nameLength + 1);
            entry->info = StatEntry(fts, entry, false);
        }

        entry->link = nullptr;
        if (head == nullptr) head = tail = entry;
        else {
            tail->link = entry;
            tail = entry;
        }
        ++count;
    }
    closedir_nid_postfix(directory);

    if (adjust) AdjustPaths(fts, head);
    fts->path[current->pathLength] = '\0';

    if (count == 0) {
        if (type == BuildRead) current->info = InfoPostorder;
        return nullptr;
    }
    if (fts->compare != nullptr && count > 1) head = Sort(fts, head, count);
    return head;
}

}

extern "C" {

GuestFts* APS5_VABI fts_open_nid_postfix(char* const* arguments, int options, EntryCompare compare) {
    if (arguments == nullptr) throw std::invalid_argument("fts_open: null argument vector");
    if ((options & ~OptionMask) || *arguments == nullptr) {
        errno = GuestInvalid;
        return nullptr;
    }
    auto* fts = static_cast<GuestFts*>(calloc_nid_postfix(1, sizeof(GuestFts)));
    if (fts == nullptr) return nullptr;
    fts->compare = compare;
    fts->options = options | NoChdir;
    fts->rootDescriptor = -1;

    std::size_t longest = 0;
    for (char* const* argument = arguments; *argument != nullptr; ++argument) longest = std::max(longest, std::strlen(*argument));
    if (GrowPath(fts, std::max(longest + 1, MaxPath))) {
        free_nid_postfix(fts);
        return nullptr;
    }

    GuestFtsEntry* parent = Allocate(fts, "", 0);
    if (parent == nullptr) {
        free_nid_postfix(fts->path);
        free_nid_postfix(fts);
        return nullptr;
    }
    parent->level = RootParentLevel;

    GuestFtsEntry* root = nullptr;
    GuestFtsEntry* last = nullptr;
    std::size_t count = 0;
    for (; *arguments != nullptr; ++arguments, ++count) {
        GuestFtsEntry* entry = Allocate(fts, *arguments, std::strlen(*arguments));
        if (entry == nullptr) {
            FreeList(root);
            free_nid_postfix(parent);
            free_nid_postfix(fts->path);
            free_nid_postfix(fts);
            return nullptr;
        }
        entry->level = RootLevel;
        entry->parent = parent;
        entry->accessPath = entry->name;
        entry->info = StatEntry(fts, entry, IsSet(fts, ComFollow));
        if (entry->info == InfoDot) entry->info = InfoDirectory;
        if (compare != nullptr) {
            entry->link = root;
            root = entry;
        } else {
            entry->link = nullptr;
            if (root == nullptr) last = root = entry;
            else {
                last->link = entry;
                last = entry;
            }
        }
    }
    if (compare != nullptr && count > 1) root = Sort(fts, root, count);

    fts->current = Allocate(fts, "", 0);
    if (fts->current == nullptr) {
        FreeList(root);
        free_nid_postfix(parent);
        free_nid_postfix(fts->path);
        free_nid_postfix(fts);
        return nullptr;
    }
    fts->current->link = root;
    fts->current->info = InfoInit;
    return fts;
}

GuestFtsEntry* APS5_VABI fts_read_nid_postfix(GuestFts* fts) {
    if (fts == nullptr) throw std::invalid_argument("fts_read: null stream");
    if (fts->current == nullptr || IsSet(fts, Stop)) return nullptr;

    GuestFtsEntry* entry = fts->current;
    const int instruction = entry->instruction;
    entry->instruction = NoInstruction;

    if (instruction == Again) {
        entry->info = StatEntry(fts, entry, false);
        return entry;
    }
    if (instruction == Follow && (entry->info == InfoLink || entry->info == InfoLinkNone)) {
        entry->info = StatEntry(fts, entry, true);
        return entry;
    }

    if (entry->info == InfoDirectory) {
        if (instruction == Skip || (IsSet(fts, CrossDevice) && entry->device != fts->device)) {
            if (fts->children != nullptr) {
                FreeList(fts->children);
                fts->children = nullptr;
            }
            entry->info = InfoPostorder;
            return entry;
        }
        if (fts->children != nullptr && IsSet(fts, NameOnly)) {
            fts->options &= ~NameOnly;
            FreeList(fts->children);
            fts->children = nullptr;
        }
        if (fts->children == nullptr && (fts->children = Build(fts, BuildRead)) == nullptr) {
            if (IsSet(fts, Stop)) return nullptr;
            return entry;
        }
        entry = fts->children;
        fts->children = nullptr;
    } else {
        GuestFtsEntry* previous = entry;
        for (;;) {
            GuestFtsEntry* next = previous->link;
            if (next == nullptr) {
                GuestFtsEntry* parent = previous->parent;
                free_nid_postfix(previous);
                if (parent->level == RootParentLevel) {
                    free_nid_postfix(parent);
                    errno = 0;
                    return fts->current = nullptr;
                }
                fts->path[parent->pathLength] = '\0';
                parent->info = parent->error ? InfoError : InfoPostorder;
                return fts->current = parent;
            }
            if (next->level == RootLevel) {
                free_nid_postfix(previous);
                Load(fts, next);
                return fts->current = next;
            }
            free_nid_postfix(previous);
            if (next->instruction == Skip) {
                previous = next;
                continue;
            }
            if (next->instruction == Follow) {
                next->info = StatEntry(fts, next, true);
                next->instruction = NoInstruction;
            }
            entry = next;
            break;
        }
    }

    char* tail = fts->path + AppendOffset(entry->parent);
    *tail++ = '/';
    std::memmove(tail, entry->name, entry->nameLength + 1);
    return fts->current = entry;
}

int APS5_VABI fts_set_nid_postfix(GuestFts*, GuestFtsEntry* entry, int instruction) {
    if (entry == nullptr) throw std::invalid_argument("fts_set: null entry");
    if (instruction != 0 && instruction != Again && instruction != Follow && instruction != NoInstruction && instruction != Skip) {
        errno = GuestInvalid;
        return 1;
    }
    entry->instruction = instruction;
    return 0;
}

GuestFtsEntry* APS5_VABI fts_children_nid_postfix(GuestFts* fts, int instruction) {
    if (fts == nullptr) throw std::invalid_argument("fts_children: null stream");
    if (instruction != 0 && instruction != NameOnly) {
        errno = GuestInvalid;
        return nullptr;
    }
    GuestFtsEntry* entry = fts->current;
    if (entry == nullptr) throw std::invalid_argument("fts_children: finished stream");
    errno = 0;
    if (IsSet(fts, Stop)) return nullptr;
    if (entry->info == InfoInit) return entry->link;
    if (entry->info != InfoDirectory) return nullptr;
    if (fts->children != nullptr) FreeList(fts->children);
    if (instruction == NameOnly) {
        fts->options |= NameOnly;
        instruction = BuildNames;
    } else {
        instruction = BuildChild;
    }
    return fts->children = Build(fts, instruction);
}

int APS5_VABI fts_close_nid_postfix(GuestFts* fts) {
    if (fts == nullptr) throw std::invalid_argument("fts_close: null stream");
    if (fts->current != nullptr) {
        GuestFtsEntry* entry = fts->current;
        while (entry->level >= RootLevel) {
            GuestFtsEntry* next = entry->link != nullptr ? entry->link : entry->parent;
            free_nid_postfix(entry);
            entry = next;
        }
        free_nid_postfix(entry);
    }
    if (fts->children != nullptr) FreeList(fts->children);
    free_nid_postfix(fts->array);
    free_nid_postfix(fts->path);
    free_nid_postfix(fts);
    return 0;
}

void* APS5_VABI fts_get_clientptr_nid_postfix(GuestFts* fts) {
    if (fts == nullptr) throw std::invalid_argument("fts_get_clientptr: null stream");
    return fts->clientPointer;
}
GuestFts* APS5_VABI fts_get_stream_nid_postfix(GuestFtsEntry* entry) {
    if (entry == nullptr) throw std::invalid_argument("fts_get_stream: null entry");
    return entry->fts;
}
void APS5_VABI fts_set_clientptr_nid_postfix(GuestFts* fts, void* pointer) {
    if (fts == nullptr) throw std::invalid_argument("fts_set_clientptr: null stream");
    fts->clientPointer = pointer;
}

using NftwCallback = int (APS5_VABI *)(const char*, const FileStat*, int, GuestFtw*);
using FtwCallback = int (APS5_VABI *)(const char*, const FileStat*, int);

int APS5_VABI nftw_nid_postfix(const char* path, NftwCallback callback, int descriptors, int flags) {
    if (path == nullptr || callback == nullptr) throw std::invalid_argument("nftw: null path or callback");
    if (descriptors < 1) {
        errno = GuestInvalid;
        return -1;
    }
    char* const paths[2] = {const_cast<char*>(path), nullptr};
    int options = ComFollow | NoChdir;
    if (flags & FtwMount) options |= CrossDevice;
    options |= (flags & FtwPhysical) ? Physical : Logical;
    const bool postorder = (flags & FtwDepth) != 0;
    GuestFts* fts = fts_open_nid_postfix(paths, options, nullptr);
    if (fts == nullptr) return -1;
    int result = 0;
    while (GuestFtsEntry* entry = fts_read_nid_postfix(fts)) {
        int type;
        switch (entry->info) {
        case InfoDirectory:
            if (postorder) continue;
            type = FtwDirectory;
            break;
        case InfoCycle:
            continue;
        case InfoUnreadable:
            type = FtwUnreadable;
            break;
        case InfoPostorder:
            if (!postorder) continue;
            type = FtwPostorder;
            break;
        case InfoFile:
        case InfoDefault:
            type = FtwFile;
            break;
        case InfoNoStat:
        case InfoNoStatOk:
            type = FtwNoStat;
            break;
        case InfoLink:
            type = FtwLink;
            break;
        case InfoLinkNone:
            type = FtwLinkNone;
            break;
        default:
            result = -1;
            break;
        }
        if (result != 0) break;
        GuestFtw position{static_cast<int>(entry->pathLength - entry->nameLength), static_cast<int>(entry->level)};
        result = callback(entry->path, entry->status, type, &position);
        if (result != 0) break;
    }
    const int savedError = errno;
    fts_close_nid_postfix(fts);
    errno = savedError;
    return result;
}

int APS5_VABI ftw_nid_postfix(const char* path, FtwCallback callback, int descriptors) {
    if (path == nullptr || callback == nullptr) throw std::invalid_argument("ftw: null path or callback");
    if (descriptors < 1) {
        errno = GuestInvalid;
        return -1;
    }
    char* const paths[2] = {const_cast<char*>(path), nullptr};
    GuestFts* fts = fts_open_nid_postfix(paths, Logical | ComFollow | NoChdir, nullptr);
    if (fts == nullptr) return -1;
    int result = 0;
    while (GuestFtsEntry* entry = fts_read_nid_postfix(fts)) {
        int type;
        switch (entry->info) {
        case InfoDirectory:
            type = FtwDirectory;
            break;
        case InfoUnreadable:
            type = FtwUnreadable;
            break;
        case InfoPostorder:
            continue;
        case InfoFile:
        case InfoDefault:
            type = FtwFile;
            break;
        case InfoNoStat:
        case InfoNoStatOk:
        case InfoLinkNone:
            type = FtwNoStat;
            break;
        case InfoLink:
            type = FtwLink;
            break;
        case InfoCycle:
            errno = GuestLoop;
            [[fallthrough]];
        default:
            result = -1;
            break;
        }
        if (result != 0) break;
        result = callback(entry->path, entry->status, type);
        if (result != 0) break;
    }
    const int savedError = errno;
    fts_close_nid_postfix(fts);
    errno = savedError;
    return result;
}

}
