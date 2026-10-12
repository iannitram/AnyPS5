#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cfenv>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

extern "C" {
double APS5_VABI ceil_nid_postfix(double);
float APS5_VABI floorf_nid_postfix(float);
long double APS5_VABI ceill_nid_postfix(long double);
long double APS5_VABI roundl_nid_postfix(long double);
double APS5_VABI nearbyint_nid_postfix(double);
float APS5_VABI rintf_nid_postfix(float);
std::int64_t APS5_VABI lrint_nid_postfix(double);
std::int64_t APS5_VABI lrintf_nid_postfix(float);
std::int64_t APS5_VABI lrintl_nid_postfix(long double);
long long APS5_VABI llrint_nid_postfix(double);
std::int64_t APS5_VABI lroundl_nid_postfix(long double);
long long APS5_VABI llroundl_nid_postfix(long double);
float APS5_VABI fabsf_nid_postfix(float);
double APS5_VABI copysign_nid_postfix(double, double);
long double APS5_VABI copysignl_nid_postfix(long double, long double);
double APS5_VABI fmax_nid_postfix(double, double);
float APS5_VABI fminf_nid_postfix(float, float);
long double APS5_VABI fmaxl_nid_postfix(long double, long double);
double APS5_VABI fmin_nid_postfix(double, double);
double APS5_VABI fdim_nid_postfix(double, double);
long double APS5_VABI fdiml_nid_postfix(long double, long double);
double APS5_VABI nextafter_nid_postfix(double, double);
double APS5_VABI nexttoward_nid_postfix(double, long double);
float APS5_VABI nexttowardf_nid_postfix(float, long double);
int APS5_VABI ilogb_nid_postfix(double);
int APS5_VABI ilogbf_nid_postfix(float);
int APS5_VABI ilogbl_nid_postfix(long double);
long double APS5_VABI logbl_nid_postfix(long double);
long double APS5_VABI modfl_nid_postfix(long double, long double*);
long double APS5_VABI fmodl_nid_postfix(long double, long double);
long double APS5_VABI remainderl_nid_postfix(long double, long double);
double APS5_VABI remquo_nid_postfix(double, double, int*);
float APS5_VABI remquof_nid_postfix(float, float, int*);
long double APS5_VABI remquol_nid_postfix(long double, long double, int*);
double APS5_VABI scalbln_nid_postfix(double, std::int64_t);
long double APS5_VABI scalbnl_nid_postfix(long double, int);
float APS5_VABI fmaf_nid_postfix(float, float, float);
long double APS5_VABI fmal_nid_postfix(long double, long double, long double);
long double APS5_VABI sqrtl_nid_postfix(long double);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

bool PositiveZero(double x) { return x == 0 && !std::signbit(x); }
bool NegativeZero(double x) { return x == 0 && std::signbit(x); }

void CheckRounding() {
    Require(ceil_nid_postfix(-0.5) == 0 && std::signbit(ceil_nid_postfix(-0.5)), "ceil keeps the sign of zero");
    Require(floorf_nid_postfix(-1.25f) == -2.0f && ceill_nid_postfix(1.000001L) == 2.0L, "floor and ceil");
    Require(roundl_nid_postfix(2.5L) == 3.0L && roundl_nid_postfix(-2.5L) == -3.0L, "roundl rounds half away from zero");
    std::fesetround(FE_UPWARD);
    Require(nearbyint_nid_postfix(1.1) == 2.0 && rintf_nid_postfix(-1.9f) == -1.0f, "rounding mode followed");
    Require(lrint_nid_postfix(1.1) == 2 && lrintl_nid_postfix(-1.9L) == -1, "lrint follows the mode");
    std::fesetround(FE_TONEAREST);
    std::feclearexcept(FE_ALL_EXCEPT);
    Require(nearbyint_nid_postfix(2.5) == 2.0 && !std::fetestexcept(FE_INEXACT), "nearbyint raises no inexact");
    Require(lrint_nid_postfix(2.5) == 2 && lrintf_nid_postfix(3.5f) == 4 && llrint_nid_postfix(-2.5) == -2, "lrint to even");
    std::feclearexcept(FE_ALL_EXCEPT);
    Require(lrint_nid_postfix(1e30) == INT64_MIN && std::fetestexcept(FE_INVALID), "lrint overflow is the indefinite integer");
    Require(lrintf_nid_postfix(std::numeric_limits<float>::quiet_NaN()) == INT64_MIN, "lrintf nan");
    Require(lrintl_nid_postfix(1e30L) == INT64_MIN, "lrintl overflow");
    Require(lroundl_nid_postfix(-2.5L) == -3 && llroundl_nid_postfix(0x1p63L) == INT64_MIN, "lroundl");
    std::feclearexcept(FE_ALL_EXCEPT);
    Require(lroundl_nid_postfix(std::numeric_limits<long double>::quiet_NaN()) == INT64_MIN && std::fetestexcept(FE_INVALID), "lroundl nan");
    Require(lroundl_nid_postfix(-9223372036854775808.0L) == INT64_MIN && lroundl_nid_postfix(9223372036854775807.0L) == INT64_MAX, "lroundl range ends");
    std::feclearexcept(FE_ALL_EXCEPT);
}

void CheckSigns() {
    Require(fabsf_nid_postfix(-0.0f) == 0 && !std::signbit(fabsf_nid_postfix(-0.0f)), "fabsf");
    Require(NegativeZero(copysign_nid_postfix(0.0, -1.0)) && copysignl_nid_postfix(3.0L, -0.0L) == -3.0L, "copysign");
    Require(PositiveZero(fmax_nid_postfix(-0.0, 0.0)) && PositiveZero(fmax_nid_postfix(0.0, -0.0)), "fmax of signed zeros");
    Require(NegativeZero(fmin_nid_postfix(0.0, -0.0)) && NegativeZero(fmin_nid_postfix(-0.0, 0.0)), "fmin of signed zeros");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Require(fmax_nid_postfix(nan, 1.0) == 1.0 && fmax_nid_postfix(1.0, nan) == 1.0 && std::isnan(fmax_nid_postfix(nan, nan)), "fmax nan");
    Require(fminf_nid_postfix(std::numeric_limits<float>::quiet_NaN(), -2.0f) == -2.0f && fmaxl_nid_postfix(1.0L, 2.0L) == 2.0L, "fminf and fmaxl");
    Require(fdim_nid_postfix(5.0, 3.0) == 2.0 && PositiveZero(fdim_nid_postfix(3.0, 5.0)) && std::isnan(fdim_nid_postfix(nan, 1.0)), "fdim");
    Require(fdiml_nid_postfix(1.0L, std::numeric_limits<long double>::quiet_NaN()) != fdiml_nid_postfix(1.0L, std::numeric_limits<long double>::quiet_NaN()), "fdiml nan");
}

void CheckExponents() {
    Require(ilogb_nid_postfix(0.0) == -INT_MAX && ilogbf_nid_postfix(-0.0f) == -INT_MAX && ilogbl_nid_postfix(0.0L) == -INT_MAX, "ilogb of zero");
    Require(ilogb_nid_postfix(std::numeric_limits<double>::quiet_NaN()) == INT_MAX && ilogb_nid_postfix(-INFINITY) == INT_MAX, "ilogb of nan and infinity");
    Require(ilogb_nid_postfix(1024.0) == 10 && ilogb_nid_postfix(std::numeric_limits<double>::denorm_min()) == -1074, "ilogb finite");
    Require(ilogbf_nid_postfix(std::numeric_limits<float>::denorm_min()) == -149 && ilogbl_nid_postfix(std::numeric_limits<long double>::denorm_min()) == -16445, "ilogb subnormal");
    Require(logbl_nid_postfix(0.375L) == -2.0L && std::isinf(logbl_nid_postfix(0.0L)), "logbl");
    Require(scalbln_nid_postfix(1.0, 3) == 8.0 && std::isinf(scalbln_nid_postfix(1.0, INT64_MAX)) && PositiveZero(scalbln_nid_postfix(1.0, INT64_MIN)), "scalbln");
    Require(scalbnl_nid_postfix(1.0L, 16383) == std::ldexp(1.0L, 16383), "scalbnl");
}

void CheckRemainders() {
    long double integral = 0;
    Require(modfl_nid_postfix(-3.75L, &integral) == -0.75L && integral == -3.0L, "modfl");
    Require(fmodl_nid_postfix(7.0L, 3.0L) == 1.0L && remainderl_nid_postfix(7.0L, 4.0L) == -1.0L, "fmodl and remainderl");
    int quotient = 0;
    Require(remquo_nid_postfix(29.0, 3.0, &quotient) == -1.0 && quotient == 2, "remquo low bits");
    Require(remquo_nid_postfix(-29.0, 3.0, &quotient) == 1.0 && quotient == -2, "remquo sign");
    Require(remquof_nid_postfix(10.0f, -4.0f, &quotient) == 2.0f && quotient == -2, "remquof");
    Require(remquol_nid_postfix(10000000005.0L, 1.0L, &quotient) == 0 && quotient == 5, "remquol keeps three bits");
    Require(remquo_nid_postfix(1e300, 3.0, &quotient) == std::remainder(1e300, 3.0), "remquo large ratio");
}

void CheckExact() {
    Require(fmaf_nid_postfix(1.0f + 0x1p-12f, 1.0f + 0x1p-12f, -1.0f) == 0x1p-11f + 0x1p-24f, "fmaf single rounding");
    Require(fmal_nid_postfix(2.0L, 3.0L, 1.0L) == 7.0L && sqrtl_nid_postfix(2.25L) == 1.5L, "fmal and sqrtl");
    Require(nextafter_nid_postfix(1.0, 2.0) == 1.0 + std::numeric_limits<double>::epsilon() && nextafter_nid_postfix(1.0, 1.0) == 1.0, "nextafter");
    Require(nexttoward_nid_postfix(0.0, -1.0L) == -std::numeric_limits<double>::denorm_min(), "nexttoward");
    Require(nexttowardf_nid_postfix(1.0f, 1.0L + 1e-10L) > 1.0f, "nexttowardf compares in long double");
}

void CheckErrno() {
    errno = 1234;
    ilogb_nid_postfix(0.0);
    nextafter_nid_postfix(std::numeric_limits<double>::max(), INFINITY);
    sqrtl_nid_postfix(-1.0L);
    fmodl_nid_postfix(1.0L, 0.0L);
    logbl_nid_postfix(0.0L);
    scalbln_nid_postfix(1.0, INT64_MAX);
    Require(errno == 1234, "errno untouched");
}

}

int main() {
    CheckRounding();
    CheckSigns();
    CheckExponents();
    CheckRemainders();
    CheckExact();
    CheckErrno();
    std::puts("guest ieee math tests passed");
    return 0;
}
