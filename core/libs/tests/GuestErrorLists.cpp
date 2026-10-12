#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

extern "C" {
extern const char* const sys_errlist_nid_postfix[];
extern const int sys_nerr_nid_postfix;
extern const char* const sys_siglist_nid_postfix[];
extern const char* const sys_signame_nid_postfix[];
extern const int sys_nsig_nid_postfix;
char* APS5_VABI strerror_nid_postfix(int);
void APS5_VABI psignal_nid_postfix(int, const char*);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stdout, "failed: %s\n", what);
        std::abort();
    }
}

}

int main() {
    Require(sys_nerr_nid_postfix == 97 && sys_nsig_nid_postfix == 32, "table sizes");
    for (int error = 0; error < sys_nerr_nid_postfix; error++)
        Require(std::strcmp(sys_errlist_nid_postfix[error], strerror_nid_postfix(error)) == 0, "sys_errlist matches strerror");
    Require(std::strcmp(sys_errlist_nid_postfix[96], "Previous owner died") == 0, "last error");
    Require(std::strcmp(sys_siglist_nid_postfix[11], "Segmentation fault") == 0 && std::strcmp(sys_siglist_nid_postfix[31], "User defined signal 2") == 0, "sys_siglist");
    Require(std::strcmp(sys_signame_nid_postfix[30], "USR1") == 0 && std::strcmp(sys_signame_nid_postfix[0], "Signal 0") == 0, "sys_signame");

    const std::string name = "anyps5-psignal-" + std::to_string(std::rand());
    Require(std::freopen(name.c_str(), "w", stderr) != nullptr, "redirect stderr");
    psignal_nid_postfix(2, "worker");
    psignal_nid_postfix(15, "");
    psignal_nid_postfix(40, nullptr);
    std::fclose(stderr);
    std::ifstream file(name);
    std::stringstream text;
    text << file.rdbuf();
    file.close();
    std::filesystem::remove(name);
    Require(text.str() == "worker: Interrupt\nTerminated\nUnknown signal\n", "psignal output");
    std::puts("guest error list tests passed");
    return 0;
}
