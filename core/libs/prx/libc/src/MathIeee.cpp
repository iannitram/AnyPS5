#include <algorithm>
#include <cerrno>
#include <cfenv>
#include <climits>
#include <cmath>
#include <cstdint>
#include <immintrin.h>
#include <stdexcept>

#include "prx/libc/include/General.hpp"

namespace {

template<typename F>
auto KeepErrno(F function) {
    volatile int& hostErrno = errno;
    const int saved = hostErrno;
    auto result = function();
    hostErrno = saved;
    return result;
}

template<typename T>
T Maximum(T x, T y) {
    if (std::isnan(x)) return y;
    if (std::isnan(y)) return x;
    if (std::signbit(x) != std::signbit(y)) return std::signbit(x) ? y : x;
    return x > y ? x : y;
}

template<typename T>
T Minimum(T x, T y) {
    if (std::isnan(x)) return y;
    if (std::isnan(y)) return x;
    if (std::signbit(x) != std::signbit(y)) return std::signbit(x) ? x : y;
    return x < y ? x : y;
}

template<typename T>
T PositiveDifference(T x, T y) {
    if (std::isnan(x)) return x;
    if (std::isnan(y)) return y;
    return x > y ? x - y : T(0);
}

template<typename T>
int ExponentOf(T x) {
    if (x == 0) return -INT_MAX;
    if (std::isnan(x) || std::isinf(x)) return INT_MAX;
    return KeepErrno([x] { return std::ilogb(x); });
}

std::int64_t ConvertX87(long double x) {
    std::int64_t result;
    asm volatile("fistpll %0" : "=m"(result) : "t"(x) : "st");
    return result;
}

std::int64_t RoundToLong(long double x) {
    const long double rounded = std::round(x);
    if (rounded >= -0x1p63L && rounded < 0x1p63L) return static_cast<std::int64_t>(rounded);
    std::feraiseexcept(FE_INVALID);
    return INT64_MIN;
}

long double PartialRemainder(long double x, long double y, int* quotient) {
    if (quotient == nullptr) throw std::invalid_argument("remquo: null quotient");
    long double result = x;
    std::uint16_t status;
    do {
        asm volatile("fprem1\n\tfnstsw %%ax" : "+t"(result), "=a"(status) : "u"(y));
    } while (status & 0x400);
    unsigned bits = status >> 6;
    unsigned value = bits;
    bits &= 0x108;
    bits = (bits >> 7) | (bits << 25);
    value |= bits;
    bits = (bits << 4) | (bits >> 28);
    value = (bits | value) & 7;
    const bool negative = std::signbit(x) != std::signbit(y);
    *quotient = negative ? -static_cast<int>(value) : static_cast<int>(value);
    return result;
}

int ScaleExponent(std::int64_t exponent) {
    return static_cast<int>(std::clamp<std::int64_t>(exponent, -100000, 100000));
}

}

