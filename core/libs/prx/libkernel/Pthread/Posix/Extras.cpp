#include "prx/libkernel/include/Api.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "../include/Pthread.hpp"
#include "Common.hpp"
#include "prx/libkernel/KernelErrors.hpp"

extern "C" {
int APS5_VABI scePthreadSetaffinity(Pthread thread, KernelCpumask mask);
int APS5_VABI scePthreadAttrGetaffinity(const PthreadAttr* attr, KernelCpumask* mask);
int APS5_VABI scePthreadAttrSetaffinity(PthreadAttr* attr, KernelCpumask mask);
int APS5_VABI scePthreadRename(Pthread thread, const char* name);
}

namespace {

constexpr int ProcessPrivate = 0;
constexpr int ProcessShared = 1;
constexpr int PriorityProtect = 2;
constexpr std::size_t CpusetBytes = 32;
constexpr int OutOfRange = 34;
constexpr int Deadlock = 11;
constexpr std::uintptr_t LastMutexSentinel = 2;

using ForkHandler = void (APS5_VABI *)();

struct ForkHandlers {
    ForkHandler prepare;
    ForkHandler parent;
    ForkHandler child;
};

std::mutex forkLock;
std::vector<ForkHandlers> forkHandlers;
int concurrency = 0;

bool ValidShared(int shared) { return shared == ProcessPrivate || shared == ProcessShared; }

bool AttributeSet(const PthreadAttr* attr) { return attr != nullptr && *attr != nullptr; }

bool LiveMutex(const PthreadMutex* mutex) { return mutex != nullptr && reinterpret_cast<std::uintptr_t>(*mutex) > LastMutexSentinel; }

bool ZeroFrom(const void* bytes, std::size_t start, std::size_t end) {
    for (std::size_t index = start; index < end; index++)
        if (static_cast<const unsigned char*>(bytes)[index] != 0) return false;
    return true;
}

}

extern "C" {






int APS5_VABI pthread_mutex_getprioceiling_nid_postfix(const PthreadMutex* mutex, int* ceiling) {
    if (!LiveMutex(mutex) || (*mutex)->_protocol != PriorityProtect) return PosixThread::GUEST_EINVAL;
    if (ceiling == nullptr) throw std::invalid_argument("pthread_mutex_getprioceiling: null ceiling");
    *ceiling = (*mutex)->_ceiling;
    return 0;
}

int APS5_VABI pthread_mutex_setprioceiling_nid_postfix(PthreadMutex* mutex, int ceiling, int* previous) {
    if (!LiveMutex(mutex) || (*mutex)->_protocol != PriorityProtect) return PosixThread::GUEST_EINVAL;
    if (previous != nullptr) *previous = (*mutex)->_ceiling;
    (*mutex)->_ceiling = ceiling;
    return 0;
}

int APS5_VABI pthread_condattr_getclock_nid_postfix(const PthreadCondattr* attr, int* clock) {
    if (attr == nullptr || *attr == nullptr || clock == nullptr) return PosixThread::GUEST_EINVAL;
    *clock = (*attr)->_clockid;
    return 0;
}








int APS5_VABI pthread_setname_np_nid_postfix(Pthread thread, const char* name) {
    if (thread == nullptr) return PosixThread::GUEST_ESRCH;
    if (name != nullptr && std::strlen(name) >= 32) return PosixThread::ToErrno(SCE_KERNEL_ERROR_ENAMETOOLONG);
    return PosixThread::ToErrno(scePthreadRename(thread, name != nullptr ? name : ""));
}

int APS5_VABI pthread_attr_setstackaddr_nid_postfix(PthreadAttr* attr, void* address) {
    if (!AttributeSet(attr)) return PosixThread::GUEST_EINVAL;
    (*attr)->stackAddress = address;
    return 0;
}

int APS5_VABI pthread_attr_getaffinity_np_nid_postfix(const PthreadAttr* attr, std::size_t size, void* cpuset) {
    if (!AttributeSet(attr)) return PosixThread::GUEST_EINVAL;
    if (size < CpusetBytes) return OutOfRange;
    if (cpuset == nullptr) throw std::invalid_argument("pthread_attr_getaffinity_np: null cpuset");
    std::memset(cpuset, 0, size);
    if (!(*attr)->_affinityExplicit) {
        std::memset(cpuset, 0xff, CpusetBytes);
        return 0;
    }
    KernelCpumask mask = 0;
    const int result = scePthreadAttrGetaffinity(attr, &mask);
    if (result != 0) return PosixThread::ToErrno(result);
    std::memcpy(cpuset, &mask, sizeof(mask));
    return 0;
}

int APS5_VABI pthread_attr_setaffinity_np_nid_postfix(PthreadAttr* attr, std::size_t size, const void* cpuset) {
    if (!AttributeSet(attr)) return PosixThread::GUEST_EINVAL;
    if (size == 0 || cpuset == nullptr) {
        (*attr)->_affinity = DEFAULT_THREAD_AFFINITY;
        (*attr)->_affinityExplicit = false;
        return 0;
    }
    if (size < CpusetBytes) return OutOfRange;
    if (!ZeroFrom(cpuset, sizeof(KernelCpumask), size)) return PosixThread::GUEST_EINVAL;
    KernelCpumask mask = 0;
    std::memcpy(&mask, cpuset, sizeof(mask));
    return PosixThread::ToErrno(scePthreadAttrSetaffinity(attr, mask));
}


}
