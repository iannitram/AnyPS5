#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "Common.hpp"
#include "../include/Cancel.hpp"
#include "prx/libkernel/Time/include/TimedWait.hpp"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <cstdarg>
#include <list>
#include <mutex>
#include <stdexcept>
#include <string>

// The guest sem_t is at least 8 bytes; its first word holds a pointer to the host semaphore.
struct PosixSemaphore {
    std::mutex lock;
    TimedWait::Condition available;
    unsigned int count;
};

static constexpr int GUEST_EAGAIN = 35;
static constexpr int GUEST_EOVERFLOW = 84;
static constexpr unsigned int GUEST_SEM_VALUE_MAX = 0x7fffffffu;
static constexpr int GUEST_REALTIME_CLOCK = 0;

static constexpr int GUEST_ENOENT = 2;
static constexpr int GUEST_EEXIST = 17;
static constexpr int GUEST_ENAMETOOLONG = 63;
static constexpr int GUEST_O_CREAT = 0x0200;
static constexpr int GUEST_O_EXCL = 0x0800;
static constexpr std::size_t GUEST_PATH_MAX = 1024;
static constexpr std::size_t NAMED_PREFIX_LENGTH = sizeof("/tmp/SEMD") - 1;
static constexpr std::size_t GUEST_SEM_SIZE = 16;

struct NamedSemaphore {
    std::string name;
    bool linked = true;
    int openCount = 0;
    alignas(8) unsigned char storage[GUEST_SEM_SIZE]{};
};

static std::mutex namedLock;
static std::list<NamedSemaphore> namedSemaphores;

static int Fail(int error) {
    errno = error;
    return -1;
}

static PosixSemaphore* Get(void* sem) {
    if (!sem) return nullptr;
    PosixSemaphore* semaphore = nullptr;
    std::memcpy(&semaphore, sem, sizeof(semaphore));
    return semaphore;
}

static int WaitFor(PosixSemaphore* semaphore, const KernelUseconds* usec) {
    ThreadCancel::Check();
    std::unique_lock lock(semaphore->lock);
    const auto ready = [&] { return semaphore->count > 0; };
    if (usec) {
        if (!ThreadCancel::WaitUntil(semaphore->available, lock, TimedWait::DeadlineNanos(*usec), ready)) return Fail(PosixThread::GUEST_ETIMEDOUT);
    } else {
        ThreadCancel::WaitUntil(semaphore->available, lock, std::nullopt, ready);
    }
    --semaphore->count;
    return 0;
}

