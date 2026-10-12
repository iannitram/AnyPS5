#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libkernel/Pthread/Posix/Common.hpp"
#include "prx/libkernel/System/include/SignalMask.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

extern "C" int* APS5_VABI __error_nid_postfix();
extern "C" int APS5_VABI getpid_nid_postfix(void);
extern "C" bool SignalInterrupts_nid_no_patch(int signal);
extern "C" Pthread APS5_VABI scePthreadSelf(void);

static_assert(sizeof(GuestSigaction) == 32 && offsetof(GuestSigaction, flags) == 8 && offsetof(GuestSigaction, mask) == 12);

namespace {
using GuestHandler = void (APS5_VABI *)(int);
constexpr int MaxSignal = 128;
constexpr int GuestSigkill = 9;
constexpr int GuestSigstop = 17;
constexpr int GuestSigthr = 32;
constexpr int SaRestart = 0x2;
constexpr int SaSiginfo = 0x40;
std::atomic<GuestHandler> handlers[MaxSignal + 1]{};
static_assert(std::atomic<GuestHandler>::is_always_lock_free);
struct AtomicSignalSet {
    std::atomic<std::uint32_t> bits[4]{};
    GuestSignalSet Load() const {
        GuestSignalSet set{};
        for (int word = 0; word < 4; ++word) set.bits[word] = bits[word].load(std::memory_order_relaxed);
        return set;
    }
    void Store(const GuestSignalSet& set) {
        for (int word = 0; word < 4; ++word) bits[word].store(set.bits[word], std::memory_order_relaxed);
    }
    bool Contains(int guest) const {
        return (bits[(guest - 1) / 32].load(std::memory_order_relaxed) & (1u << ((guest - 1) % 32))) != 0;
    }
    void Add(int guest) {
        bits[(guest - 1) / 32].fetch_or(1u << ((guest - 1) % 32), std::memory_order_relaxed);
    }
    bool Take(int guest) {
        const std::uint32_t bit = 1u << ((guest - 1) % 32);
        return (bits[(guest - 1) / 32].fetch_and(~bit, std::memory_order_relaxed) & bit) != 0;
    }
};
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
thread_local AtomicSignalSet threadMask;
AtomicSignalSet pending;
std::mutex registration;
GuestSigaction dispositions[MaxSignal + 1]{};
int NativeSignal(int guest) {
    switch (guest) {
        case 2: return SIGINT;
        case 4: return SIGILL;
        case 6: return SIGABRT;
        case 8: return SIGFPE;
        case 11: return SIGSEGV;
        case 15: return SIGTERM;
        default: return 0;
    }
}
void Dispatch(int native) {
    int guest = 0;
    for (int candidate : {2, 4, 6, 8, 11, 15})
        if (NativeSignal(candidate) == native) { guest = candidate; break; }
    if (!guest) return;
#ifdef _WIN32
    // Preserve the guest's persistent registration across CRT delivery.
    std::signal(native, Dispatch);
#endif
    if (threadMask.Contains(guest)) {
        pending.Add(guest);
        return;
    }
    const auto callback = handlers[guest].load();
    if (reinterpret_cast<std::uintptr_t>(callback) > 1) callback(guest);
}
bool Delivered(const GuestSigaction& action) {
    return action.handler > 1 && (action.flags & SaSiginfo) == 0;
}
bool Install(int guest, const GuestSigaction& action) {
    const int native = NativeSignal(guest);
    if (!native) return true;
    if (action.handler == 1) pending.Take(guest);
    const auto previous = handlers[guest].exchange(Delivered(action) ? reinterpret_cast<GuestHandler>(action.handler) : nullptr);
    auto hostHandler = action.handler == 1 ? SIG_IGN : Delivered(action) ? Dispatch : SIG_DFL;
    if (std::signal(native, hostHandler) == SIG_ERR) {
        handlers[guest].store(previous);
        return false;
    }
    return true;
}
}

