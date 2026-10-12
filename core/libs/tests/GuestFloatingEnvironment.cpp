#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

struct Environment {
    std::uint32_t control;
    std::uint32_t status;
    std::uint32_t tag;
    char other[16];
    std::uint32_t mxcsr;
};

}

extern "C" {
extern const Environment __fe_dfl_env_nid_postfix;
int APS5_VABI feclearexcept_nid_postfix(int);
int APS5_VABI fegetexceptflag_nid_postfix(std::uint16_t*, int);
int APS5_VABI fesetexceptflag_nid_postfix(const std::uint16_t*, int);
int APS5_VABI feraiseexcept_nid_postfix(int);
int APS5_VABI fetestexcept_nid_postfix(int);
int APS5_VABI fegetround_nid_postfix();
int APS5_VABI fesetround_nid_postfix(int);
int APS5_VABI fegetenv_nid_postfix(Environment*);
int APS5_VABI feholdexcept_nid_postfix(Environment*);
int APS5_VABI fesetenv_nid_postfix(const Environment*);
int APS5_VABI feupdateenv_nid_postfix(const Environment*);
int APS5_VABI feenableexcept_nid_postfix(int);
int APS5_VABI fedisableexcept_nid_postfix(int);
int APS5_VABI fegetexcept_nid_postfix();
}

