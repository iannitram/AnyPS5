#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI fwprintf_nid_postfix(FileStream*, const char16_t*, ...);
int APS5_VABI vfwprintf_nid_postfix(FileStream*, const char16_t*, void*);
int APS5_VABI putwc_nid_postfix(char16_t, FileStream*);
std::int32_t APS5_VABI getwc_nid_postfix(FileStream*);
char16_t* APS5_VABI fgetws_nid_postfix(char16_t*, int, FileStream*);
int APS5_VABI fwide_nid_postfix(FileStream*, int);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

#ifdef _WIN32
#define APS5_TEST_VA_LIST __builtin_sysv_va_list
#define APS5_TEST_VA_START __builtin_sysv_va_start
#define APS5_TEST_VA_END __builtin_sysv_va_end
#else
#define APS5_TEST_VA_LIST std::va_list
#define APS5_TEST_VA_START va_start
#define APS5_TEST_VA_END va_end
#endif

int APS5_VABI CallVfwprintf(FileStream* stream, const char16_t* format, ...) {
    APS5_TEST_VA_LIST args;
    APS5_TEST_VA_START(args, format);
    const int result = vfwprintf_nid_postfix(stream, format, args);
    APS5_TEST_VA_END(args);
    return result;
}

std::string Contents(std::FILE* file) {
    std::fflush(file);
    std::rewind(file);
    std::string text;
    char buffer[256];
    std::size_t count;
    while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0) text.append(buffer, count);
    std::rewind(file);
    return text;
}

}

int main() {
    std::FILE* file = std::tmpfile();
    Require(file != nullptr, "tmpfile");
    FileStream stream(file);
    Require(fwprintf_nid_postfix(&stream, u"%d-%ls|", 42, u"wide") == 8, "fwprintf");
    Require(CallVfwprintf(&stream, u"%s %c\n", "narrow", 'x') == 9, "vfwprintf");
    Require(putwc_nid_postfix(u'é', &stream) == u'é', "putwc");
    Require(putwc_nid_postfix(u'\n', &stream) == u'\n', "putwc newline");
    Require(Contents(file) == "42-wide|narrow x\n\xc3\xa9\n", "UTF-8 output");

    std::FILE* input = std::tmpfile();
    std::fputs("line one\nab", input);
    std::rewind(input);
    FileStream reader(input);
    char16_t buffer[32];
    Require(fgetws_nid_postfix(buffer, 32, &reader) == buffer && std::u16string(buffer) == u"line one\n", "fgetws stops after newline");
    Require(fgetws_nid_postfix(buffer, 2, &reader) == buffer && std::u16string(buffer) == u"a", "fgetws respects the size");
    Require(getwc_nid_postfix(&reader) == u'b', "getwc");
    Require(fgetws_nid_postfix(buffer, 32, &reader) == nullptr, "fgetws at end of file");
    Require(fgetws_nid_postfix(buffer, 0, &reader) == nullptr && errno == 22, "fgetws size 0");

    Require(fwide_nid_postfix(&reader, 0) == 1 && fwide_nid_postfix(&reader, -3) == 1, "fgetwc orients the stream wide, as FreeBSD's ORIENT does");
    Require(fwide_nid_postfix(&stream, 0) == 1, "fwprintf orients the stream wide");
    std::FILE* unoriented = std::tmpfile();
    Require(unoriented != nullptr, "tmpfile");
    FileStream fresh(unoriented);
    Require(fwide_nid_postfix(&fresh, 0) == 0, "no orientation");
    Require(fwide_nid_postfix(&fresh, -5) == -1 && fwide_nid_postfix(&fresh, 1) == -1 && fwide_nid_postfix(&fresh, 0) == -1, "byte orientation sticks");

    bool threw = false;
    try {
        fgetws_nid_postfix(nullptr, 4, &reader);
    } catch (const std::exception&) {
        threw = true;
    }
    Require(threw, "null buffer throws");
    threw = false;
    try {
        fwprintf_nid_postfix(&fresh, u"%1$d|", 5);
    } catch (const std::exception&) {
        threw = true;
    }
    Require(threw, "an unsupported conversion throws instead of failing silently");
    std::fclose(unoriented);
    std::puts("guest wide stream tests passed");
    return 0;
}
