#include <cerrno>
#include <cmath>

#include "prx/libc/include/General.hpp"

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

template<typename F>
auto KeepErrno(F function) {
    volatile int& hostErrno = errno;
    const int saved = hostErrno;
    auto result = function();
    hostErrno = saved;
    return result;
}

template<typename TComplex, typename TFloat>
TComplex Power(TComplex base, TComplex exponent) {
    const TFloat x = __real__ exponent;
    const TFloat y = __imag__ exponent;
    const TFloat magnitude = std::hypot(__real__ base, __imag__ base);
    if (magnitude == 0) return Make<TComplex, TFloat>(0, 0);
    const TFloat argument = std::atan2(__imag__ base, __real__ base);
    TFloat r = std::pow(magnitude, x);
    TFloat theta = x * argument;
    if (y != 0) {
        r = r * std::exp(-y * argument);
        theta = theta + y * std::log(magnitude);
    }
    return Make<TComplex, TFloat>(r * std::cos(theta), r * std::sin(theta));
}

template<typename TComplex, typename TFloat>
TComplex DecimalLogarithm(TComplex z, TComplex naturalLogarithm) {
    const TFloat scale = static_cast<TFloat>(0.434294481903251827651128918916605082L);
    const TFloat re = __real__ z;
    const TFloat im = __imag__ z;
    if (std::isfinite(re) && std::isfinite(im)) {
        const TFloat magnitude = std::hypot(re, im);
        if (std::isfinite(magnitude) && magnitude != 0) return Make<TComplex, TFloat>(std::log10(magnitude), std::atan2(im, re) * scale);
    }
    return Make<TComplex, TFloat>(__real__ naturalLogarithm * scale, __imag__ naturalLogarithm * scale);
}

template<typename TComplex, typename TFloat>
TComplex Project(TComplex z) {
    if (!std::isinf(__real__ z) && !std::isinf(__imag__ z)) return z;
    return Make<TComplex, TFloat>(TFloat(INFINITY), std::copysign(TFloat(0), TFloat(__imag__ z)));
}

}

#define APS5_COMPLEX_FUNCTIONS(SUFFIX, TFloat, TComplex)                                                         \
    TFloat APS5_VABI cabs##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return std::hypot(__real__ z, __imag__ z); }); } \
    TFloat APS5_VABI carg##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return std::atan2(__imag__ z, __real__ z); }); } \
    TFloat APS5_VABI creal##SUFFIX##_nid_postfix(TComplex z) { return __real__ z; }                              \
    TFloat APS5_VABI cimag##SUFFIX##_nid_postfix(TComplex z) { return __imag__ z; }                              \
    TComplex APS5_VABI conj##SUFFIX##_nid_postfix(TComplex z) { return ~z; }                                     \
    TComplex APS5_VABI cproj##SUFFIX##_nid_postfix(TComplex z) { return Project<TComplex, TFloat>(z); }          \
    TComplex APS5_VABI cacos##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_cacos##SUFFIX(z); }); } \
    TComplex APS5_VABI casin##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_casin##SUFFIX(z); }); } \
    TComplex APS5_VABI catan##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_catan##SUFFIX(z); }); } \
    TComplex APS5_VABI cacosh##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_cacosh##SUFFIX(z); }); } \
    TComplex APS5_VABI casinh##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_casinh##SUFFIX(z); }); } \
    TComplex APS5_VABI catanh##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_catanh##SUFFIX(z); }); } \
    TComplex APS5_VABI ccos##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_ccos##SUFFIX(z); }); } \
    TComplex APS5_VABI csin##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_csin##SUFFIX(z); }); } \
    TComplex APS5_VABI ctan##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_ctan##SUFFIX(z); }); } \
    TComplex APS5_VABI ccosh##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_ccosh##SUFFIX(z); }); } \
    TComplex APS5_VABI csinh##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_csinh##SUFFIX(z); }); } \
    TComplex APS5_VABI ctanh##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_ctanh##SUFFIX(z); }); } \
    TComplex APS5_VABI cexp##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_cexp##SUFFIX(z); }); } \
    TComplex APS5_VABI clog##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_clog##SUFFIX(z); }); } \
    TComplex APS5_VABI csqrt##SUFFIX##_nid_postfix(TComplex z) { return KeepErrno([&] { return __builtin_csqrt##SUFFIX(z); }); } \
    TComplex APS5_VABI clog10##SUFFIX##_nid_postfix(TComplex z) {                                                \
        return KeepErrno([&] { return DecimalLogarithm<TComplex, TFloat>(z, __builtin_clog##SUFFIX(z)); });      \
    }                                                                                                            \
    TComplex APS5_VABI cpow##SUFFIX##_nid_postfix(TComplex base, TComplex exponent) {                            \
        return KeepErrno([&] { return Power<TComplex, TFloat>(base, exponent); });                               \
    }

extern "C" {

APS5_COMPLEX_FUNCTIONS(f, float, ComplexFloat)
APS5_COMPLEX_FUNCTIONS(, double, ComplexDouble)
APS5_COMPLEX_FUNCTIONS(l, long double, ComplexLongDouble)

}

#undef APS5_COMPLEX_FUNCTIONS
