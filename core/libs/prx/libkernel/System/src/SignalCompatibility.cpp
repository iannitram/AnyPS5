#include "prx/libkernel/include/Api.hpp"
#include <atomic>
#include <cstdint>

#include "prx/libc/include/General.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libc/include/Api.hpp"

namespace {

using SignalSet = GuestSignalSet;
using SignalAction = GuestSigaction;
static_assert(sizeof(SignalAction) == 32);

constexpr int Block = 1;
constexpr int Unblock = 2;
constexpr int SetMask = 3;
constexpr int Restart = 0x2;
constexpr int Interrupt = Restart;
constexpr std::uintptr_t Ignore = 1;
constexpr std::uintptr_t Hold = 3;
constexpr std::uintptr_t Error = static_cast<std::uintptr_t>(-1);


}

extern "C" {

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

int APS5_VABI sigpause_nid_postfix(int mask) {
    (void)mask;
    NotImplemented_nid_no_patch("sigpause: waiting for a delivered signal");
    return -1;
}

}
