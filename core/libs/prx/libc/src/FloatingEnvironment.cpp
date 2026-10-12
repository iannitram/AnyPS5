#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "prx/libc/include/General.hpp"

namespace {

struct GuestFloatingEnvironment {
    struct {
        std::uint32_t control;
        std::uint32_t status;
        std::uint32_t tag;
        char other[16];
    } x87;
    std::uint32_t mxcsr;
};

static_assert(sizeof(GuestFloatingEnvironment) == 32);
static_assert(offsetof(GuestFloatingEnvironment, mxcsr) == 28);

using GuestExceptionFlags = std::uint16_t;

constexpr int AllExceptions = 0x3f;
constexpr int RoundMask = 0x0c00;
constexpr int SseRoundShift = 3;
constexpr int SseMaskShift = 7;

void StoreEnvironment(GuestFloatingEnvironment* environment) {
    asm volatile("fnstenv %0" : "=m"(environment->x87));
}

void LoadEnvironment(const GuestFloatingEnvironment* environment) {
    asm volatile("fldenv %0" : : "m"(environment->x87) : "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)");
}

void ClearX87Exceptions() { asm volatile("fnclex"); }

std::uint16_t X87Status() {
    std::uint16_t status;
    asm volatile("fnstsw %0" : "=am"(status));
    return status;
}

std::uint16_t X87Control() {
    std::uint16_t control;
    asm volatile("fnstcw %0" : "=m"(control));
    return control;
}

void SetX87Control(std::uint16_t control) { asm volatile("fldcw %0" : : "m"(control)); }

std::uint32_t Mxcsr() {
    std::uint32_t mxcsr;
    asm volatile("stmxcsr %0" : "=m"(mxcsr));
    return mxcsr;
}

void SetMxcsr(std::uint32_t mxcsr) { asm volatile("ldmxcsr %0" : : "m"(mxcsr)); }

void Wait() { asm volatile("fwait"); }

template<typename T>
T* Require(T* pointer, const char* function) {
    if (pointer == nullptr) throw std::invalid_argument(std::string(function) + ": null pointer");
    return pointer;
}

}

extern "C" {

extern const GuestFloatingEnvironment __fe_dfl_env_nid_postfix;
const GuestFloatingEnvironment __fe_dfl_env_nid_postfix {
    {0xffff0000u | 0x037f, 0xffff0000u, 0xffffffffu, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\xff', '\xff'}},
    0x1f80,
};

int APS5_VABI feclearexcept_nid_postfix(int excepts) {
    GuestFloatingEnvironment environment;
    if (excepts == AllExceptions) {
        ClearX87Exceptions();
    } else {
        StoreEnvironment(&environment);
        environment.x87.status &= ~excepts;
        LoadEnvironment(&environment);
    }
    SetMxcsr(Mxcsr() & ~excepts);
    return 0;
}

int APS5_VABI fegetexceptflag_nid_postfix(GuestExceptionFlags* flags, int excepts) {
    const std::uint32_t mxcsr = Mxcsr();
    const std::uint16_t status = X87Status();
    *Require(flags, __func__) = static_cast<GuestExceptionFlags>((mxcsr | status) & excepts);
    return 0;
}

int APS5_VABI fesetexceptflag_nid_postfix(const GuestExceptionFlags* flags, int excepts) {
    const GuestExceptionFlags value = *Require(flags, __func__);
    GuestFloatingEnvironment environment;
    StoreEnvironment(&environment);
    environment.x87.status &= ~excepts;
    environment.x87.status |= value & excepts;
    LoadEnvironment(&environment);
    std::uint32_t mxcsr = Mxcsr();
    mxcsr &= ~excepts;
    mxcsr |= value & excepts;
    SetMxcsr(mxcsr);
    return 0;
}

int APS5_VABI feraiseexcept_nid_postfix(int excepts) {
    const GuestExceptionFlags flags = static_cast<GuestExceptionFlags>(excepts);
    fesetexceptflag_nid_postfix(&flags, excepts);
    Wait();
    return 0;
}

int APS5_VABI fetestexcept_nid_postfix(int excepts) {
    const std::uint32_t mxcsr = Mxcsr();
    const std::uint16_t status = X87Status();
    return static_cast<int>((status | mxcsr) & excepts);
}

int APS5_VABI fegetround_nid_postfix() {
    return X87Control() & RoundMask;
}

int APS5_VABI fesetround_nid_postfix(int round) {
    if (round & ~RoundMask) return -1;
    std::uint16_t control = X87Control();
    control &= ~RoundMask;
    control |= round;
    SetX87Control(control);
    std::uint32_t mxcsr = Mxcsr();
    mxcsr &= ~(RoundMask << SseRoundShift);
    mxcsr |= round << SseRoundShift;
    SetMxcsr(mxcsr);
    return 0;
}

int APS5_VABI fegetenv_nid_postfix(GuestFloatingEnvironment* environment) {
    Require(environment, __func__);
    StoreEnvironment(environment);
    environment->mxcsr = Mxcsr();
    SetX87Control(static_cast<std::uint16_t>(environment->x87.control));
    return 0;
}

int APS5_VABI feholdexcept_nid_postfix(GuestFloatingEnvironment* environment) {
    Require(environment, __func__);
    std::uint32_t mxcsr = Mxcsr();
    StoreEnvironment(environment);
    ClearX87Exceptions();
    environment->mxcsr = mxcsr;
    mxcsr &= ~AllExceptions;
    mxcsr |= AllExceptions << SseMaskShift;
    SetMxcsr(mxcsr);
    return 0;
}

int APS5_VABI fesetenv_nid_postfix(const GuestFloatingEnvironment* environment) {
    Require(environment, __func__);
    LoadEnvironment(environment);
    SetMxcsr(environment->mxcsr);
    return 0;
}

int APS5_VABI feupdateenv_nid_postfix(const GuestFloatingEnvironment* environment) {
    Require(environment, __func__);
    const std::uint16_t status = X87Status();
    const std::uint32_t mxcsr = Mxcsr();
    fesetenv_nid_postfix(environment);
    feraiseexcept_nid_postfix(static_cast<int>((mxcsr | status) & AllExceptions));
    return 0;
}

int APS5_VABI feenableexcept_nid_postfix(int mask) {
    mask &= AllExceptions;
    std::uint16_t control = X87Control();
    std::uint32_t mxcsr = Mxcsr();
    const int previous = static_cast<int>(~(control | mxcsr >> SseMaskShift) & AllExceptions);
    control &= ~mask;
    SetX87Control(control);
    mxcsr &= ~(mask << SseMaskShift);
    SetMxcsr(mxcsr);
    return previous;
}

int APS5_VABI fedisableexcept_nid_postfix(int mask) {
    mask &= AllExceptions;
    std::uint16_t control = X87Control();
    std::uint32_t mxcsr = Mxcsr();
    const int previous = static_cast<int>(~(control | mxcsr >> SseMaskShift) & AllExceptions);
    control |= mask;
    SetX87Control(control);
    mxcsr |= mask << SseMaskShift;
    SetMxcsr(mxcsr);
    return previous;
}

int APS5_VABI fegetexcept_nid_postfix() {
    return ~X87Control() & AllExceptions;
}

}
