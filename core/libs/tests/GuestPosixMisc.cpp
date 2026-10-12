#include "SceTypes.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestTime.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

extern "C" {
GuestTm* APS5_VABI localtime_r_nid_postfix(const std::int64_t*, GuestTm*);
GuestTm* APS5_VABI gmtime_r_nid_postfix(const std::int64_t*, GuestTm*);
char* APS5_VABI asctime_r_nid_postfix(const GuestTm*, char*);
char* APS5_VABI ctime_r_nid_postfix(const std::int64_t*, char*);
int APS5_VABI isascii_nid_postfix(int);
int APS5_VABI getw_nid_postfix(FileStream*);
int APS5_VABI putw_nid_postfix(int, FileStream*);
void* APS5_VABI valloc_nid_postfix(std::size_t);
void APS5_VABI free_nid_postfix(void*);
int APS5_VABI timespec_get_nid_postfix(KernelTimespec*, int);
std::uint16_t* APS5_VABI seed48_nid_postfix(const std::uint16_t*);
void APS5_VABI lcong48_nid_postfix(const std::uint16_t*);
void APS5_VABI srand48_nid_postfix(std::int64_t);
std::int64_t APS5_VABI lrand48_nid_postfix();
std::int64_t APS5_VABI nrand48_nid_postfix(std::uint16_t*);
std::uint32_t APS5_VABI arc4random_uniform_nid_postfix(std::uint32_t);
int APS5_VABI mkstemps_nid_postfix(char*, int);
char* APS5_VABI mkdtemp_nid_postfix(char*);
int APS5_VABI close_nid_postfix(int);
int* APS5_VABI __error_nid_postfix();
std::size_t APS5_VABI confstr_nid_postfix(int, char*, std::size_t);
char* APS5_VABI getwd_nid_postfix(char*);
char* APS5_VABI gets_nid_postfix(char*);
char* APS5_VABI getcwd_nid_postfix(char*, std::size_t);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s (errno %d)\n", what, *__error_nid_postfix());
        std::abort();
    }
}

void CheckTime() {
    const std::int64_t when = 1000000000;
    GuestTm utc{};
    Require(gmtime_r_nid_postfix(&when, &utc) == &utc && utc.tm_year == 101 && utc.tm_hour == 1 && utc.tm_min == 46, "gmtime_r");
    char buffer[64];
    Require(asctime_r_nid_postfix(&utc, buffer) == buffer && std::strcmp(buffer, "Sun Sep  9 01:46:40 2001\n") == 0, "asctime_r");
    GuestTm local{};
    Require(localtime_r_nid_postfix(&when, &local) == &local, "localtime_r");
    Require(ctime_r_nid_postfix(&when, buffer) == buffer && std::strlen(buffer) == 25, "ctime_r");
    GuestTm odd = utc;
    odd.tm_wday = 9;
    odd.tm_mon = -1;
    Require(std::strcmp(asctime_r_nid_postfix(&odd, buffer), "??? ???  9 01:46:40 2001\n") == 0, "out-of-range names");
    odd = utc;
    odd.tm_year = 99999 - 1900;
    *__error_nid_postfix() = 0;
    Require(asctime_r_nid_postfix(&odd, buffer) == nullptr && *__error_nid_postfix() == 84, "five-digit year overflows");
    odd = utc;
    odd.tm_year = 900 - 1900;
    Require(std::strcmp(asctime_r_nid_postfix(&odd, buffer), "Sun Sep  9 01:46:40 0900\n") == 0, "a short year has strftime's four digits");
    odd.tm_year = -1900;
    Require(std::strcmp(asctime_r_nid_postfix(&odd, buffer), "Sun Sep  9 01:46:40 0000\n") == 0, "year zero");
    odd.tm_year = -1905;
    Require(std::strcmp(asctime_r_nid_postfix(&odd, buffer), "Sun Sep  9 01:46:40 -005\n") == 0, "a small negative year, as _yconv writes it");
    const std::int64_t huge = INT64_MAX;
    *__error_nid_postfix() = 0;
    Require(gmtime_r_nid_postfix(&huge, &utc) == nullptr && *__error_nid_postfix() == 0, "an unrepresentable time leaves errno alone, as tzcode's timesub does");
    Require(std::strcmp(asctime_r_nid_postfix(nullptr, buffer), "??? ??? ?? ??:??:?? ????\n") == 0 && *__error_nid_postfix() == EINVAL, "null tm");

    KernelTimespec now{};
    Require(timespec_get_nid_postfix(&now, 1) == 1 && now.tv_sec > 1600000000 && now.tv_nsec >= 0 && now.tv_nsec < 1000000000, "timespec_get");
    Require(timespec_get_nid_postfix(&now, 2) == 0, "unknown base");
    Require(timespec_get_nid_postfix(nullptr, 1) == 0 && *__error_nid_postfix() == 14, "a null time fails as clock_gettime does");
}

