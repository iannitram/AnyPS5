#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
int APS5_VABI pthread_mutexattr_init_nid_postfix(void**);
int APS5_VABI pthread_mutexattr_destroy_nid_postfix(void**);
int APS5_VABI pthread_mutexattr_setprotocol_nid_postfix(void**, int);
int APS5_VABI pthread_mutexattr_setprioceiling_nid_postfix(void**, int);
int APS5_VABI pthread_mutexattr_gettype_nid_postfix(void* const*, int*);
int APS5_VABI pthread_mutexattr_getprotocol_nid_postfix(void* const*, int*);
int APS5_VABI pthread_mutexattr_getpshared_nid_postfix(void* const*, int*);
int APS5_VABI pthread_mutexattr_setpshared_nid_postfix(void**, int);
int APS5_VABI pthread_mutexattr_setkind_np_nid_postfix(void**, int);
int APS5_VABI pthread_mutexattr_getkind_np_nid_postfix(void*);
int APS5_VABI pthread_mutex_init_nid_postfix(void**, void* const*);
int APS5_VABI pthread_mutex_destroy_nid_postfix(void**);
int APS5_VABI pthread_mutex_lock_nid_postfix(void**);
int APS5_VABI pthread_mutex_unlock_nid_postfix(void**);
int APS5_VABI pthread_mutex_getprioceiling_nid_postfix(void* const*, int*);
int APS5_VABI pthread_mutex_setprioceiling_nid_postfix(void**, int, int*);
int APS5_VABI pthread_condattr_init_nid_postfix(void**);
int APS5_VABI pthread_condattr_destroy_nid_postfix(void**);
int APS5_VABI pthread_condattr_setclock_nid_postfix(void**, int);
int APS5_VABI pthread_condattr_getclock_nid_postfix(void* const*, int*);
int APS5_VABI pthread_condattr_getpshared_nid_postfix(void* const*, int*);
int APS5_VABI pthread_condattr_setpshared_nid_postfix(void**, int);
int APS5_VABI pthread_rwlockattr_init_nid_postfix(void**);
int APS5_VABI pthread_rwlockattr_destroy_nid_postfix(void**);
int APS5_VABI pthread_rwlockattr_getpshared_nid_postfix(void* const*, int*);
int APS5_VABI pthread_rwlockattr_setpshared_nid_postfix(void**, int);
int APS5_VABI pthread_getconcurrency_nid_postfix(void);
int APS5_VABI pthread_setconcurrency_nid_postfix(int);
int APS5_VABI pthread_atfork_nid_postfix(void (APS5_VABI *)(), void (APS5_VABI *)(), void (APS5_VABI *)());
void* APS5_VABI pthread_self_nid_postfix(void);
int APS5_VABI pthread_setname_np_nid_postfix(void*, const char*);
int APS5_VABI pthread_getname_np_nid_postfix(void*, char*);
int APS5_VABI pthread_attr_init_nid_postfix(void**);
int APS5_VABI pthread_attr_destroy_nid_postfix(void**);
int APS5_VABI pthread_attr_setstackaddr_nid_postfix(void**, void*);
int APS5_VABI pthread_attr_getstackaddr_nid_postfix(void* const*, void**);
int APS5_VABI pthread_attr_getaffinity_np_nid_postfix(void* const*, std::size_t, void*);
int APS5_VABI pthread_attr_setaffinity_np_nid_postfix(void**, std::size_t, const void*);
int APS5_VABI pthread_setaffinity_np_nid_postfix(void*, std::size_t, const void*);
int APS5_VABI pthread_getaffinity_np_nid_postfix(void*, std::size_t, void*);
int* APS5_VABI __error_nid_postfix();
int APS5_VABI pthread_create_nid_postfix(void**, void* const*, void* (APS5_VABI *)(void*), void*);
int APS5_VABI pthread_join_nid_postfix(void*, void**);
int APS5_VABI pthread_detach_nid_postfix(void*);
int APS5_VABI pthread_timedjoin_np_nid_postfix(void*, void**, const KernelTimespec*);
int APS5_VABI clock_gettime_nid_postfix(int, KernelTimespec*);
}

namespace {

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "failed: %s\n", message);
        std::abort();
    }
}