namespace {

constexpr int Invalid = 0x01;
constexpr int DivideByZero = 0x04;
constexpr int Overflow = 0x08;
constexpr int Underflow = 0x10;
constexpr int Inexact = 0x20;
constexpr int AllExceptions = 0x3f;
constexpr int ToNearest = 0x000;
constexpr int Downward = 0x400;
constexpr int Upward = 0x800;
constexpr int TowardZero = 0xc00;

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

std::uint16_t Control() {
    std::uint16_t control;
    asm volatile("fnstcw %0" : "=m"(control));
    return control;
}

std::uint32_t Mxcsr() {
    std::uint32_t mxcsr;
    asm volatile("stmxcsr %0" : "=m"(mxcsr));
    return mxcsr;
}

void Reset() {
    Require(fesetenv_nid_postfix(&__fe_dfl_env_nid_postfix) == 0, "reset");
}

template<typename F>
void RequireThrows(F function, const char* what) {
    try { function(); } catch (const std::invalid_argument&) { return; }
    Require(false, what);
}

void CheckDefault() {
    const auto& environment = __fe_dfl_env_nid_postfix;
    Require(environment.control == 0xffff037f, "default control word");
    Require(environment.status == 0xffff0000, "default status word");
    Require(environment.tag == 0xffffffff, "default tag word");
    Require(static_cast<unsigned char>(environment.other[14]) == 0xff && static_cast<unsigned char>(environment.other[15]) == 0xff, "default operand selector");
    Require(environment.mxcsr == 0x1f80, "default mxcsr");
    Reset();
    Require((Control() & 0x0f3f) == 0x033f && (Mxcsr() & 0xffbf) == 0x1f80, "default loaded");
    Require(fegetround_nid_postfix() == ToNearest && fegetexcept_nid_postfix() == 0 && fetestexcept_nid_postfix(AllExceptions) == 0, "default state");
}

void CheckRounding() {
    for (int mode : {Downward, Upward, TowardZero, ToNearest}) {
        Require(fesetround_nid_postfix(mode) == 0, "set rounding");
        Require(fegetround_nid_postfix() == mode, "get rounding");
        Require((Control() & 0x0c00) == mode, "x87 rounding");
        Require((Mxcsr() & 0x6000) == static_cast<std::uint32_t>(mode << 3), "sse rounding");
    }
    volatile float one = 1.0f, three = 3.0f;
    fesetround_nid_postfix(Upward);
    const float up = one / three;
    fesetround_nid_postfix(Downward);
    const float down = one / three;
    Require(up > down, "sse arithmetic follows rounding");
    Require(fesetround_nid_postfix(0x401) == -1 && fesetround_nid_postfix(0x1000) == -1, "invalid rounding");
    Require(fegetround_nid_postfix() == Downward, "invalid rounding keeps mode");
    Reset();
}

void CheckFlags() {
    Require(feraiseexcept_nid_postfix(Inexact | Overflow) == 0, "raise");
    Require(fetestexcept_nid_postfix(AllExceptions) == (Inexact | Overflow), "raised flags");
    Require((Mxcsr() & AllExceptions) == (Inexact | Overflow), "raised in mxcsr");
    Require(feclearexcept_nid_postfix(Overflow) == 0 && fetestexcept_nid_postfix(AllExceptions) == Inexact, "clear one");
    std::uint16_t saved = 0;
    Require(fegetexceptflag_nid_postfix(&saved, Inexact | Invalid) == 0 && saved == Inexact, "get flags");
    Require(feclearexcept_nid_postfix(AllExceptions) == 0 && fetestexcept_nid_postfix(AllExceptions) == 0, "clear all");
    std::uint16_t set = Invalid | Underflow;
    Require(fesetexceptflag_nid_postfix(&set, Underflow | DivideByZero) == 0, "set flags");
    Require(fetestexcept_nid_postfix(AllExceptions) == Underflow, "set flags masked by excepts");
    Require(fesetexceptflag_nid_postfix(&saved, AllExceptions) == 0 && fetestexcept_nid_postfix(AllExceptions) == Inexact, "restore flags");
    feclearexcept_nid_postfix(AllExceptions);
    volatile double zero = 0.0, value = 1.0;
    volatile double result = value / zero;
    (void)result;
    Require(fetestexcept_nid_postfix(DivideByZero) == DivideByZero, "arithmetic sets flag");
    Reset();
}

void CheckEnvironment() {
    fesetround_nid_postfix(TowardZero);
    feraiseexcept_nid_postfix(Inexact);
    Environment saved {};
    const std::uint16_t controlBefore = Control();
    Require(fegetenv_nid_postfix(&saved) == 0, "get environment");
    Require(Control() == controlBefore, "get environment keeps control word");
    Require((saved.control & 0x0c00) == TowardZero && (saved.status & Inexact) && (saved.mxcsr & 0x6000) == 0x6000, "saved environment");
    Reset();
    Require(fegetround_nid_postfix() == ToNearest && fetestexcept_nid_postfix(AllExceptions) == 0, "default restored");
    Require(fesetenv_nid_postfix(&saved) == 0, "set environment");
    Require(fegetround_nid_postfix() == TowardZero && fetestexcept_nid_postfix(Inexact) == Inexact, "environment restored");
    Reset();
}

void CheckHold() {
    fesetround_nid_postfix(Upward);
    feraiseexcept_nid_postfix(Overflow);
    feenableexcept_nid_postfix(Invalid);
    Environment held {};
    Require(feholdexcept_nid_postfix(&held) == 0, "hold");
    Require(fetestexcept_nid_postfix(AllExceptions) == 0, "hold clears flags");
    Require((Mxcsr() & (AllExceptions << 7)) == static_cast<std::uint32_t>(AllExceptions << 7), "hold masks sse exceptions");
    Require(fegetround_nid_postfix() == Upward, "hold keeps rounding");
    Require((held.mxcsr & Overflow) && !(held.mxcsr & (Invalid << 7)), "hold saves mxcsr");
    feraiseexcept_nid_postfix(Inexact);
    Require(feupdateenv_nid_postfix(&held) == 0, "update");
    Require(fetestexcept_nid_postfix(AllExceptions) == (Overflow | Inexact), "update merges flags");
    Require(fegetexcept_nid_postfix() == Invalid, "update restores traps");
    fedisableexcept_nid_postfix(AllExceptions);
    Reset();
}

void CheckTraps() {
    Require(fegetexcept_nid_postfix() == 0, "no traps");
    Require(feenableexcept_nid_postfix(DivideByZero | Overflow | 0x100) == 0, "enable returns previous");
    Require(fegetexcept_nid_postfix() == (DivideByZero | Overflow), "enabled traps");
    Require((Control() & (DivideByZero | Overflow)) == 0, "x87 unmasked");
    Require((Mxcsr() & ((DivideByZero | Overflow) << 7)) == 0, "sse unmasked");
    Require(fedisableexcept_nid_postfix(Overflow) == (DivideByZero | Overflow), "disable returns previous");
    Require(fegetexcept_nid_postfix() == DivideByZero, "remaining trap");
    Require(fedisableexcept_nid_postfix(AllExceptions) == DivideByZero && fegetexcept_nid_postfix() == 0, "all disabled");
    Reset();
}

void CheckNull() {
    RequireThrows([] { fegetexceptflag_nid_postfix(nullptr, AllExceptions); }, "null get flags");
    RequireThrows([] { fesetexceptflag_nid_postfix(nullptr, AllExceptions); }, "null set flags");
    RequireThrows([] { fegetenv_nid_postfix(nullptr); }, "null get environment");
    RequireThrows([] { feholdexcept_nid_postfix(nullptr); }, "null hold");
    RequireThrows([] { fesetenv_nid_postfix(nullptr); }, "null set environment");
    RequireThrows([] { feupdateenv_nid_postfix(nullptr); }, "null update");
}

}

int main() {
    CheckDefault();
    CheckRounding();
    CheckFlags();
    CheckEnvironment();
    CheckHold();
    CheckTraps();
    CheckNull();
    Reset();
    std::puts("guest floating environment tests passed");
    return 0;
}