extern "C" {

int APS5_VABI sem_init_nid_postfix(void* sem, int pshared, unsigned int value) {
    (void)pshared;
    if (!sem || value > GUEST_SEM_VALUE_MAX) return Fail(PosixThread::GUEST_EINVAL);
    auto* semaphore = new PosixSemaphore();
    semaphore->count = value;
    std::memcpy(sem, &semaphore, sizeof(semaphore));
    return 0;
}

static int Destroy(void* sem) {
    auto* semaphore = Get(sem);
    if (!semaphore) return Fail(PosixThread::GUEST_EINVAL);
    delete semaphore;
    std::memset(sem, 0, sizeof(semaphore));
    return 0;
}

int APS5_VABI sem_destroy_nid_postfix(void* sem) {
    std::lock_guard lock(namedLock);
    for (const auto& entry : namedSemaphores)
        if (entry.storage == sem) return Fail(PosixThread::GUEST_EINVAL);
    return Destroy(sem);
}

int APS5_VABI sem_wait_nid_postfix(void* sem) {
    auto* semaphore = Get(sem);
    if (!semaphore) return Fail(PosixThread::GUEST_EINVAL);
    return WaitFor(semaphore, nullptr);
}

int APS5_VABI sem_trywait_nid_postfix(void* sem) {
    auto* semaphore = Get(sem);
    if (!semaphore) return Fail(PosixThread::GUEST_EINVAL);
    std::lock_guard lock(semaphore->lock);
    if (semaphore->count == 0) return Fail(GUEST_EAGAIN);
    --semaphore->count;
    return 0;
}

int APS5_VABI sem_timedwait_nid_postfix(void* sem, const KernelTimespec* abstime) {
    auto* semaphore = Get(sem);
    if (!semaphore) return Fail(PosixThread::GUEST_EINVAL);
    {
        std::lock_guard lock(semaphore->lock);
        if (semaphore->count > 0) {
            --semaphore->count;
            return 0;
        }
    }
    if (!abstime) return WaitFor(semaphore, nullptr);
    KernelUseconds usec = 0;
    if (!PosixThread::RelativeMicroseconds(GUEST_REALTIME_CLOCK, abstime, &usec)) return Fail(PosixThread::GUEST_EINVAL);
    return WaitFor(semaphore, &usec);
}

int APS5_VABI sem_reltimedwait_np_nid_postfix(void* sem, uint32_t usec) {
    auto* semaphore = Get(sem);
    if (!semaphore) return Fail(PosixThread::GUEST_EINVAL);
    const KernelUseconds timeout = usec;
    return WaitFor(semaphore, &timeout);
}

int APS5_VABI sem_post_nid_postfix(void* sem) {
    auto* semaphore = Get(sem);
    if (!semaphore) return Fail(PosixThread::GUEST_EINVAL);
    {
        std::lock_guard lock(semaphore->lock);
        if (semaphore->count == GUEST_SEM_VALUE_MAX) return Fail(GUEST_EOVERFLOW);
        ++semaphore->count;
    }
    semaphore->available.NotifyOne();
    return 0;
}

int APS5_VABI sem_getvalue_nid_postfix(void* sem, int* value) {
    auto* semaphore = Get(sem);
    if (!semaphore || !value) return Fail(PosixThread::GUEST_EINVAL);
    std::lock_guard lock(semaphore->lock);
    *value = static_cast<int>(semaphore->count);
    return 0;
}


void* APS5_VABI sem_open_nid_postfix(const char* name, int flags, ...) {
    if (name == nullptr) throw std::invalid_argument("sem_open: null name");
    if (name[0] != '/') {
        errno = PosixThread::GUEST_EINVAL;
        return nullptr;
    }
    ++name;
    if (NAMED_PREFIX_LENGTH + std::strlen(name) >= GUEST_PATH_MAX) {
        errno = GUEST_ENAMETOOLONG;
        return nullptr;
    }
    if (flags & ~(GUEST_O_CREAT | GUEST_O_EXCL)) {
        errno = PosixThread::GUEST_EINVAL;
        return nullptr;
    }
    unsigned int value = 0;
    if (flags & GUEST_O_CREAT) {
#ifdef _WIN32
        __builtin_sysv_va_list arguments;
        __builtin_sysv_va_start(arguments, flags);
        (void)__builtin_va_arg(arguments, int);
        value = __builtin_va_arg(arguments, unsigned int);
        __builtin_sysv_va_end(arguments);
#else
        va_list arguments;
        va_start(arguments, flags);
        (void)va_arg(arguments, int);
        value = va_arg(arguments, unsigned int);
        va_end(arguments);
#endif
    }
    std::lock_guard lock(namedLock);
    for (auto& named : namedSemaphores) {
        if (!named.linked || named.name != name) continue;
        if ((flags & (GUEST_O_CREAT | GUEST_O_EXCL)) == (GUEST_O_CREAT | GUEST_O_EXCL)) {
            errno = GUEST_EEXIST;
            return nullptr;
        }
        ++named.openCount;
        return named.storage;
    }
    if (!(flags & GUEST_O_CREAT)) {
        errno = GUEST_ENOENT;
        return nullptr;
    }
    auto& named = namedSemaphores.emplace_back();
    named.name = name;
    if (sem_init_nid_postfix(named.storage, 1, value) != 0) {
        namedSemaphores.pop_back();
        return nullptr;
    }
    named.openCount = 1;
    return named.storage;
}

int APS5_VABI sem_close_nid_postfix(void* sem) {
    std::lock_guard lock(namedLock);
    for (auto entry = namedSemaphores.begin(); entry != namedSemaphores.end(); ++entry) {
        if (entry->storage != sem || entry->openCount == 0) continue;
        if (--entry->openCount == 0 && !entry->linked) {
            Destroy(entry->storage);
            namedSemaphores.erase(entry);
        }
        return 0;
    }
    return Fail(PosixThread::GUEST_EINVAL);
}

int APS5_VABI sem_unlink_nid_postfix(const char* name) {
    if (name == nullptr) throw std::invalid_argument("sem_unlink: null name");
    if (name[0] != '/') return Fail(GUEST_ENOENT);
    ++name;
    if (NAMED_PREFIX_LENGTH + std::strlen(name) >= GUEST_PATH_MAX) return Fail(GUEST_ENAMETOOLONG);
    std::lock_guard lock(namedLock);
    for (auto entry = namedSemaphores.begin(); entry != namedSemaphores.end(); ++entry) {
        if (!entry->linked || entry->name != name) continue;
        entry->linked = false;
        if (entry->openCount == 0) {
            Destroy(entry->storage);
            namedSemaphores.erase(entry);
        }
        return 0;
    }
    return Fail(GUEST_ENOENT);
}

}