void APS5_VABI Handler() {}

std::atomic<bool> release{false};

void* APS5_VABI Sleeper(void* argument) {
    while (!release) std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return argument;
}

KernelTimespec In(long milliseconds) {
    KernelTimespec now{};
    clock_gettime_nid_postfix(0, &now);
    now.tv_nsec += milliseconds * 1000000;
    now.tv_sec += now.tv_nsec / 1000000000;
    now.tv_nsec %= 1000000000;
    return now;
}

void CheckTimedJoin() {
    void* thread = nullptr;
    int token = 7;
    Require(pthread_create_nid_postfix(&thread, nullptr, Sleeper, &token) == 0, "create");
    KernelTimespec soon = In(30);
    void* value = nullptr;
    Require(pthread_timedjoin_np_nid_postfix(thread, &value, &soon) == 60 && value == nullptr, "timed join times out");
    KernelTimespec invalid{0, 1000000000};
    Require(pthread_timedjoin_np_nid_postfix(thread, &value, &invalid) == 22 && pthread_timedjoin_np_nid_postfix(thread, &value, nullptr) == 22, "invalid timeout");
    Require(pthread_timedjoin_np_nid_postfix(pthread_self_nid_postfix(), &value, &soon) == 11, "self join deadlocks");
    Require(pthread_timedjoin_np_nid_postfix(nullptr, &value, &soon) == 22, "a null thread is EINVAL, as join_common checks");
    release = true;
    KernelTimespec later = In(5000);
    Require(pthread_timedjoin_np_nid_postfix(thread, &value, &later) == 0 && value == &token, "timed join returns the value");

    release = false;
    Require(pthread_create_nid_postfix(&thread, nullptr, Sleeper, nullptr) == 0 && pthread_detach_nid_postfix(thread) == 0, "detached thread");
    Require(pthread_timedjoin_np_nid_postfix(thread, &value, &later) == 22, "detached thread cannot be joined");
    release = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void CheckMutexAttributes() {
    void* attr = nullptr;
    int value = -1;
    Require(pthread_mutexattr_init_nid_postfix(&attr) == 0, "init");
    Require(pthread_mutexattr_getprotocol_nid_postfix(&attr, &value) == 0 && value == 0, "default protocol");
    Require(pthread_mutexattr_getpshared_nid_postfix(&attr, &value) == 0 && value == 0, "default pshared");
    Require(pthread_mutexattr_setpshared_nid_postfix(&attr, 1) == 0 && pthread_mutexattr_getpshared_nid_postfix(&attr, &value) == 0 && value == 1, "process shared");
    Require(pthread_mutexattr_setpshared_nid_postfix(&attr, 2) == 22, "invalid pshared");
    Require(pthread_mutexattr_setkind_np_nid_postfix(&attr, 2) == 0 && pthread_mutexattr_getkind_np_nid_postfix(attr) == 2, "kind");
    Require(pthread_mutexattr_gettype_nid_postfix(&attr, &value) == 0 && value == 2, "kind is the type");
    *__error_nid_postfix() = 0;
    Require(pthread_mutexattr_getkind_np_nid_postfix(nullptr) == -1 && *__error_nid_postfix() == 22, "null kind");
    void* missing = nullptr;
    Require(pthread_mutexattr_setkind_np_nid_postfix(&missing, 1) == -1 && *__error_nid_postfix() == 22, "uninitialized kind");
    Require(pthread_mutexattr_setkind_np_nid_postfix(&attr, 99) == 0, "setkind_np stores any kind");
    void* rejected = nullptr;
    Require(pthread_mutex_init_nid_postfix(&rejected, &attr) == 22 && rejected == nullptr, "mutex_init rejects an invalid kind, as mutex_check_attr does");
    Require(pthread_mutexattr_setkind_np_nid_postfix(&attr, 1) == 0, "restore type");

    void* mutex = nullptr;
    Require(pthread_mutex_init_nid_postfix(&mutex, &attr) == 0, "mutex init");
    Require(pthread_mutex_getprioceiling_nid_postfix(&mutex, &value) == 22, "no ceiling without PRIO_PROTECT");
    Require(pthread_mutex_destroy_nid_postfix(&mutex) == 0, "mutex destroy");
    Require(pthread_mutex_getprioceiling_nid_postfix(&mutex, &value) == 22 && pthread_mutex_setprioceiling_nid_postfix(&mutex, 1, nullptr) == 22, "a destroyed mutex has no ceiling");
    void* adaptive = reinterpret_cast<void*>(std::uintptr_t{1});
    Require(pthread_mutex_getprioceiling_nid_postfix(&adaptive, &value) == 22, "an initializer has no ceiling");

    Require(pthread_mutexattr_setprotocol_nid_postfix(&attr, 2) == 0 && pthread_mutexattr_setprioceiling_nid_postfix(&attr, 700) == 0, "PRIO_PROTECT");
    Require(pthread_mutexattr_getprotocol_nid_postfix(&attr, &value) == 0 && value == 2, "protocol");
    Require(pthread_mutex_init_nid_postfix(&mutex, &attr) == 0, "protected mutex");
    Require(pthread_mutex_getprioceiling_nid_postfix(&mutex, &value) == 0 && value == 700, "ceiling");
    int previous = 0;
    Require(pthread_mutex_setprioceiling_nid_postfix(&mutex, 600, &previous) == 0 && previous == 700, "set ceiling");
    Require(pthread_mutex_getprioceiling_nid_postfix(&mutex, &value) == 0 && value == 600, "new ceiling");
    Require(pthread_mutex_lock_nid_postfix(&mutex) == 0 && pthread_mutex_unlock_nid_postfix(&mutex) == 0, "protected mutex locks");
    Require(pthread_mutex_destroy_nid_postfix(&mutex) == 0 && pthread_mutexattr_destroy_nid_postfix(&attr) == 0, "cleanup");
    Require(pthread_mutexattr_getprotocol_nid_postfix(&missing, &value) == 22, "uninitialized attr");
}

void CheckOtherAttributes() {
    void* cond = nullptr;
    int value = -1;
    Require(pthread_condattr_init_nid_postfix(&cond) == 0, "condattr init");
    Require(pthread_condattr_getclock_nid_postfix(&cond, &value) == 0 && value == 0, "default clock");
    Require(pthread_condattr_setclock_nid_postfix(&cond, 4) == 0 && pthread_condattr_getclock_nid_postfix(&cond, &value) == 0 && value == 4, "monotonic clock");
    Require(pthread_condattr_getpshared_nid_postfix(&cond, &value) == 0 && value == 0, "cond pshared default");
    Require(pthread_condattr_setpshared_nid_postfix(&cond, 1) == 0 && pthread_condattr_getpshared_nid_postfix(&cond, &value) == 0 && value == 1, "cond pshared");
    Require(pthread_condattr_setpshared_nid_postfix(&cond, -1) == 22 && pthread_condattr_getclock_nid_postfix(&cond, nullptr) == 22, "cond errors");
    Require(pthread_condattr_destroy_nid_postfix(&cond) == 0, "condattr destroy");

    void* rwlock = nullptr;
    Require(pthread_rwlockattr_init_nid_postfix(&rwlock) == 0, "rwlockattr init");
    Require(pthread_rwlockattr_getpshared_nid_postfix(&rwlock, &value) == 0 && value == 0, "rwlock pshared default");
    Require(pthread_rwlockattr_setpshared_nid_postfix(&rwlock, 1) == 0 && pthread_rwlockattr_getpshared_nid_postfix(&rwlock, &value) == 0 && value == 1, "rwlock pshared");
    Require(pthread_rwlockattr_setpshared_nid_postfix(&rwlock, 3) == 22, "rwlock invalid pshared");
    Require(pthread_rwlockattr_destroy_nid_postfix(&rwlock) == 0, "rwlockattr destroy");

    Require(pthread_getconcurrency_nid_postfix() == 0 && pthread_setconcurrency_nid_postfix(4) == 0 && pthread_getconcurrency_nid_postfix() == 4, "concurrency");
    Require(pthread_setconcurrency_nid_postfix(-1) == 22 && pthread_getconcurrency_nid_postfix() == 4, "negative concurrency");
    Require(pthread_atfork_nid_postfix(Handler, nullptr, Handler) == 0, "atfork");
}

void CheckThreads() {
    void* self = pthread_self_nid_postfix();
    char name[64] = {};
    Require(pthread_setname_np_nid_postfix(self, "extras-test") == 0 && pthread_getname_np_nid_postfix(self, name) == 0 && std::strcmp(name, "extras-test") == 0, "setname");
    Require(pthread_setname_np_nid_postfix(self, std::string(40, 'n').c_str()) == 63, "name too long");
    Require(pthread_setname_np_nid_postfix(nullptr, "x") == 3, "no thread");

    void* attr = nullptr;
    int stack = 0;
    void* address = nullptr;
    Require(pthread_attr_init_nid_postfix(&attr) == 0, "attr init");
    Require(pthread_attr_setstackaddr_nid_postfix(&attr, &stack) == 0 && pthread_attr_getstackaddr_nid_postfix(&attr, &address) == 0 && address == &stack, "stackaddr");
    unsigned char cpus[32] = {};
    cpus[0] = 0x05;
    Require(pthread_attr_setaffinity_np_nid_postfix(&attr, sizeof(cpus), cpus) == 0, "attr setaffinity");
    unsigned char read[32];
    std::memset(read, 0xff, sizeof(read));
    Require(pthread_attr_getaffinity_np_nid_postfix(&attr, sizeof(read), read) == 0 && read[0] == 0x05 && read[1] == 0 && read[31] == 0, "attr getaffinity");
    Require(pthread_attr_getaffinity_np_nid_postfix(&attr, 8, read) == 34 && pthread_attr_setaffinity_np_nid_postfix(&attr, 16, cpus) == 34, "attr cpuset size");
    unsigned char wide[40] = {};
    wide[0] = 0x03;
    Require(pthread_attr_setaffinity_np_nid_postfix(&attr, sizeof(wide), wide) == 0, "a larger set with zero high bytes is accepted, as thr_attr.c does");
    std::memset(wide, 0xee, sizeof(wide));
    Require(pthread_attr_getaffinity_np_nid_postfix(&attr, sizeof(wide), wide) == 0 && wide[0] == 0x03 && wide[1] == 0 && wide[39] == 0, "a larger read zeroes the tail");
    wide[36] = 1;
    Require(pthread_attr_setaffinity_np_nid_postfix(&attr, sizeof(wide), wide) == 22, "high bytes past the kernel set are EINVAL");
    Require(pthread_attr_setaffinity_np_nid_postfix(&attr, 0, nullptr) == 0, "a null set clears the affinity");
    Require(pthread_attr_getaffinity_np_nid_postfix(&attr, sizeof(read), read) == 0 && read[0] == 0xff && read[31] == 0xff, "no stored set reads as all CPUs");
    Require(pthread_attr_destroy_nid_postfix(&attr) == 0, "attr destroy");
    void* missing = nullptr;
    Require(pthread_attr_setstackaddr_nid_postfix(&missing, &stack) == 22, "uninitialized attr");

    unsigned char original[32];
    Require(pthread_getaffinity_np_nid_postfix(self, sizeof(original), original) == 0, "getaffinity");
    cpus[0] = 0x01;
    Require(pthread_setaffinity_np_nid_postfix(self, sizeof(cpus), cpus) == 0, "setaffinity");
    Require(pthread_getaffinity_np_nid_postfix(self, sizeof(read), read) == 0 && read[0] == 0x01, "affinity applied");
    Require(pthread_setaffinity_np_nid_postfix(self, sizeof(original), original) == 0, "restore affinity");
    Require(pthread_setaffinity_np_nid_postfix(self, 4, cpus) == 34 && pthread_setaffinity_np_nid_postfix(nullptr, sizeof(cpus), cpus) == 3, "setaffinity errors");
    unsigned char none[32] = {};
    Require(pthread_setaffinity_np_nid_postfix(self, sizeof(none), none) == 11, "an empty set is EDEADLK, as cpuset_testupdate gives");
}

}

int main() {
    CheckMutexAttributes();
    CheckOtherAttributes();
    CheckThreads();
    CheckTimedJoin();
    std::puts("guest pthread extras tests passed");
    return 0;
}
