#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include "prx/libc/include/Api.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libkernel_sys/include/Api.hpp"


namespace {
using SignalSet = GuestSignalSet;
using SignalAction = GuestSigaction;
using Handler = void (APS5_VABI *)(int);
}


namespace {

constexpr int UserSignal = 30;
constexpr int OtherSignal = 31;
constexpr std::uint32_t UserBit = 1u << (UserSignal - 1);
constexpr std::uint32_t OtherBit = 1u << (OtherSignal - 1);

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

std::uint32_t Mask() {
    SignalSet set{};
    _sigprocmask_nid_postfix(1, nullptr, &set);
    return set.bits[0];
}

volatile int caught = 0;
void APS5_VABI Catch(int value) { caught = value; }

SignalSet Pending() {
    SignalSet set{};
    sigpending_nid_postfix(&set);
    return set;
}

SignalSet FullMask() {
    SignalSet set{};
    _sigprocmask_nid_postfix(1, nullptr, &set);
    return set;
}

SignalAction Action(int signal) {
    SignalAction action{};
    sigaction_nid_postfix(signal, nullptr, &action);
    return action;
}

}

int main() {
    Require(sighold_nid_postfix(UserSignal) == 0 && Mask() == UserBit, "sighold");
    Require(sigrelse_nid_postfix(UserSignal) == 0 && Mask() == 0, "sigrelse");
    *__error_nid_postfix() = 0;
    Require(sighold_nid_postfix(0) == -1 && *__error_nid_postfix() == 22, "sighold of an invalid signal");
    Require(sighold_nid_postfix(40) == 0 && FullMask().bits[1] == 0x80u, "sighold of a signal above 32");
    Require(sigrelse_nid_postfix(40) == 0 && FullMask().bits[1] == 0, "sigrelse of a signal above 32");
    Require(sigset_nid_postfix(9, 3) == 0 && sigset_nid_postfix(9, 3) == 0 && (FullMask().bits[0] & (1u << 8)) == 0, "SIGKILL cannot be held");
    Require(sigrelse_nid_postfix(129) == -1 && *__error_nid_postfix() == 22, "sigrelse of an invalid signal");

    Require(sigignore_nid_postfix(UserSignal) == 0 && Action(UserSignal).handler == 1, "sigignore");

    Require(sigset_nid_postfix(UserSignal, 0) == 1 && Action(UserSignal).handler == 0, "sigset resets the handler");

    Require(sigset_nid_postfix(UserSignal, 1) == 0 && Action(UserSignal).handler == 1, "sigset installs and returns the previous handler");
    Require(sigset_nid_postfix(UserSignal, 3) == 1 && Mask() == UserBit, "sigset hold blocks and returns the handler");
    Require(sigset_nid_postfix(UserSignal, 3) == 3, "holding a held signal returns SIG_HOLD");
    Require(sigset_nid_postfix(UserSignal, 0) == 3 && Mask() == 0 && Action(UserSignal).handler == 0, "sigset releases and returns SIG_HOLD");
    Require(sigset_nid_postfix(0, 1) == static_cast<std::uintptr_t>(-1), "sigset of an invalid signal is SIG_ERR");

    SignalSet pending{{1, 2, 3, 4}};
    Require(sigpending_nid_postfix(&pending) == 0 && pending.bits[0] == 0 && pending.bits[3] == 0, "nothing is pending");
    Require(sigpending_nid_postfix(nullptr) == -1 && *__error_nid_postfix() == 14, "null set");

    constexpr int Terminate = 15;
    const SignalSet terminate{{1u << (Terminate - 1), 0, 0, 0}};
    signal_nid_postfix(Terminate, Catch);
    caught = 0;
    Require(_sigprocmask_nid_postfix(1, &terminate, nullptr) == 0 && raise_nid_postfix(Terminate) == 0 && caught == 0, "a blocked signal is not delivered");
    Require(Pending().bits[0] == terminate.bits[0], "a blocked signal stays pending");
    Require(_sigprocmask_nid_postfix(2, &terminate, nullptr) == 0 && caught == Terminate, "unblocking delivers the pending signal");
    Require(Pending().bits[0] == 0, "a delivered signal is no longer pending");
    caught = 0;
    Require(_sigprocmask_nid_postfix(1, &terminate, nullptr) == 0 && raise_nid_postfix(Terminate) == 0, "raise while blocked");
    signal_nid_postfix(Terminate, reinterpret_cast<Handler>(std::uintptr_t{1}));
    Require(Pending().bits[0] == 0, "ignoring a signal discards it");
    Require(_sigprocmask_nid_postfix(2, &terminate, nullptr) == 0 && caught == 0, "a discarded signal is not delivered");
    signal_nid_postfix(Terminate, Catch);
    Require(_sigprocmask_nid_postfix(1, &terminate, nullptr) == 0 && raise_nid_postfix(Terminate) == 0 && caught == 0, "raise while blocked again");
    const SignalAction information{reinterpret_cast<std::uintptr_t>(Catch), 0x40, {}};
    Require(sigaction_nid_postfix(Terminate, &information, nullptr) == 0, "switch to an SA_SIGINFO handler");
    bool threwPending = false;
    try { _sigprocmask_nid_postfix(2, &terminate, nullptr); } catch (const std::exception&) { threwPending = true; }
    Require(threwPending && caught == 0, "a pending signal for an SA_SIGINFO handler throws as raise does");
    signal_nid_postfix(Terminate, nullptr);

    const int self = getpid_nid_postfix();
    Require(sigqueue_nid_postfix(self, 0, nullptr) == 0, "sigqueue checks the process");
    *__error_nid_postfix() = 0;
    Require(sigqueue_nid_postfix(self + 1, 0, nullptr) == -1 && *__error_nid_postfix() == 3, "sigqueue to another process");
    Require(sigqueue_nid_postfix(0, 0, nullptr) == -1 && *__error_nid_postfix() == 22, "sigqueue needs a positive pid");
    Require(sigqueue_nid_postfix(self, 129, nullptr) == -1 && *__error_nid_postfix() == 22, "sigqueue of an invalid signal");
    Require(killpg_nid_postfix(self, 0) == 0 && killpg_nid_postfix(0, 0) == 0, "killpg of the own group");
    Require(killpg_nid_postfix(1, 0) == -1 && *__error_nid_postfix() == 3, "killpg of group 1");
    int thrown = 0;
    try { sigpause_nid_postfix(0); } catch (const std::exception&) { ++thrown; }
    Require(thrown == 1, "sigpause throws");

    std::puts("guest signal compatibility tests passed");
    return 0;
}
