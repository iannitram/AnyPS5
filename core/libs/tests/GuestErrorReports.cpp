#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

extern "C" {
using ExitHook = void (APS5_VABI *)(int);
void APS5_VABI err_set_file_nid_postfix(void*);
void APS5_VABI err_set_exit_nid_postfix(ExitHook);
[[noreturn]] void APS5_VABI err_nid_postfix(int, const char*, ...);
[[noreturn]] void APS5_VABI errc_nid_postfix(int, int, const char*, ...);
[[noreturn]] void APS5_VABI errx_nid_postfix(int, const char*, ...);
void APS5_VABI warn_nid_postfix(const char*, ...);
void APS5_VABI warnc_nid_postfix(int, const char*, ...);
void APS5_VABI warnx_nid_postfix(const char*, ...);
void APS5_VABI vwarn_nid_postfix(const char*, void*);
void APS5_VABI vwarnx_nid_postfix(const char*, void*);
const char* APS5_VABI getprogname_nid_postfix();
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

struct Exited {
    int value;
};

void APS5_VABI ThrowOnExit(int value) { throw Exited{value}; }

std::string Drain(std::FILE* file) {
    std::fflush(file);
    std::rewind(file);
    std::string text;
    char buffer[256];
    std::size_t count;
    while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0) text.append(buffer, count);
    std::rewind(file);
    return text;
}

void APS5_VABI CallVwarn(const char* format, ...) {
#ifdef _WIN32
    __builtin_sysv_va_list arguments;
    __builtin_sysv_va_start(arguments, format);
#else
    std::va_list arguments;
    va_start(arguments, format);
#endif
    vwarn_nid_postfix(format, arguments);
#ifdef _WIN32
    __builtin_sysv_va_end(arguments);
#else
    va_end(arguments);
#endif
}

void APS5_VABI CallVwarnx(const char* format, ...) {
#ifdef _WIN32
    __builtin_sysv_va_list arguments;
    __builtin_sysv_va_start(arguments, format);
#else
    std::va_list arguments;
    va_start(arguments, format);
#endif
    vwarnx_nid_postfix(format, arguments);
#ifdef _WIN32
    __builtin_sysv_va_end(arguments);
#else
    va_end(arguments);
#endif
}

}

int main() {
    const std::string name = getprogname_nid_postfix();
    std::FILE* file = std::tmpfile();
    Require(file != nullptr, "tmpfile");
    FileStream stream(file);
    err_set_file_nid_postfix(&stream);

    errno = 2;
    warn_nid_postfix("open %s", "a.txt");
    errno = 0;
    warnc_nid_postfix(13, "code %d", 7);
    warnx_nid_postfix("plain %d", 5);
    warn_nid_postfix(nullptr);
    warnx_nid_postfix(nullptr);
    errno = 22;
    CallVwarn("v%s", "warn");
    CallVwarnx("v%sx", "warn");
    const std::string expected = name + ": open a.txt: No such file or directory\n" +
                                 name + ": code 7: Permission denied\n" +
                                 name + ": plain 5\n" +
                                 name + ": No error: 0\n" +
                                 name + ": \n" +
                                 name + ": vwarn: Invalid argument\n" +
                                 name + ": vwarnx\n";
    const std::string written = Drain(file);
    if (written != expected) std::fprintf(stderr, "%s", written.c_str());
    Require(written == expected, "warn output");

    std::FILE* second = std::tmpfile();
    FileStream exits(second);
    err_set_file_nid_postfix(&exits);
    err_set_exit_nid_postfix(ThrowOnExit);
    try {
        errno = 1;
        err_nid_postfix(3, "fatal %c", '!');
    } catch (const Exited& exited) {
        Require(exited.value == 3, "err exit value");
    }
    try {
        errx_nid_postfix(4, "fatal");
    } catch (const Exited& exited) {
        Require(exited.value == 4, "errx exit value");
    }
    try {
        errc_nid_postfix(5, 2, nullptr);
    } catch (const Exited& exited) {
        Require(exited.value == 5, "errc exit value");
    }
    Require(Drain(second) == name + ": fatal !: Operation not permitted\n" + name + ": fatal\n" + name + ": No such file or directory\n", "err output");
    err_set_exit_nid_postfix(nullptr);

#ifndef _WIN32
    const pid_t child = fork();
    Require(child >= 0, "fork");
    if (child == 0) {
        err_set_file_nid_postfix(&exits);
        errx_nid_postfix(42, "exiting");
    }
    int status = 0;
    Require(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 42, "errx exits with the value");
#endif

    err_set_file_nid_postfix(nullptr);
    std::puts("guest error report tests passed");
    return 0;
}
