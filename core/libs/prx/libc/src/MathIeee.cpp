#include "prx/libc/include/Api.hpp"
#include <algorithm>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
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

bool IsNan(double x) {
    std::uint64_t bits;
    std::memcpy(&bits, &x, sizeof(bits));
    return (bits & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL;
}

bool IsNan(float x) {
    std::uint32_t bits;
    std::memcpy(&bits, &x, sizeof(bits));
    return (bits & 0x7fffffffU) > 0x7f800000U;
}

struct X87Bits {
    std::uint64_t mantissa;
    unsigned exponent;
};

X87Bits Decode(long double x) {
    unsigned char bytes[sizeof(long double)];
    std::memcpy(bytes, &x, sizeof(bytes));
    X87Bits bits {};
    std::uint16_t signAndExponent;
    std::memcpy(&bits.mantissa, bytes, sizeof(bits.mantissa));
    std::memcpy(&signAndExponent, bytes + 8, sizeof(signAndExponent));
    bits.exponent = signAndExponent & 0x7fffU;
    return bits;
}

bool IsNan(long double x) {
    const X87Bits bits = Decode(x);
    return bits.exponent == 0x7fffU && (bits.mantissa & ~(1ULL << 63)) != 0;
}

template<typename T>
T Maximum(T x, T y) {
    if (IsNan(x)) return y;
    if (IsNan(y)) return x;
    if (std::signbit(x) != std::signbit(y)) return std::signbit(x) ? y : x;
    return x > y ? x : y;
}

template<typename T>
T Minimum(T x, T y) {
    if (IsNan(x)) return y;
    if (IsNan(y)) return x;
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

int ExponentOfX87(long double x) {
    const X87Bits bits = Decode(x);
    if (bits.exponent == 0x7fffU) return INT_MAX;
    if (bits.exponent != 0) return static_cast<int>(bits.exponent) - 16383;
    if (bits.mantissa == 0) return -INT_MAX;
    return -16382 - std::countl_zero(bits.mantissa);
}

std::int64_t ConvertX87(long double x) {
    std::int64_t result;
    asm volatile("fistpll %0" : "=m"(result) : "t"(x) : "st");
    return result;
}

std::int64_t RoundToLong(long double x) {
    if (x > -0x1p63L && x < 0x1p63L - 0.5L) return static_cast<std::int64_t>(std::round(x));
    std::feraiseexcept(FE_INVALID);
    return INT64_MAX;
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





long double APS5_VABI nextafterl_nid_postfix(long double x, long double y) { return KeepErrno([=] { return std::nextafter(x, y); }); }
double APS5_VABI nexttoward_nid_postfix(double x, long double y) { return KeepErrno([=] { return std::nexttoward(x, y); }); }
float APS5_VABI nexttowardf_nid_postfix(float x, long double y) { return KeepErrno([=] { return std::nexttoward(x, y); }); }
long double APS5_VABI nexttowardl_nid_postfix(long double x, long double y) { return KeepErrno([=] { return std::nexttoward(x, y); }); }





}
