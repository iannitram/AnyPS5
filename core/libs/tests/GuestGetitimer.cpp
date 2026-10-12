#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <thread>

struct GuestItimerval {
    KernelTimeval interval;
    KernelTimeval value;
};

static_assert(sizeof(GuestItimerval) == 32 && offsetof(GuestItimerval, value) == 16);

extern "C" {
int APS5_VABI getitimer_nid_postfix(int, GuestItimerval*);
int APS5_VABI setitimer_nid_postfix(int, const GuestItimerval*, GuestItimerval*);
std::uint32_t APS5_VABI ualarm_nid_postfix(std::uint32_t, std::uint32_t);
unsigned int APS5_VABI GuestAlarm_nid_no_patch(unsigned int);
int* APS5_VABI __error_nid_postfix();
using Handler = void (APS5_VABI *)(int);
Handler APS5_VABI signal_nid_postfix(int, Handler);
}

static void Require(bool value, const char* message) {
    if (!value) {
        std::fprintf(stderr, "%s\n", message);
        std::abort();
    }
}

static void APS5_VABI AlarmHandler(int) {}

static bool Throws(void (*action)()) {
    try { action(); }
    catch (const std::runtime_error&) { return true; }
    return false;
}

static GuestItimerval Read() {
    GuestItimerval timer{};
    Require(getitimer_nid_postfix(0, &timer) == 0, "getitimer failed");
    return timer;
}

static std::int64_t Micros(const KernelTimeval& value) { return value.tv_sec * 1000000 + value.tv_usec; }

static void CheckSetitimer() {
    GuestItimerval request{{0, 500000}, {2, 0}};
    GuestItimerval previous{{7, 7}, {7, 7}};
    Require(setitimer_nid_postfix(0, &request, &previous) == 0, "setitimer");
    Require(Micros(previous.value) == 0 && Micros(previous.interval) == 0, "no previous timer");
    GuestItimerval current = Read();
    Require(Micros(current.value) > 1900000 && Micros(current.value) <= 2000000 && Micros(current.interval) == 500000, "armed with interval");
    GuestItimerval replacement{{0, 0}, {5, 0}};
    Require(setitimer_nid_postfix(0, &replacement, &previous) == 0 && Micros(previous.interval) == 500000 && Micros(previous.value) > 1900000, "previous value and interval");
    Require(Micros(Read().interval) == 0, "one-shot replacement");

    GuestItimerval invalid{{0, 0}, {0, 1000000}};
    Require(setitimer_nid_postfix(0, &invalid, nullptr) == -1 && *__error_nid_postfix() == 22, "microseconds out of range");
    invalid = {{0, -1}, {1, 0}};
    Require(setitimer_nid_postfix(0, &invalid, nullptr) == -1 && *__error_nid_postfix() == 22, "invalid interval while arming");
    invalid = {{0, 0}, {std::numeric_limits<int>::max(), 0}};
    Require(setitimer_nid_postfix(0, &invalid, nullptr) == -1 && *__error_nid_postfix() == 22, "seconds above INT32_MAX / 2");
    Require(Micros(Read().value) > 0, "failed requests keep the timer");

    GuestItimerval disarm{{0, -5}, {0, 0}};
    Require(setitimer_nid_postfix(0, &disarm, &previous) == 0 && Micros(previous.value) > 0, "disarming ignores the interval");
    Require(Micros(Read().value) == 0, "disarmed");
    Require(setitimer_nid_postfix(0, nullptr, &previous) == 0 && Micros(previous.value) == 0, "null request reads the timer");
    Require(setitimer_nid_postfix(5, &request, nullptr) == -1 && *__error_nid_postfix() == 22, "invalid timer");

    GuestItimerval periodic{{0, 100000}, {0, 100000}};
    Require(setitimer_nid_postfix(0, &periodic, nullptr) == 0, "periodic");
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    current = Read();
    Require(Micros(current.value) > 0 && Micros(current.value) <= 100000 && Micros(current.interval) == 100000, "periodic timer reloads after expiring");
    Require(GuestAlarm_nid_no_patch(0) == 1, "alarm reports the periodic timer");
    Require(Micros(Read().value) == 0 && Micros(Read().interval) == 0, "alarm clears the interval");

    Require(ualarm_nid_postfix(1500000, 250000) == 0, "ualarm");
    current = Read();
    Require(Micros(current.value) > 1400000 && Micros(current.interval) == 250000, "ualarm arms with reload");
    const std::uint32_t left = ualarm_nid_postfix(0, 0);
    Require(left > 1400000 && left <= 1500000 && Micros(Read().value) == 0, "ualarm returns the remaining microseconds");

    const auto ignore = reinterpret_cast<Handler>(std::uintptr_t{1});
    Require(ualarm_nid_postfix(5000000, 0) == 0, "arm before installing a handler");
    Require(Throws([] { signal_nid_postfix(14, AlarmHandler); }), "installing a SIGALRM handler while armed is not implemented");
    Require(ualarm_nid_postfix(0, 0) > 0 && signal_nid_postfix(14, AlarmHandler) == ignore, "a handler installs while disarmed");
    Require(Throws([] { ualarm_nid_postfix(5000000, 0); }) && Micros(Read().value) == 0, "arming with a SIGALRM handler is not implemented");
    Require(signal_nid_postfix(14, ignore) == AlarmHandler, "restore the ignored disposition");
}

