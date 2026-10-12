#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

struct SignalSet {
    std::uint32_t bits[4];
};

}

extern "C" {
int APS5_VABI posix_spawn_file_actions_init_nid_postfix(void**);
int APS5_VABI posix_spawn_file_actions_destroy_nid_postfix(void**);
int APS5_VABI posix_spawn_file_actions_addopen_nid_postfix(void**, int, const char*, int, std::uint16_t);
int APS5_VABI posix_spawn_file_actions_adddup2_nid_postfix(void**, int, int);
int APS5_VABI posix_spawn_file_actions_addclose_nid_postfix(void**, int);
int APS5_VABI posix_spawnattr_init_nid_postfix(void**);
int APS5_VABI posix_spawnattr_destroy_nid_postfix(void**);
int APS5_VABI posix_spawnattr_getflags_nid_postfix(void* const*, short*);
int APS5_VABI posix_spawnattr_setflags_nid_postfix(void**, short);
int APS5_VABI posix_spawnattr_getpgroup_nid_postfix(void* const*, int*);
int APS5_VABI posix_spawnattr_setpgroup_nid_postfix(void**, int);
int APS5_VABI posix_spawnattr_getschedparam_nid_postfix(void* const*, int*);
int APS5_VABI posix_spawnattr_setschedparam_nid_postfix(void**, const int*);
int APS5_VABI posix_spawnattr_getschedpolicy_nid_postfix(void* const*, int*);
int APS5_VABI posix_spawnattr_setschedpolicy_nid_postfix(void**, int);
int APS5_VABI posix_spawnattr_getsigdefault_nid_postfix(void* const*, SignalSet*);
int APS5_VABI posix_spawnattr_setsigdefault_nid_postfix(void**, const SignalSet*);
int APS5_VABI posix_spawnattr_getsigmask_nid_postfix(void* const*, SignalSet*);
int APS5_VABI posix_spawnattr_setsigmask_nid_postfix(void**, const SignalSet*);
int APS5_VABI posix_spawn_nid_postfix(int*, const char*, void* const*, void* const*, char* const*, char* const*);
int APS5_VABI posix_spawnp_nid_postfix(int*, const char*, void* const*, void* const*, char* const*, char* const*);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

template<typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

void CheckFileActions() {
    void* actions = nullptr;
    Require(posix_spawn_file_actions_init_nid_postfix(&actions) == 0 && actions != nullptr, "file actions init");
    Require(posix_spawn_file_actions_addopen_nid_postfix(&actions, 3, "/app0/log.txt", 0x0601, 0644) == 0, "addopen");
    Require(posix_spawn_file_actions_adddup2_nid_postfix(&actions, 3, 1) == 0, "adddup2");
    Require(posix_spawn_file_actions_addclose_nid_postfix(&actions, 3) == 0, "addclose");
    Require(posix_spawn_file_actions_addopen_nid_postfix(&actions, -1, "x", 0, 0) == 9, "addopen bad descriptor");
    Require(posix_spawn_file_actions_adddup2_nid_postfix(&actions, 1, -2) == 9, "adddup2 bad descriptor");
    Require(posix_spawn_file_actions_addclose_nid_postfix(&actions, -1) == 9, "addclose bad descriptor");
    for (int i = 0; i < 100; i++) Require(posix_spawn_file_actions_addclose_nid_postfix(&actions, i) == 0, "many actions");
    Require(Throws([&] { posix_spawn_file_actions_addopen_nid_postfix(&actions, 3, nullptr, 0, 0); }), "addopen null path");
    Require(posix_spawn_file_actions_destroy_nid_postfix(&actions) == 0, "file actions destroy");
    void* empty = nullptr;
    Require(posix_spawn_file_actions_init_nid_postfix(&empty) == 0 && posix_spawn_file_actions_destroy_nid_postfix(&empty) == 0, "empty list");
    void* missing = nullptr;
    Require(Throws([&] { posix_spawn_file_actions_addclose_nid_postfix(&missing, 1); }), "uninitialized list");
}

void CheckAttributes() {
    void* attributes = nullptr;
    Require(posix_spawnattr_init_nid_postfix(&attributes) == 0 && attributes != nullptr, "attr init");
    short flags = -1;
    int value = -1;
    SignalSet set{{1, 1, 1, 1}};
    Require(posix_spawnattr_getflags_nid_postfix(&attributes, &flags) == 0 && flags == 0, "default flags");
    Require(posix_spawnattr_getpgroup_nid_postfix(&attributes, &value) == 0 && value == 0, "default group");
    Require(posix_spawnattr_getsigmask_nid_postfix(&attributes, &set) == 0 && set.bits[0] == 0 && set.bits[3] == 0, "default mask");

    Require(posix_spawnattr_setflags_nid_postfix(&attributes, 0x3f) == 0, "set flags");
    Require(posix_spawnattr_setpgroup_nid_postfix(&attributes, 77) == 0, "set group");
    const int priority = 700;
    Require(posix_spawnattr_setschedparam_nid_postfix(&attributes, &priority) == 0, "set param");
    Require(posix_spawnattr_setschedpolicy_nid_postfix(&attributes, 2) == 0, "set policy");
    const SignalSet defaults{{0x4000, 0, 0, 0x80000000}};
    const SignalSet mask{{0x2, 0x4, 0x8, 0x10}};
    Require(posix_spawnattr_setsigdefault_nid_postfix(&attributes, &defaults) == 0, "set defaults");
    Require(posix_spawnattr_setsigmask_nid_postfix(&attributes, &mask) == 0, "set mask");

    Require(posix_spawnattr_getflags_nid_postfix(&attributes, &flags) == 0 && flags == 0x3f, "get flags");
    Require(posix_spawnattr_getpgroup_nid_postfix(&attributes, &value) == 0 && value == 77, "get group");
    Require(posix_spawnattr_getschedparam_nid_postfix(&attributes, &value) == 0 && value == 700, "get param");
    Require(posix_spawnattr_getschedpolicy_nid_postfix(&attributes, &value) == 0 && value == 2, "get policy");
    Require(posix_spawnattr_getsigdefault_nid_postfix(&attributes, &set) == 0 && std::memcmp(&set, &defaults, sizeof(set)) == 0, "get defaults");
    Require(posix_spawnattr_getsigmask_nid_postfix(&attributes, &set) == 0 && std::memcmp(&set, &mask, sizeof(set)) == 0, "get mask");
    Require(posix_spawnattr_destroy_nid_postfix(&attributes) == 0, "attr destroy");
}

void CheckSpawn() {
    int pid = 0;
    char* const arguments[] = {const_cast<char*>("tool"), nullptr};
    Require(Throws([&] { posix_spawn_nid_postfix(&pid, "/app0/tool", nullptr, nullptr, arguments, nullptr); }), "posix_spawn throws");
    Require(Throws([&] { posix_spawnp_nid_postfix(&pid, "tool", nullptr, nullptr, arguments, nullptr); }), "posix_spawnp throws");
}

}

int main() {
    std::array<void*, 10> api{};
    ApplicationHeapRegister_nid_no_patch(api.data());
    CheckFileActions();
    CheckAttributes();
    CheckSpawn();
    std::puts("guest spawn tests passed");
    return 0;
}
