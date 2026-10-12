#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {

struct Permissions {
    std::uint32_t creatorUser;
    std::uint32_t creatorGroup;
    std::uint32_t user;
    std::uint32_t group;
    std::uint16_t mode;
    std::uint16_t sequence;
    std::int64_t key;
};

struct Status {
    Permissions permissions;
    std::size_t size;
    int lastProcess;
    int creatorProcess;
    int attachments;
    std::int64_t attachTime;
    std::int64_t detachTime;
    std::int64_t changeTime;
};

}

extern "C" {
int APS5_VABI shmget_nid_postfix(std::int64_t, std::size_t, int);
void* APS5_VABI shmat_nid_postfix(int, const void*, int);
int APS5_VABI shmdt_nid_postfix(const void*);
int APS5_VABI shmctl_nid_postfix(int, int, Status*);
int APS5_VABI getpid_nid_postfix(void);
int* APS5_VABI __error_nid_postfix();
}

namespace {

constexpr int Create = 01000;
constexpr int Exclusive = 02000;
constexpr int Remove = 0;
constexpr int Set = 1;
constexpr int Stat = 2;
void* const Failed = reinterpret_cast<void*>(-1);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "failed: %s (errno %d)\n", message, *__error_nid_postfix());
        std::abort();
    }
}

bool Fails(long long result, int error) { return result == -1 && *__error_nid_postfix() == error; }

void CheckKeys() {
    Require(Fails(shmget_nid_postfix(0x5150, 64, 0), 2), "missing key");
    const int first = shmget_nid_postfix(0x5150, 4096, Create | 0640);
    Require(first >= 0, "create keyed segment");
    Require(shmget_nid_postfix(0x5150, 0, 0) == first && shmget_nid_postfix(0x5150, 4096, Create) == first, "lookup by key");
    Require(Fails(shmget_nid_postfix(0x5150, 4097, 0), 22), "larger than the segment");
    Require(Fails(shmget_nid_postfix(0x5150, 64, Create | Exclusive), 17), "exclusive");
    Require(Fails(shmget_nid_postfix(0, 0, Create), 22) && Fails(shmget_nid_postfix(0x5151, std::size_t{1} << 40, Create), 22), "size limits");
    const int other = shmget_nid_postfix(0, 4096, Create);
    Require(other >= 0 && other != first && shmget_nid_postfix(0, 4096, Create) != other, "private keys are always new");

    Status status{};
    Require(shmctl_nid_postfix(first, Stat, &status) == 0, "stat");
    Require(status.size == 4096 && status.permissions.key == 0x5150 && status.permissions.mode == 0640, "stat values");
    Require(status.creatorProcess == getpid_nid_postfix() && status.attachments == 0 && status.changeTime > 0, "stat creator");
    status.permissions.mode = 0600 | 01000;
    status.permissions.user = 42;
    Require(shmctl_nid_postfix(first, Set, &status) == 0 && shmctl_nid_postfix(first, Stat, &status) == 0, "set");
    Require(status.permissions.mode == 0600 && status.permissions.user == 42, "set keeps only permission bits");
    Require(Fails(shmctl_nid_postfix(first, 99, &status), 22), "unknown command");

    Require(shmctl_nid_postfix(first, Remove, nullptr) == 0, "remove");
    Require(Fails(shmctl_nid_postfix(first, Stat, &status), 22), "removed identifier is gone");
    Require(Fails(shmget_nid_postfix(0x5150, 64, 0), 2), "removed key is gone");
    const int reused = shmget_nid_postfix(0x5150, 64, Create);
    Require(reused >= 0 && reused != first, "slot reuse gets a new identifier");
    Require(Fails(shmctl_nid_postfix(first, Stat, &status), 22), "stale identifier");
    Require(shmctl_nid_postfix(reused, Remove, nullptr) == 0 && shmctl_nid_postfix(other, Remove, nullptr) == 0, "cleanup");
}

void CheckAttach() {
    const int identifier = shmget_nid_postfix(0, 100000, Create | 0600);
    Require(identifier >= 0, "create");
    auto* memory = static_cast<unsigned char*>(shmat_nid_postfix(identifier, nullptr, 0));
    Require(memory != Failed && reinterpret_cast<std::uintptr_t>(memory) % 4096 == 0, "attach is page aligned");
    Require(memory[0] == 0 && memory[99999] == 0, "zero filled");
    std::memset(memory, 0x5a, 100000);
    auto* second = static_cast<unsigned char*>(shmat_nid_postfix(identifier, nullptr, 0));
    Require(second != Failed && second[12345] == 0x5a, "attachments share memory");
    Status status{};
    Require(shmctl_nid_postfix(identifier, Stat, &status) == 0 && status.attachments == 2 && status.attachTime > 0 && status.lastProcess == getpid_nid_postfix(), "attach count");

    Require(shmctl_nid_postfix(identifier, Remove, nullptr) == 0, "remove while attached");
    Require(memory[500] == 0x5a, "removed segment stays mapped");
    Require(shmat_nid_postfix(identifier, nullptr, 0) == Failed && *__error_nid_postfix() == 22, "removed segment cannot be attached");
    Require(shmdt_nid_postfix(memory) == 0 && shmdt_nid_postfix(second) == 0, "detach both");
    Require(Fails(shmdt_nid_postfix(memory), 22), "detach unattached address");
    Require(shmat_nid_postfix(12345, nullptr, 0) == Failed && *__error_nid_postfix() == 22, "invalid identifier");

    bool threw = false;
    try {
        shmat_nid_postfix(shmget_nid_postfix(0, 64, Create), reinterpret_cast<void*>(0x10000000), 0);
    } catch (const std::exception&) {
        threw = true;
    }
    Require(threw, "fixed address throws");
}

void CheckLimits() {
    const int identifier = shmget_nid_postfix(0, 64, Create);
    std::vector<void*> attached;
    for (int i = 0; i < 128; i++) {
        void* address = shmat_nid_postfix(identifier, nullptr, 0);
        if (address == Failed) break;
        attached.push_back(address);
    }
    Require(attached.size() == 128 && shmat_nid_postfix(identifier, nullptr, 0) == Failed && *__error_nid_postfix() == 24, "attach limit");
    for (void* address : attached) Require(shmdt_nid_postfix(address) == 0, "detach all");
    Require(shmctl_nid_postfix(identifier, Remove, nullptr) == 0, "remove");
}

}

int main() {
    CheckKeys();
    CheckAttach();
    CheckLimits();
    std::puts("guest shared memory segment tests passed");
    return 0;
}
