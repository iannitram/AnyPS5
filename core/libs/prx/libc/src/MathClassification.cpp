#include "prx/libc/include/Api.hpp"
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














}
