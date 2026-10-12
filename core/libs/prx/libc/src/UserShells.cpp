#include <cstddef>
#include <mutex>

#include "prx/libc/include/General.hpp"

namespace {

char BourneShell[] = "/bin/sh";
char CShell[] = "/bin/csh";
char* DefaultShells[] = {BourneShell, CShell, nullptr};

std::mutex shellsLock;
char* const* cursor = nullptr;

}

extern "C" {

char* APS5_VABI getusershell_nid_postfix(void) {
    std::lock_guard lock(shellsLock);
    if (cursor == nullptr) cursor = DefaultShells;
    char* shell = *cursor;
    if (shell != nullptr) ++cursor;
    return shell;
}

void APS5_VABI setusershell_nid_postfix(void) {
    std::lock_guard lock(shellsLock);
    cursor = DefaultShells;
}

void APS5_VABI endusershell_nid_postfix(void) {
    std::lock_guard lock(shellsLock);
    cursor = nullptr;
}

}
