#include <bit>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "prx/libc/include/General.hpp"

namespace {

constexpr int Infinite = 0x01;
constexpr int NotANumber = 0x02;
constexpr int Normal = 0x04;
constexpr int Subnormal = 0x08;
constexpr int Zero = 0x10;

struct ExtendedBits {
    std::uint64_t mantissa;
    unsigned exponent;
};

ExtendedBits Decode(long double value) {
    static_assert(sizeof(long double) >= 10);
    unsigned char bytes[sizeof(long double)];
    std::memcpy(bytes, &value, sizeof(value));
    ExtendedBits bits{};
    std::memcpy(&bits.mantissa, bytes, 8);
    std::uint16_t top;
    std::memcpy(&top, bytes + 8, 2);
    bits.exponent = top & 0x7fffu;
    return bits;
}

constexpr std::uint64_t IntegerBit = std::uint64_t{1} << 63;

template<typename F>
auto KeepErrno(F function) {
    volatile int& hostErrno = errno;
    const int saved = hostErrno;
    auto result = function();
    hostErrno = saved;
    return result;
}

int Exponent(double x) {
    if (x == 0) return -INT_MAX;
    if (std::isnan(x) || std::isinf(x)) return INT_MAX;
    return KeepErrno([x] { return std::ilogb(x); });
}

template<typename T>
T Scale(T x, T exponent, T limit) {
    if (std::isnan(x) || std::isnan(exponent)) return x * exponent;
    if (!std::isfinite(exponent)) {
        if (exponent > 0) return x * exponent;
        return x / (-exponent);
    }
    if (std::rint(exponent) != exponent) return (exponent - exponent) / (exponent - exponent);
    if (exponent > limit) return KeepErrno([x] { return std::scalbn(x, 65000); });
    if (-exponent > limit) return KeepErrno([x] { return std::scalbn(x, -65000); });
    return KeepErrno([x, exponent] { return std::scalbn(x, static_cast<int>(exponent)); });
}

}

extern "C" {

int APS5_VABI isinf_nid_postfix(double x) {
    const auto bits = std::bit_cast<std::uint64_t>(x);
    return (bits & 0x7fffffffffffffffull) == 0x7ff0000000000000ull;
}

int APS5_VABI isnan_nid_postfix(double x) {
    const auto bits = std::bit_cast<std::uint64_t>(x);
    return (bits & 0x7fffffffffffffffull) > 0x7ff0000000000000ull;
}

int APS5_VABI finite_nid_postfix(double x) {
    const auto high = static_cast<std::uint32_t>(std::bit_cast<std::uint64_t>(x) >> 32);
    return static_cast<int>(((high & 0x7fffffffu) - 0x7ff00000u) >> 31);
}

int APS5_VABI finitef_nid_postfix(float x) {
    const auto bits = std::bit_cast<std::uint32_t>(x);
    return static_cast<int>(((bits & 0x7fffffffu) - 0x7f800000u) >> 31);
}

int APS5_VABI __isinfl_nid_postfix(long double x) {
    const ExtendedBits bits = Decode(x);
    return bits.exponent == 0x7fff && (bits.mantissa & ~IntegerBit) == 0;
}

int APS5_VABI __isnanl_nid_postfix(long double x) {
    const ExtendedBits bits = Decode(x);
    return bits.exponent == 0x7fff && (bits.mantissa & ~IntegerBit) != 0;
}

int APS5_VABI __isfinitel_nid_postfix(long double x) {
    return Decode(x).exponent != 0x7fff;
}

int APS5_VABI __isnormall_nid_postfix(long double x) {
    const unsigned exponent = Decode(x).exponent;
    return exponent != 0 && exponent != 0x7fff;
}

int APS5_VABI __fpclassifyl_nid_postfix(long double x) {
    const ExtendedBits bits = Decode(x);
    if (bits.exponent == 0) return bits.mantissa == 0 ? Zero : Subnormal;
    if (bits.exponent == 0x7fff) return (bits.mantissa & ~IntegerBit) == 0 ? Infinite : NotANumber;
    return Normal;
}

double APS5_VABI drem_nid_postfix(double x, double y) { return KeepErrno([=] { return std::remainder(x, y); }); }
float APS5_VABI dremf_nid_postfix(float x, float y) { return KeepErrno([=] { return std::remainder(x, y); }); }

double APS5_VABI scalb_nid_postfix(double x, double exponent) { return Scale(x, exponent, 65000.0); }
float APS5_VABI scalbf_nid_postfix(float x, float exponent) { return Scale(x, exponent, 65000.0f); }

double APS5_VABI significand_nid_postfix(double x) {
    return Scale(x, static_cast<double>(-Exponent(x)), 65000.0);
}

float APS5_VABI significandf_nid_postfix(float x) {
    return Scale(x, static_cast<float>(-Exponent(x)), 65000.0f);
}

}
