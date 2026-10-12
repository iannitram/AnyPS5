#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

using ComplexFloat = __complex__ float;
using ComplexDouble = __complex__ double;
using ComplexLongDouble = __complex__ long double;

extern "C" {
double APS5_VABI cabs_nid_postfix(ComplexDouble);
float APS5_VABI cargf_nid_postfix(ComplexFloat);
long double APS5_VABI creall_nid_postfix(ComplexLongDouble);
long double APS5_VABI cimagl_nid_postfix(ComplexLongDouble);
ComplexDouble APS5_VABI conj_nid_postfix(ComplexDouble);
ComplexFloat APS5_VABI cprojf_nid_postfix(ComplexFloat);
ComplexDouble APS5_VABI cproj_nid_postfix(ComplexDouble);
ComplexDouble APS5_VABI csqrt_nid_postfix(ComplexDouble);
ComplexFloat APS5_VABI csqrtf_nid_postfix(ComplexFloat);
ComplexLongDouble APS5_VABI csqrtl_nid_postfix(ComplexLongDouble);
ComplexDouble APS5_VABI cexp_nid_postfix(ComplexDouble);
ComplexFloat APS5_VABI cexpf_nid_postfix(ComplexFloat);
ComplexLongDouble APS5_VABI cexpl_nid_postfix(ComplexLongDouble);
ComplexDouble APS5_VABI clog_nid_postfix(ComplexDouble);
ComplexDouble APS5_VABI clog10_nid_postfix(ComplexDouble);
ComplexDouble APS5_VABI cpow_nid_postfix(ComplexDouble, ComplexDouble);
ComplexLongDouble APS5_VABI cpowl_nid_postfix(ComplexLongDouble, ComplexLongDouble);
ComplexDouble APS5_VABI cacos_nid_postfix(ComplexDouble);
ComplexDouble APS5_VABI catanh_nid_postfix(ComplexDouble);
ComplexFloat APS5_VABI csinhf_nid_postfix(ComplexFloat);
ComplexLongDouble APS5_VABI ctanl_nid_postfix(ComplexLongDouble);

double APS5_VABI erf_nid_postfix(double);
float APS5_VABI erfcf_nid_postfix(float);
double APS5_VABI tgamma_nid_postfix(double);
double APS5_VABI lgamma_nid_postfix(double);
double APS5_VABI lgamma_r_nid_postfix(double, int*);
float APS5_VABI lgammaf_r_nid_postfix(float, int*);
long double APS5_VABI lgammal_nid_postfix(long double);
double APS5_VABI gamma_nid_postfix(double);
double APS5_VABI j0_nid_postfix(double);
double APS5_VABI jn_nid_postfix(int, double);
float APS5_VABI y1f_nid_postfix(float);
long double APS5_VABI atan2l_nid_postfix(long double, long double);
long double APS5_VABI hypotl_nid_postfix(long double, long double);
long double APS5_VABI nanl_nid_postfix(const char*);
extern int signgam_nid_postfix;
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

template<typename TFloat>
bool Near(TFloat value, TFloat expected, TFloat tolerance) {
    return std::fabs(value - expected) <= tolerance * (std::fabs(expected) > 1 ? std::fabs(expected) : 1);
}

template<typename TComplex, typename TFloat>
bool NearComplex(TComplex value, TFloat real, TFloat imaginary, TFloat tolerance) {
    return Near<TFloat>(__real__ value, real, tolerance) && Near<TFloat>(__imag__ value, imaginary, tolerance);
}

template<typename TComplex, typename TFloat>
TComplex Make(TFloat real, TFloat imaginary) {
    TComplex result;
    __real__ result = real;
    __imag__ result = imaginary;
    return result;
}

void CheckComplex() {
    const ComplexDouble z = Make<ComplexDouble>(3.0, 4.0);
    Require(cabs_nid_postfix(z) == 5.0, "cabs");
    Require(Near(cargf_nid_postfix(Make<ComplexFloat>(0.0f, 2.0f)), 1.5707964f, 1e-6f), "cargf");
    const ComplexLongDouble wide = Make<ComplexLongDouble>(1.25L, -2.5L);
    Require(creall_nid_postfix(wide) == 1.25L && cimagl_nid_postfix(wide) == -2.5L, "long double parts");
    const ComplexDouble conjugate = conj_nid_postfix(z);
    Require(__real__ conjugate == 3.0 && __imag__ conjugate == -4.0, "conj");

    const ComplexFloat projected = cprojf_nid_postfix(Make<ComplexFloat>(2.0f, -INFINITY));
    Require(std::isinf(__real__ projected) && __real__ projected > 0 && __imag__ projected == 0 && std::signbit(__imag__ projected), "cprojf infinity");
    const ComplexDouble finite = cproj_nid_postfix(z);
    Require(__real__ finite == 3.0 && __imag__ finite == 4.0, "cproj finite");

    Require(NearComplex(csqrt_nid_postfix(Make<ComplexDouble>(-4.0, 0.0)), 0.0, 2.0, 1e-15), "csqrt negative real");
    Require(NearComplex(csqrtf_nid_postfix(Make<ComplexFloat>(3.0f, 4.0f)), 2.0f, 1.0f, 1e-6f), "csqrtf");
    Require(NearComplex(csqrtl_nid_postfix(Make<ComplexLongDouble>(-3.0L, 4.0L)), 1.0L, 2.0L, 1e-18L), "csqrtl");
    const double pi = std::acos(-1.0);
    Require(NearComplex(cexp_nid_postfix(Make<ComplexDouble>(0.0, pi)), -1.0, 0.0, 1e-15), "cexp");
    Require(NearComplex(cexpf_nid_postfix(Make<ComplexFloat>(1.0f, 0.0f)), 2.7182817f, 0.0f, 1e-6f), "cexpf");
    Require(NearComplex(cexpl_nid_postfix(Make<ComplexLongDouble>(0.0L, 0.0L)), 1.0L, 0.0L, 0.0L), "cexpl");
    Require(NearComplex(clog_nid_postfix(Make<ComplexDouble>(-1.0, 0.0)), 0.0, pi, 1e-15), "clog");
    Require(NearComplex(clog10_nid_postfix(Make<ComplexDouble>(100.0, 0.0)), 2.0, 0.0, 1e-15), "clog10");
    Require(NearComplex(clog10_nid_postfix(Make<ComplexDouble>(0.0, 10.0)), 1.0, pi / 2 / std::log(10.0), 1e-15), "clog10 imaginary");
    Require(NearComplex(cpow_nid_postfix(Make<ComplexDouble>(0.0, 1.0), Make<ComplexDouble>(2.0, 0.0)), -1.0, 0.0, 1e-15), "cpow");
    Require(NearComplex(cpowl_nid_postfix(Make<ComplexLongDouble>(2.0L, 0.0L), Make<ComplexLongDouble>(10.0L, 0.0L)), 1024.0L, 0.0L, 1e-17L), "cpowl");
    Require(NearComplex(cacos_nid_postfix(Make<ComplexDouble>(2.0, 0.0)), 0.0, -1.3169578969248166, 1e-15), "cacos");
    Require(NearComplex(catanh_nid_postfix(Make<ComplexDouble>(0.0, 1.0)), 0.0, pi / 4, 1e-15), "catanh");
    Require(NearComplex(csinhf_nid_postfix(Make<ComplexFloat>(0.0f, 1.0f)), 0.0f, 0.84147096f, 1e-6f), "csinhf");
    Require(NearComplex(ctanl_nid_postfix(Make<ComplexLongDouble>(0.0L, 1.0L)), 0.0L, 0.761594155955764888119L, 1e-18L), "ctanl");
}

void CheckSpecial() {
    Require(Near(erf_nid_postfix(1.0), 0.8427007929497149, 1e-15), "erf");
    Require(Near(erfcf_nid_postfix(2.0f), 0.004677735f, 1e-6f), "erfcf");
    Require(tgamma_nid_postfix(5.0) == 24.0, "tgamma");

    int sign = 0;
    Require(Near(lgamma_r_nid_postfix(-0.5, &sign), std::log(2 * std::sqrt(std::acos(-1.0))), 1e-15) && sign == -1, "lgamma_r negative");
    Require(Near(lgamma_r_nid_postfix(-1.5, &sign), std::log(4 * std::sqrt(std::acos(-1.0)) / 3), 1e-15) && sign == 1, "lgamma_r positive gamma");
    Require(std::isinf(lgamma_r_nid_postfix(-2.0, &sign)) && sign == 1, "lgamma_r pole");
    lgammaf_r_nid_postfix(-0.0f, &sign);
    Require(sign == -1, "lgammaf_r negative zero");
    lgammaf_r_nid_postfix(3.0f, &sign);
    Require(sign == 1, "lgammaf_r positive");
    signgam_nid_postfix = 0;
    Require(Near(lgamma_nid_postfix(4.0), std::log(6.0), 1e-15) && signgam_nid_postfix == 1, "lgamma sets signgam");
    lgammal_nid_postfix(-2.5L);
    Require(signgam_nid_postfix == -1, "lgammal sets signgam");
    Require(gamma_nid_postfix(-0.5) == lgamma_nid_postfix(-0.5), "gamma is lgamma");

    Require(Near(j0_nid_postfix(1.0), 0.7651976865579666, 1e-15), "j0");
    Require(Near(jn_nid_postfix(2, 3.0), 0.48609126058589103, 1e-15), "jn");
    Require(Near(y1f_nid_postfix(1.0f), -0.7812128f, 1e-6f), "y1f");

    Require(Near(atan2l_nid_postfix(1.0L, 1.0L), 0.785398163397448309616L, 1e-18L), "atan2l");
    Require(hypotl_nid_postfix(5.0L, 12.0L) == 13.0L, "hypotl");
    Require(std::isnan(nanl_nid_postfix("")), "nanl");
}

}

int main() {
    CheckComplex();
    CheckSpecial();
    std::puts("guest complex math tests passed");
    return 0;
}