extern "C" {

double APS5_VABI ceil_nid_postfix(double x) { return std::ceil(x); }
float APS5_VABI ceilf_nid_postfix(float x) { return std::ceil(x); }
long double APS5_VABI ceill_nid_postfix(long double x) { return std::ceil(x); }
double APS5_VABI floor_nid_postfix(double x) { return std::floor(x); }
float APS5_VABI floorf_nid_postfix(float x) { return std::floor(x); }
long double APS5_VABI floorl_nid_postfix(long double x) { return std::floor(x); }
float APS5_VABI truncf_nid_postfix(float x) { return std::trunc(x); }
long double APS5_VABI roundl_nid_postfix(long double x) { return std::round(x); }
double APS5_VABI nearbyint_nid_postfix(double x) { return std::nearbyint(x); }
float APS5_VABI nearbyintf_nid_postfix(float x) { return std::nearbyint(x); }
long double APS5_VABI nearbyintl_nid_postfix(long double x) { return std::nearbyint(x); }
double APS5_VABI rint_nid_postfix(double x) { return std::rint(x); }
float APS5_VABI rintf_nid_postfix(float x) { return std::rint(x); }
long double APS5_VABI rintl_nid_postfix(long double x) { return std::rint(x); }

std::int64_t APS5_VABI lrint_nid_postfix(double x) { return _mm_cvtsd_si64(_mm_set_sd(x)); }
std::int64_t APS5_VABI lrintf_nid_postfix(float x) { return _mm_cvtss_si64(_mm_set_ss(x)); }
std::int64_t APS5_VABI lrintl_nid_postfix(long double x) { return ConvertX87(x); }
long long APS5_VABI llrint_nid_postfix(double x) { return lrint_nid_postfix(x); }
long long APS5_VABI llrintf_nid_postfix(float x) { return lrintf_nid_postfix(x); }
long long APS5_VABI llrintl_nid_postfix(long double x) { return lrintl_nid_postfix(x); }
std::int64_t APS5_VABI lroundl_nid_postfix(long double x) { return RoundToLong(x); }
long long APS5_VABI llroundl_nid_postfix(long double x) { return RoundToLong(x); }

float APS5_VABI fabsf_nid_postfix(float x) { return std::fabs(x); }
long double APS5_VABI fabsl_nid_postfix(long double x) { return std::fabs(x); }
double APS5_VABI copysign_nid_postfix(double x, double y) { return std::copysign(x, y); }
float APS5_VABI copysignf_nid_postfix(float x, float y) { return std::copysign(x, y); }
long double APS5_VABI copysignl_nid_postfix(long double x, long double y) { return std::copysign(x, y); }

double APS5_VABI fmax_nid_postfix(double x, double y) { return Maximum(x, y); }
float APS5_VABI fmaxf_nid_postfix(float x, float y) { return Maximum(x, y); }
long double APS5_VABI fmaxl_nid_postfix(long double x, long double y) { return Maximum(x, y); }
double APS5_VABI fmin_nid_postfix(double x, double y) { return Minimum(x, y); }
float APS5_VABI fminf_nid_postfix(float x, float y) { return Minimum(x, y); }
long double APS5_VABI fminl_nid_postfix(long double x, long double y) { return Minimum(x, y); }
double APS5_VABI fdim_nid_postfix(double x, double y) { return PositiveDifference(x, y); }
float APS5_VABI fdimf_nid_postfix(float x, float y) { return PositiveDifference(x, y); }
long double APS5_VABI fdiml_nid_postfix(long double x, long double y) { return PositiveDifference(x, y); }

double APS5_VABI nextafter_nid_postfix(double x, double y) { return KeepErrno([=] { return std::nextafter(x, y); }); }
long double APS5_VABI nextafterl_nid_postfix(long double x, long double y) { return KeepErrno([=] { return std::nextafter(x, y); }); }
double APS5_VABI nexttoward_nid_postfix(double x, long double y) { return KeepErrno([=] { return std::nexttoward(x, y); }); }
float APS5_VABI nexttowardf_nid_postfix(float x, long double y) { return KeepErrno([=] { return std::nexttoward(x, y); }); }
long double APS5_VABI nexttowardl_nid_postfix(long double x, long double y) { return KeepErrno([=] { return std::nexttoward(x, y); }); }

int APS5_VABI ilogb_nid_postfix(double x) { return ExponentOf(x); }
int APS5_VABI ilogbf_nid_postfix(float x) { return ExponentOf(x); }
int APS5_VABI ilogbl_nid_postfix(long double x) { return ExponentOf(x); }
long double APS5_VABI logbl_nid_postfix(long double x) { return KeepErrno([=] { return std::logb(x); }); }

long double APS5_VABI modfl_nid_postfix(long double x, long double* integral) {
    if (integral == nullptr) throw std::invalid_argument("modfl: null integral part");
    return std::modf(x, integral);
}
long double APS5_VABI fmodl_nid_postfix(long double x, long double y) { return KeepErrno([=] { return std::fmod(x, y); }); }
long double APS5_VABI remainderl_nid_postfix(long double x, long double y) { return KeepErrno([=] { return std::remainder(x, y); }); }
double APS5_VABI remquo_nid_postfix(double x, double y, int* quotient) { return static_cast<double>(PartialRemainder(x, y, quotient)); }
float APS5_VABI remquof_nid_postfix(float x, float y, int* quotient) { return static_cast<float>(PartialRemainder(x, y, quotient)); }
long double APS5_VABI remquol_nid_postfix(long double x, long double y, int* quotient) { return PartialRemainder(x, y, quotient); }

long double APS5_VABI scalbnl_nid_postfix(long double x, int exponent) { return KeepErrno([=] { return std::scalbn(x, exponent); }); }
double APS5_VABI scalbln_nid_postfix(double x, std::int64_t exponent) { return KeepErrno([=] { return std::scalbn(x, ScaleExponent(exponent)); }); }
float APS5_VABI scalblnf_nid_postfix(float x, std::int64_t exponent) { return KeepErrno([=] { return std::scalbn(x, ScaleExponent(exponent)); }); }
long double APS5_VABI scalblnl_nid_postfix(long double x, std::int64_t exponent) { return KeepErrno([=] { return std::scalbn(x, ScaleExponent(exponent)); }); }

float APS5_VABI fmaf_nid_postfix(float x, float y, float z) { return std::fma(x, y, z); }
long double APS5_VABI fmal_nid_postfix(long double x, long double y, long double z) { return KeepErrno([=] { return std::fma(x, y, z); }); }
long double APS5_VABI sqrtl_nid_postfix(long double x) { return KeepErrno([=] { return std::sqrt(x); }); }

}
