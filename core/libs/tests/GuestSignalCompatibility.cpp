#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {
struct SignalSet {
    std::uint32_t bits[4];
};
struct SignalAction {
    std::uintptr_t handler;
    int flags;
    SignalSet mask;
};
struct SignalVector {
    std::uintptr_t handler;
    int mask;
    int flags;
};
using Handler = void (APS5_VABI *)(int);
}

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI sigaction_nid_postfix(int, const SignalAction*, SignalAction*);
int APS5_VABI _sigprocmask_nid_postfix(int, const SignalSet*, SignalSet*);
Handler APS5_VABI signal_nid_postfix(int, Handler);
int APS5_VABI sigvec_nid_postfix(int, const SignalVector*, SignalVector*);
int APS5_VABI sigsetmask_nid_postfix(int);
int APS5_VABI sigblock_nid_postfix(int);
int APS5_VABI sighold_nid_postfix(int);
int APS5_VABI sigrelse_nid_postfix(int);
int APS5_VABI sigignore_nid_postfix(int);
std::uintptr_t APS5_VABI sigset_nid_postfix(int, std::uintptr_t);
int APS5_VABI siginterrupt_nid_postfix(int, int);
int APS5_VABI sigpending_nid_postfix(SignalSet*);
int APS5_VABI raise_nid_postfix(int);
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
    const int original = sigsetmask_nid_postfix(0);
    Require(sigblock_nid_postfix(static_cast<int>(UserBit)) == 0 && Mask() == UserBit, "sigblock adds and returns the previous mask");
    Require(sigblock_nid_postfix(static_cast<int>(OtherBit)) == static_cast<int>(UserBit) && Mask() == (UserBit | OtherBit), "sigblock accumulates");
    Require(sigsetmask_nid_postfix(static_cast<int>(OtherBit)) == static_cast<int>(UserBit | OtherBit) && Mask() == OtherBit, "sigsetmask replaces");
    Require(sigsetmask_nid_postfix(0) == static_cast<int>(OtherBit), "sigsetmask clears");

    Require(sighold_nid_postfix(UserSignal) == 0 && Mask() == UserBit, "sighold");
    Require(sigrelse_nid_postfix(UserSignal) == 0 && Mask() == 0, "sigrelse");
    *__error_nid_postfix() = 0;
    Require(sighold_nid_postfix(0) == -1 && *__error_nid_postfix() == 22, "sighold of an invalid signal");
    Require(sighold_nid_postfix(40) == 0 && FullMask().bits[1] == 0x80u, "sighold of a signal above 32");
    Require(sigrelse_nid_postfix(40) == 0 && FullMask().bits[1] == 0, "sigrelse of a signal above 32");
    Require(sigset_nid_postfix(9, 3) == 0 && sigset_nid_postfix(9, 3) == 0 && (FullMask().bits[0] & (1u << 8)) == 0, "SIGKILL cannot be held");
    Require(sigrelse_nid_postfix(129) == -1 && *__error_nid_postfix() == 22, "sigrelse of an invalid signal");

    Require(sigignore_nid_postfix(UserSignal) == 0 && Action(UserSignal).handler == 1, "sigignore");

    SignalVector vector{0, static_cast<int>(OtherBit), 0};
    SignalVector previous{};
    Require(sigvec_nid_postfix(UserSignal, &vector, &previous) == 0, "sigvec");
    Require(previous.handler == 1 && previous.flags == 0x2, "previous vector flips SA_RESTART into SV_INTERRUPT");
    const SignalAction installed = Action(UserSignal);
    Require(installed.handler == 0 && installed.flags == 0x2 && installed.mask.bits[0] == OtherBit, "vector without SV_INTERRUPT restarts");
    vector.flags = 0x2;
    Require(sigvec_nid_postfix(UserSignal, &vector, nullptr) == 0 && Action(UserSignal).flags == 0, "SV_INTERRUPT clears SA_RESTART");
    Require(sigvec_nid_postfix(UserSignal, nullptr, &previous) == 0 && previous.flags == 0x2 && previous.mask == static_cast<int>(OtherBit), "query");

    Require(sigset_nid_postfix(UserSignal, 1) == 0 && Action(UserSignal).handler == 1, "sigset installs and returns the previous handler");
    Require(sigset_nid_postfix(UserSignal, 3) == 1 && Mask() == UserBit, "sigset hold blocks and returns the handler");
    Require(sigset_nid_postfix(UserSignal, 3) == 3, "holding a held signal returns SIG_HOLD");
    Require(sigset_nid_postfix(UserSignal, 0) == 3 && Mask() == 0 && Action(UserSignal).handler == 0, "sigset releases and returns SIG_HOLD");
    Require(sigset_nid_postfix(0, 1) == static_cast<std::uintptr_t>(-1), "sigset of an invalid signal is SIG_ERR");

    Require(siginterrupt_nid_postfix(UserSignal, 1) == 0 && (Action(UserSignal).flags & 0x2) == 0, "siginterrupt clears SA_RESTART");
    signal_nid_postfix(UserSignal, nullptr);
    Require((Action(UserSignal).flags & 0x2) == 0, "signal keeps an interrupting signal interrupting");
    Require(siginterrupt_nid_postfix(UserSignal, 0) == 0 && (Action(UserSignal).flags & 0x2) != 0, "siginterrupt restores SA_RESTART");
    signal_nid_postfix(UserSignal, nullptr);
    Require((Action(UserSignal).flags & 0x2) != 0, "signal restarts again");
    Require(siginterrupt_nid_postfix(200, 1) == -1 && *__error_nid_postfix() == 22, "siginterrupt of an invalid signal");

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
    signal_nid_postfix(Terminate, nullptr);

    sigsetmask_nid_postfix(original);
    std::puts("guest signal compatibility tests passed");
    return 0;
}