namespace {
constexpr int GuestSigalrm = 14;
struct AlarmState {
    std::mutex lock;
    std::condition_variable changed;
    std::optional<std::chrono::steady_clock::time_point> deadline;
    std::chrono::microseconds interval{0};
    bool threadStarted = false;
};

AlarmState& Alarms() {
    static auto* state = new AlarmState();
    return *state;
}

void ExpireAlarm() {
    std::uintptr_t handler = 0;
    {
        std::lock_guard lock(registration);
        handler = dispositions[GuestSigalrm].handler;
    }
    if (handler == 1) return;
    if (handler == 0) {
        std::fprintf(stderr, "SIGALRM: the alarm expired with the default disposition; the process terminates\n");
        std::fflush(stderr);
        std::_Exit(128 + GuestSigalrm);
    }
    throw std::runtime_error("alarm: delivering SIGALRM to a guest handler is not implemented");
}

void RequireDeliverableAlarm(int guest, std::uintptr_t handler) {
    if (guest != GuestSigalrm || handler <= 1) return;
    auto& alarms = Alarms();
    std::lock_guard lock(alarms.lock);
    if (alarms.deadline) NotImplemented_nid_no_patch("sigaction: delivering SIGALRM from an armed timer to a guest handler");
}

void RunAlarms() {
    auto& alarms = Alarms();
    std::unique_lock lock(alarms.lock);
    for (;;) {
        if (!alarms.deadline) {
            alarms.changed.wait(lock);
            continue;
        }
        const auto deadline = *alarms.deadline;
        if (alarms.changed.wait_until(lock, deadline) != std::cv_status::timeout || alarms.deadline != deadline) continue;
        if (alarms.interval.count() > 0) {
            auto next = deadline + alarms.interval;
            const auto now = std::chrono::steady_clock::now();
            if (next <= now) next += alarms.interval * ((now - next) / alarms.interval + 1);
            alarms.deadline = next;
        } else {
            alarms.deadline.reset();
        }
        lock.unlock();
        ExpireAlarm();
        lock.lock();
    }
}
}

namespace {
GuestSignalSet Maskable(GuestSignalSet set) {
    for (int fixed : {GuestSigkill, GuestSigstop, GuestSigthr})
        set.bits[(fixed - 1) / 32] &= ~(1u << ((fixed - 1) % 32));
    return set;
}

void DeliverUnblocked() {
    for (int guest : {2, 4, 6, 8, 11, 15})
        if (!threadMask.Contains(guest) && pending.Take(guest)) std::raise(NativeSignal(guest));
}

int ChangeSignalMask(int how, const GuestSignalSet* set, GuestSignalSet* previousSet) {
    const GuestSignalSet current = threadMask.Load();
    if (set != nullptr) {
        const GuestSignalSet requested = *set;
        GuestSignalSet next = current;
        switch (how) {
            case 1:
                for (int word = 0; word < 4; ++word) next.bits[word] |= Maskable(requested).bits[word];
                break;
            case 2:
                for (int word = 0; word < 4; ++word) next.bits[word] &= ~requested.bits[word];
                break;
            case 3: next = Maskable(requested); break;
            default: return 22;
        }
        threadMask.Store(next);
    }
    if (previousSet != nullptr) *previousSet = current;
    if (set != nullptr) DeliverUnblocked();
    return 0;
}
}

GuestSignalSet CurrentGuestSignalMask() {
    return threadMask.Load();
}

void SetCurrentGuestSignalMask(const GuestSignalSet& mask) {
    threadMask.Store(Maskable(mask));
    DeliverUnblocked();
}

GuestSignalSet PendingGuestSignals() {
    return pending.Load();
}

struct GuestItimerval {
    KernelTimeval interval;
    KernelTimeval value;
};
static_assert(sizeof(GuestItimerval) == 32 && offsetof(GuestItimerval, value) == 16);

extern "C" int APS5_VABI getitimer_nid_postfix(int which, GuestItimerval* output) {
    if (which < 0 || which > 2) {
        *__error_nid_postfix() = 22;
        return -1;
    }
    if (which != 0) throw std::runtime_error("getitimer: CPU interval timers are not implemented");
    if (output == nullptr) {
        *__error_nid_postfix() = 14;
        return -1;
    }
    GuestItimerval result{};
    auto& alarms = Alarms();
    {
        std::lock_guard lock(alarms.lock);
        const auto now = std::chrono::steady_clock::now();
        if (alarms.deadline && *alarms.deadline > now) {
            const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(*alarms.deadline - now).count();
            result.value.tv_sec = micros / 1000000;
            result.value.tv_usec = micros % 1000000;
            result.interval.tv_sec = alarms.interval.count() / 1000000;
            result.interval.tv_usec = alarms.interval.count() % 1000000;
        }
    }
    *output = result;
    return 0;
}

namespace {
bool InvalidTimerValue(const KernelTimeval& value) {
    return value.tv_sec < 0 || value.tv_usec < 0 || value.tv_usec >= 1000000 || value.tv_sec > INT32_MAX / 2;
}

std::chrono::microseconds Microseconds(const KernelTimeval& value) {
    return std::chrono::seconds(value.tv_sec) + std::chrono::microseconds(value.tv_usec);
}
}

