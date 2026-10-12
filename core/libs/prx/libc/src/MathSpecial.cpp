#include <cerrno>
#include <cmath>
#include <limits>

#include "prx/libc/include/General.hpp"

namespace {

template<typename F>
auto KeepErrno(F function) {
    const int saved = errno;
    auto result = function();
    errno = saved;
    return result;
}

template<typename TFloat>
int GammaSign(TFloat x) {
    if (x == 0) return std::signbit(x) ? -1 : 1;
    if (!(x < 0) || std::isinf(x)) return 1;
    const TFloat whole = std::floor(x);
    if (whole == x) return 1;
    return std::fmod(whole, static_cast<TFloat>(2)) == 0 ? 1 : -1;
}

#ifdef _WIN32
double HostJ0(double x) { return _j0(x); }
double HostJ1(double x) { return _j1(x); }
double HostJn(int n, double x) { return _jn(n, x); }
double HostY0(double x) { return _y0(x); }
double HostY1(double x) { return _y1(x); }
double HostYn(int n, double x) { return _yn(n, x); }
#else
double HostJ0(double x) { return ::j0(x); }
double HostJ1(double x) { return ::j1(x); }
double HostJn(int n, double x) { return ::jn(n, x); }
double HostY0(double x) { return ::y0(x); }
double HostY1(double x) { return ::y1(x); }
double HostYn(int n, double x) { return ::yn(n, x); }
#endif

}

extern "C" {

int signgam_nid_postfix = 0;

double APS5_VABI erf_nid_postfix(double x) { return KeepErrno([&] { return std::erf(x); }); }
float APS5_VABI erff_nid_postfix(float x) { return KeepErrno([&] { return std::erf(x); }); }
long double APS5_VABI erfl_nid_postfix(long double x) { return KeepErrno([&] { return std::erf(x); }); }
double APS5_VABI erfc_nid_postfix(double x) { return KeepErrno([&] { return std::erfc(x); }); }
float APS5_VABI erfcf_nid_postfix(float x) { return KeepErrno([&] { return std::erfc(x); }); }
long double APS5_VABI erfcl_nid_postfix(long double x) { return KeepErrno([&] { return std::erfc(x); }); }

double APS5_VABI tgamma_nid_postfix(double x) { return KeepErrno([&] { return std::tgamma(x); }); }
float APS5_VABI tgammaf_nid_postfix(float x) { return KeepErrno([&] { return std::tgamma(x); }); }
long double APS5_VABI tgammal_nid_postfix(long double x) { return KeepErrno([&] { return std::tgamma(x); }); }

double APS5_VABI lgamma_r_nid_postfix(double x, int* sign) {
    *sign = GammaSign(x);
    return KeepErrno([&] { return std::lgamma(x); });
}

float APS5_VABI lgammaf_r_nid_postfix(float x, int* sign) {
    *sign = GammaSign(x);
    return KeepErrno([&] { return std::lgamma(x); });
}

double APS5_VABI lgamma_nid_postfix(double x) { return lgamma_r_nid_postfix(x, &signgam_nid_postfix); }
float APS5_VABI lgammaf_nid_postfix(float x) { return lgammaf_r_nid_postfix(x, &signgam_nid_postfix); }

long double APS5_VABI lgammal_nid_postfix(long double x) {
    signgam_nid_postfix = GammaSign(x);
    return KeepErrno([&] { return std::lgamma(x); });
}

double APS5_VABI gamma_nid_postfix(double x) { return lgamma_nid_postfix(x); }
float APS5_VABI gammaf_nid_postfix(float x) { return lgammaf_nid_postfix(x); }

double APS5_VABI j0_nid_postfix(double x) { return KeepErrno([&] { return HostJ0(x); }); }
double APS5_VABI j1_nid_postfix(double x) { return KeepErrno([&] { return HostJ1(x); }); }
double APS5_VABI jn_nid_postfix(int n, double x) { return KeepErrno([&] { return HostJn(n, x); }); }
double APS5_VABI y0_nid_postfix(double x) { return KeepErrno([&] { return HostY0(x); }); }
double APS5_VABI y1_nid_postfix(double x) { return KeepErrno([&] { return HostY1(x); }); }
double APS5_VABI yn_nid_postfix(int n, double x) { return KeepErrno([&] { return HostYn(n, x); }); }
float APS5_VABI j0f_nid_postfix(float x) { return KeepErrno([&] { return static_cast<float>(HostJ0(x)); }); }
float APS5_VABI j1f_nid_postfix(float x) { return KeepErrno([&] { return static_cast<float>(HostJ1(x)); }); }
float APS5_VABI jnf_nid_postfix(int n, float x) { return KeepErrno([&] { return static_cast<float>(HostJn(n, x)); }); }
float APS5_VABI y0f_nid_postfix(float x) { return KeepErrno([&] { return static_cast<float>(HostY0(x)); }); }
float APS5_VABI y1f_nid_postfix(float x) { return KeepErrno([&] { return static_cast<float>(HostY1(x)); }); }
float APS5_VABI ynf_nid_postfix(int n, float x) { return KeepErrno([&] { return static_cast<float>(HostYn(n, x)); }); }

float APS5_VABI sinhf_nid_postfix(float x) { return KeepErrno([&] { return std::sinh(x); }); }
float APS5_VABI atanhf_nid_postfix(float x) { return KeepErrno([&] { return std::atanh(x); }); }
float APS5_VABI expm1f_nid_postfix(float x) { return KeepErrno([&] { return std::expm1(x); }); }
long double APS5_VABI acoshl_nid_postfix(long double x) { return KeepErrno([&] { return std::acosh(x); }); }
long double APS5_VABI asinhl_nid_postfix(long double x) { return KeepErrno([&] { return std::asinh(x); }); }
long double APS5_VABI atanhl_nid_postfix(long double x) { return KeepErrno([&] { return std::atanh(x); }); }
long double APS5_VABI asinl_nid_postfix(long double x) { return KeepErrno([&] { return std::asin(x); }); }
long double APS5_VABI atan2l_nid_postfix(long double y, long double x) { return KeepErrno([&] { return std::atan2(y, x); }); }
long double APS5_VABI cbrtl_nid_postfix(long double x) { return KeepErrno([&] { return std::cbrt(x); }); }
long double APS5_VABI expm1l_nid_postfix(long double x) { return KeepErrno([&] { return std::expm1(x); }); }
long double APS5_VABI hypotl_nid_postfix(long double x, long double y) { return KeepErrno([&] { return std::hypot(x, y); }); }
long double APS5_VABI log1pl_nid_postfix(long double x) { return KeepErrno([&] { return std::log1p(x); }); }
long double APS5_VABI log2l_nid_postfix(long double x) { return KeepErrno([&] { return std::log2(x); }); }

long double APS5_VABI nanl_nid_postfix(const char* tag) {
    (void)tag;
    return std::numeric_limits<long double>::quiet_NaN();
}

}
