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
}

namespace {

constexpr int ProcessPrivate = 0;
constexpr int ProcessShared = 1;
constexpr int PriorityProtect = 2;
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

bool LiveMutex(const PthreadMutex* mutex) { return mutex != nullptr && reinterpret_cast<std::uintptr_t>(*mutex) > LastMutexSentinel; }

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

}
