#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

extern "C" {
int APS5_VABI isinf_nid_postfix(double);
int APS5_VABI isnan_nid_postfix(double);
int APS5_VABI finite_nid_postfix(double);
int APS5_VABI finitef_nid_postfix(float);
int APS5_VABI __isinfl_nid_postfix(long double);
int APS5_VABI __isnanl_nid_postfix(long double);
int APS5_VABI __isfinitel_nid_postfix(long double);
int APS5_VABI __isnormall_nid_postfix(long double);
int APS5_VABI __fpclassifyl_nid_postfix(long double);
double APS5_VABI drem_nid_postfix(double, double);
float APS5_VABI dremf_nid_postfix(float, float);
double APS5_VABI scalb_nid_postfix(double, double);
float APS5_VABI scalbf_nid_postfix(float, float);
double APS5_VABI significand_nid_postfix(double);
float APS5_VABI significandf_nid_postfix(float);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

long double Extended(std::uint64_t mantissa, std::uint16_t exponent) {
    long double value = 0;
    unsigned char bytes[sizeof(long double)] = {};
    std::memcpy(bytes, &mantissa, 8);
    std::memcpy(bytes + 8, &exponent, 2);
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

constexpr std::uint64_t IntegerBit = std::uint64_t{1} << 63;
constexpr double Infinity = std::numeric_limits<double>::infinity();
constexpr double Nan = std::numeric_limits<double>::quiet_NaN();

}

int main() {
    Require(isinf_nid_postfix(Infinity) && isinf_nid_postfix(-Infinity) && !isinf_nid_postfix(Nan) && !isinf_nid_postfix(1e308), "isinf");
    Require(isnan_nid_postfix(Nan) && isnan_nid_postfix(-Nan) && !isnan_nid_postfix(Infinity) && !isnan_nid_postfix(0.0), "isnan");
    Require(finite_nid_postfix(1.0) == 1 && finite_nid_postfix(std::numeric_limits<double>::denorm_min()) == 1 && finite_nid_postfix(Infinity) == 0 && finite_nid_postfix(Nan) == 0, "finite");
    Require(finitef_nid_postfix(-3.0f) == 1 && finitef_nid_postfix(std::numeric_limits<float>::infinity()) == 0, "finitef");

    Require(__fpclassifyl_nid_postfix(0.0L) == 0x10 && __fpclassifyl_nid_postfix(-0.0L) == 0x10, "zero");
    Require(__fpclassifyl_nid_postfix(1.5L) == 0x04 && __fpclassifyl_nid_postfix(std::numeric_limits<long double>::denorm_min()) == 0x08, "normal and subnormal");
    Require(__fpclassifyl_nid_postfix(std::numeric_limits<long double>::infinity()) == 0x01 && __fpclassifyl_nid_postfix(std::numeric_limits<long double>::quiet_NaN()) == 0x02, "infinity and nan");
    const long double pseudoInfinity = Extended(0, 0x7fff);
    const long double pseudoNan = Extended(1, 0x7fff);
    const long double unnormal = Extended(0x4000000000000000ull, 0x3fff);
    const long double pseudoDenormal = Extended(IntegerBit, 0);
    Require(__fpclassifyl_nid_postfix(pseudoInfinity) == 0x01 && __isinfl_nid_postfix(pseudoInfinity) && !__isnanl_nid_postfix(pseudoInfinity), "pseudo-infinity ignores the integer bit");
    Require(__fpclassifyl_nid_postfix(pseudoNan) == 0x02 && __isnanl_nid_postfix(pseudoNan), "pseudo-nan");
    Require(__fpclassifyl_nid_postfix(unnormal) == 0x04 && __isnormall_nid_postfix(unnormal) && __isfinitel_nid_postfix(unnormal), "unnormal is normal by exponent");
    Require(__fpclassifyl_nid_postfix(pseudoDenormal) == 0x08 && !__isnormall_nid_postfix(pseudoDenormal), "pseudo-denormal is subnormal");
    Require(!__isfinitel_nid_postfix(std::numeric_limits<long double>::infinity()) && __isfinitel_nid_postfix(0.0L) && !__isnormall_nid_postfix(0.0L), "finite and normal");

    Require(drem_nid_postfix(7.0, 4.0) == -1.0 && dremf_nid_postfix(5.0f, 2.0f) == 1.0f, "drem rounds the quotient to even");

    Require(scalb_nid_postfix(2.0, 3.0) == 16.0 && scalb_nid_postfix(16.0, -4.0) == 1.0, "scalb");
    Require(std::isnan(scalb_nid_postfix(1.0, 0.5)) && std::isnan(scalb_nid_postfix(Nan, 1.0)) && std::isnan(scalb_nid_postfix(1.0, Nan)), "scalb non-integral and nan");
    Require(scalb_nid_postfix(1.0, Infinity) == Infinity && scalb_nid_postfix(1.0, -Infinity) == 0.0, "scalb infinite exponents");
    Require(std::isnan(scalb_nid_postfix(0.0, Infinity)) && std::isnan(scalb_nid_postfix(Infinity, -Infinity)), "scalb invalid products");
    errno = 0;
    Require(scalb_nid_postfix(1.0, 70000.0) == Infinity && scalb_nid_postfix(1.0, -70000.0) == 0.0 && errno == 0, "scalb clamps the exponent");
    Require(scalbf_nid_postfix(3.0f, 2.0f) == 12.0f && std::isnan(scalbf_nid_postfix(1.0f, 1.25f)), "scalbf");

    Require(significand_nid_postfix(12.0) == 1.5 && significand_nid_postfix(-0.375) == -1.5, "significand");
    Require(significand_nid_postfix(std::numeric_limits<double>::denorm_min()) == 1.0, "significand of a subnormal");
    Require(significand_nid_postfix(0.0) == 0.0 && significand_nid_postfix(Infinity) == Infinity && std::isnan(significand_nid_postfix(Nan)), "significand special values");
    Require(significandf_nid_postfix(48.0f) == 1.5f && significandf_nid_postfix(0.0f) == 0.0f, "significandf");
    std::puts("guest math classification tests passed");
    return 0;
}
