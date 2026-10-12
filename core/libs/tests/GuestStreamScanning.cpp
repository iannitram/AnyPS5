#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

extern "C" {
int APS5_VABI fscanf_nid_postfix(FileStream*, const char*, ...);
int APS5_VABI vfscanf_nid_postfix(FileStream*, const char*, void*);
int APS5_VABI scanf_nid_postfix(const char*, ...);
int APS5_VABI vscanf_nid_postfix(const char*, void*);
int APS5_VABI feof_nid_postfix(FileStream*);
extern FileStream* __stdinp_nid_postfix;
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

int APS5_VABI CallVfscanf(FileStream* stream, const char* format, ...) {
    APS5_TEST_VA_LIST args;
    APS5_TEST_VA_START(args, format);
    const int result = vfscanf_nid_postfix(stream, format, args);
    APS5_TEST_VA_END(args);
    return result;
}

int APS5_VABI CallVscanf(const char* format, ...) {
    APS5_TEST_VA_LIST args;
    APS5_TEST_VA_START(args, format);
    const int result = vscanf_nid_postfix(format, args);
    APS5_TEST_VA_END(args);
    return result;
}

}

int main() {
    std::FILE* file = std::tmpfile();
    Require(file != nullptr, "tmpfile");
    std::fputs("12 abc 3.5\n7", file);
    std::rewind(file);
    FileStream stream(file);
    int number = 0;
    char word[8] = {};
    double real = 0;
    Require(CallVfscanf(&stream, "%d %7s", &number, word) == 2 && number == 12 && std::strcmp(word, "abc") == 0, "vfscanf");
    Require(fscanf_nid_postfix(&stream, "%lf", &real) == 1 && real == 3.5, "fscanf continues the stream");
    Require(CallVfscanf(&stream, "%d", &number) == 1 && number == 7, "vfscanf last field");
    Require(CallVfscanf(&stream, "%d", &number) == EOF && feof_nid_postfix(&stream), "vfscanf end of file");

    const auto path = std::filesystem::path("anyps5-scanf-test-" + std::to_string(std::rand()) + ".txt");
    std::ofstream(path) << "5 six 0x10";
    Require(std::freopen(path.string().c_str(), "r", stdin) != nullptr, "redirect stdin");
    unsigned hex = 0;
    Require(scanf_nid_postfix("%d %7s", &number, word) == 2 && number == 5 && std::strcmp(word, "six") == 0, "scanf reads stdin");
    Require(CallVscanf("%x", &hex) == 1 && hex == 0x10, "vscanf reads stdin");
    Require(scanf_nid_postfix("%d", &number) == EOF && feof_nid_postfix(__stdinp_nid_postfix), "scanf end of input");
    std::fclose(stdin);
    std::filesystem::remove(path);
    std::puts("guest stream scanning tests passed");
    return 0;
}