static std::int64_t ReadRemaining() {
    struct {
        std::uint64_t before;
        GuestItimerval timer;
        std::uint64_t after;
    } guarded;
    std::memset(&guarded, 0xA5, sizeof(guarded));
    *__error_nid_postfix() = 12345;
    Require(getitimer_nid_postfix(0, &guarded.timer) == 0, "getitimer failed");
    Require(*__error_nid_postfix() == 12345, "getitimer changed errno on success");
    Require(guarded.before == 0xA5A5A5A5A5A5A5A5ull && guarded.after == guarded.before, "getitimer wrote outside itimerval");
    Require(guarded.timer.interval.tv_sec == 0 && guarded.timer.interval.tv_usec == 0, "alarm acquired a repeat interval");
    Require(guarded.timer.value.tv_sec >= 0 && guarded.timer.value.tv_usec >= 0
        && guarded.timer.value.tv_usec < 1000000, "invalid timeval normalization");
    return guarded.timer.value.tv_sec * 1000000 + guarded.timer.value.tv_usec;
}

void CheckAlarmRace() {
    const auto ignore = reinterpret_cast<Handler>(std::uintptr_t{1});
    std::atomic<bool> stop{false};
    std::thread arming([&] {
        while (!stop) {
            try { ualarm_nid_postfix(1, 0); } catch (const std::exception&) {}
        }
    });
    std::thread installing([&] {
        while (!stop) {
            try { signal_nid_postfix(14, AlarmHandler); } catch (const std::exception&) {}
            signal_nid_postfix(14, ignore);
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop = true;
    arming.join();
    installing.join();
    ualarm_nid_postfix(0, 0);
    Require(signal_nid_postfix(14, ignore) == ignore, "an alarm racing a handler install never reaches the handler");
}

int main() {
    GuestAlarm_nid_no_patch(0);
    Require(ReadRemaining() == 0, "inactive alarm is not zero");

    for (int invalid : {-1, 3, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
        GuestItimerval output{{11, 22}, {33, 44}};
        const auto saved = output;
        Require(getitimer_nid_postfix(invalid, &output) == -1 && *__error_nid_postfix() == 22, "invalid timer did not report EINVAL");
        Require(std::memcmp(&output, &saved, sizeof(output)) == 0, "invalid timer changed output");
        Require(getitimer_nid_postfix(invalid, nullptr) == -1 && *__error_nid_postfix() == 22, "invalid timer validation order changed");
    }
    Require(getitimer_nid_postfix(0, nullptr) == -1 && *__error_nid_postfix() == 14, "null output did not report EFAULT");
    for (int unsupported : {1, 2}) {
        GuestItimerval output{{11, 22}, {33, 44}};
        const auto saved = output;
        bool threw = false;
        try {
            getitimer_nid_postfix(unsupported, &output);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Require(threw && std::memcmp(&output, &saved, sizeof(output)) == 0, "unsupported CPU timer reported fabricated state");
    }

    const auto ignore = reinterpret_cast<Handler>(std::uintptr_t{1});
    Require(signal_nid_postfix(14, ignore) == nullptr, "SIGALRM was not initially defaulted");
    Require(GuestAlarm_nid_no_patch(30) == 0, "alarm was already active");
    const auto first = ReadRemaining();
    Require(first > 0 && first <= 30000000, "armed alarm is not visible");
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    Require(ReadRemaining() < first, "getitimer does not count down");
    std::array<std::thread, 4> readers;
    for (auto& reader : readers) reader = std::thread([] {
        for (int i = 0; i < 100; ++i) Require(ReadRemaining() > 0, "reading consumed the alarm");
    });
    for (auto& reader : readers) reader.join();
    Require(GuestAlarm_nid_no_patch(0) > 0, "getitimer cancelled the alarm");
    Require(ReadRemaining() == 0, "cancelled alarm is not zero");

    Require(GuestAlarm_nid_no_patch(std::numeric_limits<unsigned int>::max()) == std::numeric_limits<unsigned int>::max() && ReadRemaining() == 0,
        "an alarm beyond INT32_MAX / 2 seconds fails as setitimer does");
    Require(GuestAlarm_nid_no_patch(INT32_MAX / 2) == 0 && ReadRemaining() > static_cast<std::int64_t>(INT32_MAX / 2 - 1) * 1000000,
        "the longest alarm setitimer accepts");
    Require(GuestAlarm_nid_no_patch(0) == static_cast<unsigned int>(INT32_MAX / 2), "alarm rounds the remaining time up");
    GuestAlarm_nid_no_patch(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    Require(ReadRemaining() == 0, "expired alarm is not zero");
    GuestAlarm_nid_no_patch(0);
    CheckSetitimer();
    CheckAlarmRace();
}
