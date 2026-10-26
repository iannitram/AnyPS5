#include "prx/libc/include/Api.hpp"
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




int APS5_VABI feraiseexcept_nid_postfix(int excepts) {
    const GuestExceptionFlags flags = static_cast<GuestExceptionFlags>(excepts);
    fesetexceptflag_nid_postfix(&flags, excepts);
    Wait();
    return 0;
}






int APS5_VABI fesetenv_nid_postfix(const GuestFloatingEnvironment* environment) {
    Require(environment, __func__);
    LoadEnvironment(environment);
    SetMxcsr(environment->mxcsr);
    return 0;
}





}
