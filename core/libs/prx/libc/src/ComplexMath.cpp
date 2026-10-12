#include <cmath>

#include "prx/libc/include/General.hpp"

// GCC complex types keep the System V classes of C99 _Complex values: float pairs packed in
// one SSE register, double pairs in two, and long double pairs on the x87 stack.
using ComplexFloat = __complex__ float;
using ComplexDouble = __complex__ double;
using ComplexLongDouble = __complex__ long double;

namespace {

template<typename TComplex, typename TFloat>
TComplex Make(TFloat real, TFloat imaginary) {
    TComplex result;
    __real__ result = real;
    __imag__ result = imaginary;
    return result;
}

template<typename TComplex, typename TFloat>
TComplex Project(TComplex z) {
    if (!std::isinf(__real__ z) && !std::isinf(__imag__ z)) return z;
    return Make<TComplex, TFloat>(TFloat(INFINITY), std::copysign(TFloat(0), TFloat(__imag__ z)));
}

}

#define APS5_COMPLEX_FUNCTIONS(SUFFIX, TFloat, TComplex)                                                         \
    TFloat APS5_VABI cabs##SUFFIX##_nid_postfix(TComplex z) { return std::hypot(__real__ z, __imag__ z); }       \
    TFloat APS5_VABI carg##SUFFIX##_nid_postfix(TComplex z) { return std::atan2(__imag__ z, __real__ z); }       \
    TFloat APS5_VABI creal##SUFFIX##_nid_postfix(TComplex z) { return __real__ z; }                              \
    TFloat APS5_VABI cimag##SUFFIX##_nid_postfix(TComplex z) { return __imag__ z; }                              \
    TComplex APS5_VABI conj##SUFFIX##_nid_postfix(TComplex z) { return ~z; }                                     \
    TComplex APS5_VABI cproj##SUFFIX##_nid_postfix(TComplex z) { return Project<TComplex, TFloat>(z); }          \
    TComplex APS5_VABI cacos##SUFFIX##_nid_postfix(TComplex z) { return __builtin_cacos##SUFFIX(z); }            \
    TComplex APS5_VABI casin##SUFFIX##_nid_postfix(TComplex z) { return __builtin_casin##SUFFIX(z); }            \
    TComplex APS5_VABI catan##SUFFIX##_nid_postfix(TComplex z) { return __builtin_catan##SUFFIX(z); }            \
    TComplex APS5_VABI cacosh##SUFFIX##_nid_postfix(TComplex z) { return __builtin_cacosh##SUFFIX(z); }          \
    TComplex APS5_VABI casinh##SUFFIX##_nid_postfix(TComplex z) { return __builtin_casinh##SUFFIX(z); }          \
    TComplex APS5_VABI catanh##SUFFIX##_nid_postfix(TComplex z) { return __builtin_catanh##SUFFIX(z); }          \
    TComplex APS5_VABI ccos##SUFFIX##_nid_postfix(TComplex z) { return __builtin_ccos##SUFFIX(z); }              \
    TComplex APS5_VABI csin##SUFFIX##_nid_postfix(TComplex z) { return __builtin_csin##SUFFIX(z); }              \
    TComplex APS5_VABI ctan##SUFFIX##_nid_postfix(TComplex z) { return __builtin_ctan##SUFFIX(z); }              \
    TComplex APS5_VABI ccosh##SUFFIX##_nid_postfix(TComplex z) { return __builtin_ccosh##SUFFIX(z); }            \
    TComplex APS5_VABI csinh##SUFFIX##_nid_postfix(TComplex z) { return __builtin_csinh##SUFFIX(z); }            \
    TComplex APS5_VABI ctanh##SUFFIX##_nid_postfix(TComplex z) { return __builtin_ctanh##SUFFIX(z); }            \
    TComplex APS5_VABI cexp##SUFFIX##_nid_postfix(TComplex z) { return __builtin_cexp##SUFFIX(z); }              \
    TComplex APS5_VABI clog##SUFFIX##_nid_postfix(TComplex z) { return __builtin_clog##SUFFIX(z); }              \
    TComplex APS5_VABI csqrt##SUFFIX##_nid_postfix(TComplex z) { return __builtin_csqrt##SUFFIX(z); }            \
    TComplex APS5_VABI clog10##SUFFIX##_nid_postfix(TComplex z) {                                                \
        const TComplex logarithm = __builtin_clog##SUFFIX(z);                                                    \
        const TFloat scale = static_cast<TFloat>(1) / std::log(static_cast<TFloat>(10));                                                   \
        return Make<TComplex, TFloat>(__real__ logarithm * scale, __imag__ logarithm * scale);                  \
    }                                                                                                            \
    TComplex APS5_VABI cpow##SUFFIX##_nid_postfix(TComplex base, TComplex exponent) {                            \
        return __builtin_cexp##SUFFIX(exponent * __builtin_clog##SUFFIX(base));                                  \
    }

extern "C" {

APS5_COMPLEX_FUNCTIONS(f, float, ComplexFloat)
APS5_COMPLEX_FUNCTIONS(, double, ComplexDouble)
APS5_COMPLEX_FUNCTIONS(l, long double, ComplexLongDouble)

}

#undef APS5_COMPLEX_FUNCTIONS