extern "C" int APS5_VABI setitimer_nid_postfix(int which, const GuestItimerval* input, GuestItimerval* output) {
    if (which < 0 || which > 2) {
        *__error_nid_postfix() = 22;
        return -1;
    }
    if (input == nullptr) return getitimer_nid_postfix(which, output);
    if (which != 0) throw std::runtime_error("setitimer: CPU interval timers are not implemented");
    GuestItimerval request = *input;
    if (InvalidTimerValue(request.value)) {
        *__error_nid_postfix() = 22;
        return -1;
    }
    const bool arming = request.value.tv_sec != 0 || request.value.tv_usec != 0;
    if (!arming) request.interval = {};
    else if (InvalidTimerValue(request.interval)) {
        *__error_nid_postfix() = 22;
        return -1;
    }
    if (arming) {
        std::lock_guard lock(registration);
        if (dispositions[GuestSigalrm].handler > 1) NotImplemented_nid_no_patch("setitimer: delivering SIGALRM to a guest handler");
    }
    GuestItimerval previous{};
    auto& alarms = Alarms();
    {
        std::lock_guard lock(alarms.lock);
        const auto now = std::chrono::steady_clock::now();
        if (alarms.deadline && *alarms.deadline > now) {
            const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(*alarms.deadline - now).count();
            previous.value.tv_sec = micros / 1000000;
            previous.value.tv_usec = micros % 1000000;
            previous.interval.tv_sec = alarms.interval.count() / 1000000;
            previous.interval.tv_usec = alarms.interval.count() % 1000000;
        }
        alarms.interval = Microseconds(request.interval);
        if (arming) alarms.deadline = now + Microseconds(request.value);
        else alarms.deadline.reset();
        if (!alarms.threadStarted) {
            std::thread(RunAlarms).detach();
            alarms.threadStarted = true;
        }
        alarms.changed.notify_all();
    }
    if (output != nullptr) *output = previous;
    return 0;
}

extern "C" unsigned int APS5_VABI GuestAlarm_nid_no_patch(unsigned int seconds) {
    GuestItimerval request{};
    request.value.tv_sec = seconds;
    GuestItimerval previous{};
    if (setitimer_nid_postfix(0, &request, &previous) != 0) return static_cast<unsigned int>(-1);
    if (previous.value.tv_usec != 0) ++previous.value.tv_sec;
    return static_cast<unsigned int>(previous.value.tv_sec);
}

extern "C" std::uint32_t APS5_VABI ualarm_nid_postfix(std::uint32_t microseconds, std::uint32_t reload) {
    GuestItimerval request{};
    request.interval.tv_sec = reload / 1000000;
    request.interval.tv_usec = reload % 1000000;
    request.value.tv_sec = microseconds / 1000000;
    request.value.tv_usec = microseconds % 1000000;
    GuestItimerval previous{};
    if (setitimer_nid_postfix(0, &request, &previous) != 0) return static_cast<std::uint32_t>(-1);
    return static_cast<std::uint32_t>(previous.value.tv_sec * 1000000 + previous.value.tv_usec);
}

struct GuestStack {
    void* sp;
    std::size_t size;
    int flags;
};
static_assert(sizeof(GuestStack) == 24 && offsetof(GuestStack, flags) == 16);
namespace {
constexpr int SsDisable = 4;
constexpr std::size_t MinSignalStackSize = 2048;
thread_local GuestStack alternateStack{nullptr, 0, SsDisable};
}

