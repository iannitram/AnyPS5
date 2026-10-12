#include <atomic>
#include <cstdint>

#include "prx/libc/include/General.hpp"
#include "prx/libkernel/System/include/SignalMask.hpp"

namespace {

using SignalSet = GuestSignalSet;
using SignalAction = GuestSigaction;
static_assert(sizeof(SignalAction) == 32);

struct SignalVector {
    std::uintptr_t handler;
    int mask;
    int flags;
};
static_assert(sizeof(SignalVector) == 16);

constexpr int Block = 1;
constexpr int Unblock = 2;
constexpr int SetMask = 3;
constexpr int Restart = 0x2;
constexpr int Interrupt = Restart;
constexpr int MaxSignal = 128;
constexpr std::uintptr_t Ignore = 1;
constexpr std::uintptr_t Hold = 3;
constexpr std::uintptr_t Error = static_cast<std::uintptr_t>(-1);

std::atomic<std::uint32_t> interrupting[4];

}

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI sigaction_nid_postfix(int signal, const SignalAction* action, SignalAction* previous);
int APS5_VABI _sigprocmask_nid_postfix(int how, const SignalSet* set, SignalSet* previous);
int APS5_VABI sigemptyset_nid_postfix(SignalSet* set);
int APS5_VABI sigaddset_nid_postfix(SignalSet* set, int signal);
int APS5_VABI sigismember_nid_postfix(const SignalSet* set, int signal);

bool SignalInterrupts_nid_no_patch(int signal) {
    if (signal < 1 || signal > MaxSignal) return false;
    const int bit = signal - 1;
    return (interrupting[bit / 32].load() >> (bit % 32)) & 1;
}

int APS5_VABI sigvec_nid_postfix(int signal, const SignalVector* vector, SignalVector* previous) {
    SignalAction action{};
    SignalAction old{};
    if (vector != nullptr) {
        action.handler = vector->handler;
        action.flags = vector->flags ^ Interrupt;
        sigemptyset_nid_postfix(&action.mask);
        action.mask.bits[0] = static_cast<std::uint32_t>(vector->mask);
    }
    const int result = sigaction_nid_postfix(signal, vector != nullptr ? &action : nullptr, previous != nullptr ? &old : nullptr);
    if (result == 0 && previous != nullptr) {
        previous->handler = old.handler;
        previous->flags = old.flags ^ Interrupt;
        previous->mask = static_cast<int>(old.mask.bits[0]);
    }
    return result;
}

int APS5_VABI sigsetmask_nid_postfix(int mask) {
    SignalSet set{};
    SignalSet old{};
    sigemptyset_nid_postfix(&set);
    set.bits[0] = static_cast<std::uint32_t>(mask);
    const int result = _sigprocmask_nid_postfix(SetMask, &set, &old);
    return result != 0 ? result : static_cast<int>(old.bits[0]);
}

int APS5_VABI sigblock_nid_postfix(int mask) {
    SignalSet set{};
    SignalSet old{};
    sigemptyset_nid_postfix(&set);
    set.bits[0] = static_cast<std::uint32_t>(mask);
    const int result = _sigprocmask_nid_postfix(Block, &set, &old);
    return result != 0 ? result : static_cast<int>(old.bits[0]);
}

int APS5_VABI sighold_nid_postfix(int signal) {
    SignalSet set{};
    sigemptyset_nid_postfix(&set);
    if (sigaddset_nid_postfix(&set, signal) == -1) return -1;
    return _sigprocmask_nid_postfix(Block, &set, nullptr);
}

int APS5_VABI sigrelse_nid_postfix(int signal) {
    SignalSet set{};
    sigemptyset_nid_postfix(&set);
    if (sigaddset_nid_postfix(&set, signal) == -1) return -1;
    return _sigprocmask_nid_postfix(Unblock, &set, nullptr);
}

int APS5_VABI sigignore_nid_postfix(int signal) {
    SignalAction action{};
    action.handler = Ignore;
    return sigaction_nid_postfix(signal, &action, nullptr);
}

std::uintptr_t APS5_VABI sigset_nid_postfix(int signal, std::uintptr_t disposition) {
    SignalSet set{};
    SignalSet previousMask{};
    SignalAction previous{};
    sigemptyset_nid_postfix(&set);
    if (sigaddset_nid_postfix(&set, signal) == -1) return Error;
    if (_sigprocmask_nid_postfix(Block, nullptr, &previousMask) == -1) return Error;
    if (disposition == Hold) {
        if (_sigprocmask_nid_postfix(Block, &set, &previousMask) == -1) return Error;
        if (sigismember_nid_postfix(&previousMask, signal)) return Hold;
        if (sigaction_nid_postfix(signal, nullptr, &previous) == -1) return Error;
        return previous.handler;
    }
    if (_sigprocmask_nid_postfix(Unblock, &set, &previousMask) == -1) return Error;
    SignalAction action{};
    action.handler = disposition;
    if (sigaction_nid_postfix(signal, &action, &previous) == -1) return Error;
    return sigismember_nid_postfix(&previousMask, signal) ? Hold : previous.handler;
}

int APS5_VABI siginterrupt_nid_postfix(int signal, int flag) {
    SignalAction action{};
    const int result = sigaction_nid_postfix(signal, nullptr, &action);
    if (result < 0) return result;
    const int bit = signal - 1;
    if (flag) {
        interrupting[bit / 32].fetch_or(1u << (bit % 32));
        action.flags &= ~Restart;
    } else {
        interrupting[bit / 32].fetch_and(~(1u << (bit % 32)));
        action.flags |= Restart;
    }
    return sigaction_nid_postfix(signal, &action, nullptr);
}

int APS5_VABI sigpending_nid_postfix(SignalSet* set) {
    if (set == nullptr) {
        *__error_nid_postfix() = 14;
        return -1;
    }
    *set = PendingGuestSignals();
    return 0;
}

}