void CheckSmall() {
    Require(isascii_nid_postfix(0x7f) && !isascii_nid_postfix(0x80) && !isascii_nid_postfix(-1), "isascii");
    void* page = valloc_nid_postfix(100);
    Require(page != nullptr && reinterpret_cast<std::uintptr_t>(page) % 16384 == 0, "valloc is page aligned");
    free_nid_postfix(page);

    std::FILE* file = std::tmpfile();
    FileStream stream(file);
    Require(putw_nid_postfix(0x12345678, &stream) == 0 && putw_nid_postfix(-2, &stream) == 0, "putw");
    std::rewind(file);
    Require(getw_nid_postfix(&stream) == 0x12345678 && getw_nid_postfix(&stream) == -2, "getw");
    Require(getw_nid_postfix(&stream) == EOF, "getw at end of file");

    srand48_nid_postfix(1);
    const std::int64_t first = lrand48_nid_postfix();
    const std::uint16_t seed[3] = {0x330e, 1, 0};
    std::uint16_t* previous = seed48_nid_postfix(seed);
    Require(previous != nullptr && lrand48_nid_postfix() == first, "seed48 matches srand48");
    const std::uint16_t parameters[7] = {1, 0, 0, 3, 0, 0, 5};
    lcong48_nid_postfix(parameters);
    std::uint16_t state[3] = {1, 0, 0};
    Require(nrand48_nid_postfix(state) == 0 && state[0] == 8 && state[1] == 0, "lcong48 sets multiplier and addend");
    srand48_nid_postfix(1);
    Require(lrand48_nid_postfix() == first, "srand48 restores the default parameters");

    for (int i = 0; i < 1000; i++) Require(arc4random_uniform_nid_postfix(7) < 7, "arc4random_uniform range");
    Require(arc4random_uniform_nid_postfix(0) == 0 && arc4random_uniform_nid_postfix(1) == 0, "arc4random_uniform small bounds");
}

void CheckConfstr() {
    char buffer[64];
    Require(confstr_nid_postfix(1, buffer, sizeof(buffer)) == 30 && std::strcmp(buffer, "/usr/bin:/bin:/usr/sbin:/sbin") == 0, "_CS_PATH");
    Require(confstr_nid_postfix(1, nullptr, 0) == 30, "_CS_PATH size");
    char small[5];
    Require(confstr_nid_postfix(1, small, sizeof(small)) == 30 && std::strcmp(small, "/usr") == 0, "truncated copy");
    Require(confstr_nid_postfix(8, buffer, sizeof(buffer)) == 1 && buffer[0] == 0, "LP64 flags");
    Require(confstr_nid_postfix(2, buffer, sizeof(buffer)) == 36 && std::strcmp(buffer, "unsupported programming environment") == 0, "ILP32 flags");
    Require(confstr_nid_postfix(14, buffer, sizeof(buffer)) == 21 && std::strcmp(buffer, "_POSIX_V6_LP64_OFF64") == 0, "width restricted environments");
    *__error_nid_postfix() = 0;
    Require(confstr_nid_postfix(99, buffer, sizeof(buffer)) == 0 && *__error_nid_postfix() == EINVAL, "unknown name");
}

void CheckLines() {
    char cwd[1024];
    char wd[1024];
    Require(getcwd_nid_postfix(cwd, sizeof(cwd)) != nullptr && getwd_nid_postfix(wd) == wd && std::strcmp(cwd, wd) == 0, "getwd");

    const std::string input = "anyps5-gets-" + std::to_string(std::rand());
    { std::FILE* file = std::fopen(input.c_str(), "w"); std::fputs("first line\nlast", file); std::fclose(file); }
    Require(std::freopen(input.c_str(), "r", stdin) != nullptr, "redirect stdin");
    char line[64];
    Require(gets_nid_postfix(line) == line && std::strcmp(line, "first line") == 0, "gets drops the newline");
    Require(gets_nid_postfix(line) == line && std::strcmp(line, "last") == 0, "gets returns a last unterminated line");
    Require(gets_nid_postfix(line) == nullptr, "gets at end of file");
    std::fclose(stdin);
    std::filesystem::remove(input);
}

void CheckTemporaries() {
    std::string pattern = "anyps5-mkstemps-XXXXXX.log";
    const int descriptor = mkstemps_nid_postfix(pattern.data(), 4);
    Require(descriptor >= 0 && pattern.size() == 26 && pattern.rfind("anyps5-mkstemps-", 0) == 0 && pattern.rfind(".log") == 22, "mkstemps keeps the prefix and the suffix");
    Require(std::filesystem::exists(pattern), "mkstemps created the file");
    close_nid_postfix(descriptor);
    std::filesystem::remove(pattern);
    std::string bad = "short";
    Require(mkstemps_nid_postfix(bad.data(), 9) == -1 && *__error_nid_postfix() == 22, "suffix longer than the template");
    std::string whole = "abc.log";
    Require(mkstemps_nid_postfix(whole.data(), 7) == -1 && *__error_nid_postfix() == 22 && !std::filesystem::exists(whole), "a suffix as long as the template is EINVAL, as _gettemp checks");
    std::string slash = "anyps5-XXXXXX/x";
    Require(mkstemps_nid_postfix(slash.data(), 2) == -1 && *__error_nid_postfix() == 22, "a suffix with a slash is EINVAL");

    std::string directory = "anyps5-mkdtemp-XXXXXX";
    Require(mkdtemp_nid_postfix(directory.data()) == directory.data() && std::filesystem::is_directory(directory), "mkdtemp");
    std::filesystem::remove(directory);
}

}

int main() {
    std::array<void*, 10> api{};
    ApplicationHeapRegister_nid_no_patch(api.data());
    CheckTime();
    CheckSmall();
    CheckTemporaries();
    CheckConfstr();
    CheckLines();
    std::puts("guest posix misc tests passed");
    return 0;
}