extern "C" {
GuestHandler APS5_VABI signal_nid_postfix(int guest, GuestHandler handler) {
    const auto invalid = reinterpret_cast<GuestHandler>(static_cast<std::uintptr_t>(-1));
    if (guest < 1 || guest > MaxSignal || ((guest == GuestSigkill || guest == GuestSigstop) && handler != nullptr)) { *__error_nid_postfix() = 22; return invalid; }
    std::lock_guard lock(registration);
    RequireDeliverableAlarm(guest, reinterpret_cast<std::uintptr_t>(handler));
    const GuestSigaction action{reinterpret_cast<std::uintptr_t>(handler), SignalInterrupts_nid_no_patch(guest) ? 0 : SaRestart, {}};
    if (!Install(guest, action)) { *__error_nid_postfix() = 22; return invalid; }
    const auto previous = reinterpret_cast<GuestHandler>(dispositions[guest].handler);
    dispositions[guest] = action;
    return previous;
}
int APS5_VABI sigaction_nid_postfix(int guest, const GuestSigaction* action, GuestSigaction* previous) {
    if (guest < 1 || guest > MaxSignal) { *__error_nid_postfix() = 22; return -1; }
    if (action && (guest == GuestSigkill || guest == GuestSigstop) && action->handler != 0) { *__error_nid_postfix() = 22; return -1; }
    std::lock_guard lock(registration);
    const GuestSigaction old = dispositions[guest];
    if (action) {
        RequireDeliverableAlarm(guest, action->handler);
        if (!Install(guest, *action)) { *__error_nid_postfix() = 22; return -1; }
        dispositions[guest] = *action;
    }
    if (previous) *previous = old;
    return 0;
}
int APS5_VABI raise_nid_postfix(int guest) {
    const int native = NativeSignal(guest);
    if (!native) { *__error_nid_postfix() = 22; return -1; }
    {
        std::lock_guard lock(registration);
        if (dispositions[guest].handler > 1 && !Delivered(dispositions[guest])) throw std::runtime_error("raise: SA_SIGINFO handlers are not delivered");
    }
    const int result = std::raise(native);
    if (result) *__error_nid_postfix() = 22;
    return result ? -1 : 0;
}
int APS5_VABI kill_nid_postfix(int pid, int guest) {
    if (guest < 0 || guest > MaxSignal) { *__error_nid_postfix() = 22; return -1; }
    const int self = getpid_nid_postfix();
    if (pid != self && pid != 0 && pid != -self) { *__error_nid_postfix() = 3; return -1; }
    return guest == 0 ? 0 : raise_nid_postfix(guest);
}
int APS5_VABI pthread_kill_nid_postfix(Pthread thread, int guest) {
    if (guest < 0 || guest > MaxSignal) return PosixThread::GUEST_EINVAL;
    if (thread == nullptr) return PosixThread::GUEST_ESRCH;
    if (guest == 0) return 0;
    if (thread != scePthreadSelf()) NotImplemented_nid_no_patch(("pthread_kill signal " + std::to_string(guest) + " to another thread").c_str());
    return raise_nid_postfix(guest) == 0 ? 0 : *__error_nid_postfix();
}
int APS5_VABI sigaltstack_nid_postfix(const GuestStack* stack, GuestStack* previous) {
    GuestStack replacement = alternateStack;
    if (stack != nullptr) {
        if ((stack->flags & ~SsDisable) != 0) { *__error_nid_postfix() = 22; return -1; }
        if (stack->flags & SsDisable) replacement.flags = SsDisable;
        else if (stack->size < MinSignalStackSize) { *__error_nid_postfix() = 12; return -1; }
        else replacement = {stack->sp, stack->size, 0};
    }
    if (previous != nullptr) *previous = alternateStack;
    alternateStack = replacement;
    return 0;
}
int APS5_VABI _sigprocmask_nid_postfix(int how, const GuestSignalSet* set, GuestSignalSet* previousSet) {
    const int error = ChangeSignalMask(how, set, previousSet);
    if (error == 0) return 0;
    *__error_nid_postfix() = error;
    return -1;
}

int APS5_VABI sigprocmask_nid_postfix(int how, const void* set, void* previousSet) {
    return _sigprocmask_nid_postfix(how, static_cast<const GuestSignalSet*>(set),
                                    static_cast<GuestSignalSet*>(previousSet));
}

int APS5_VABI pthread_sigmask_nid_postfix(int how, const GuestSignalSet* set, GuestSignalSet* previousSet) {
    return ChangeSignalMask(how, set, previousSet);
}

int APS5_VABI sigtimedwait_nid_postfix(const GuestSignalSet* set, void* info, const void* timeout) {
    (void)set;
    (void)info;
    (void)timeout;
    NotImplemented_nid_no_patch("sigtimedwait: waiting for a pending signal");
    return -1;
}

int APS5_VABI sigwaitinfo_nid_postfix(const GuestSignalSet* set, void* info) {
    (void)set;
    (void)info;
    NotImplemented_nid_no_patch("sigwaitinfo: waiting for a pending signal");
    return -1;
}

int APS5_VABI sigwait_nid_postfix(const GuestSignalSet* set, int* received) {
    (void)set;
    (void)received;
    NotImplemented_nid_no_patch("sigwait: waiting for a pending signal");
    return -1;
}

int APS5_VABI pause_nid_postfix(void) {
    NotImplemented_nid_no_patch("pause: waiting for a delivered signal");
    return -1;
}
}

extern "C" {

int APS5_VABI _is_signal_return_nid_postfix(std::uint64_t programCounter) {
    (void)programCounter;
    return 0;
}

}
