#include "prx/libSceLibcInternal/include/Api.hpp"
#include <cerrno>
#include <cstdarg>

#include "SceTypes.hpp"
#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {
extern FileStream* __stderrp_nid_postfix;
const char* APS5_VABI getprogname_nid_postfix();
char* APS5_VABI strerror_nid_postfix(int error);
int APS5_VABI fprintf_nid_postfix(FileStream* stream, const char* format, ...);
int APS5_VABI vfprintf_nid_postfix(FileStream* stream, const char* format, VaList* arguments);
[[noreturn]] void APS5_VABI exit_nid_postfix(int code);
}

namespace {

using ExitHook = void (APS5_VABI *)(int);

FileStream* errorStream = nullptr;
ExitHook exitHook = nullptr;

FileStream* Stream() {
    if (errorStream == nullptr) errorStream = __stderrp_nid_postfix;
    return errorStream;
}

void Report(bool withCode, int code, const char* format, VaList* arguments) {
    FileStream* stream = Stream();
    fprintf_nid_postfix(stream, "%s: ", getprogname_nid_postfix());
    if (format != nullptr) {
        vfprintf_nid_postfix(stream, format, arguments);
        if (withCode) fprintf_nid_postfix(stream, ": ");
    }
    if (withCode) fprintf_nid_postfix(stream, "%s\n", strerror_nid_postfix(code));
    else fprintf_nid_postfix(stream, "\n");
}

[[noreturn]] void Exit(int value) {
    if (exitHook != nullptr) exitHook(value);
    exit_nid_postfix(value);
}

}

#ifdef _WIN32
#define APS5_GUEST_VA_LIST __builtin_sysv_va_list
#define APS5_GUEST_VA_START __builtin_sysv_va_start
#define APS5_GUEST_VA_END __builtin_sysv_va_end
#else
#define APS5_GUEST_VA_LIST std::va_list
#define APS5_GUEST_VA_START va_start
#define APS5_GUEST_VA_END va_end
#endif

extern "C" {















}
