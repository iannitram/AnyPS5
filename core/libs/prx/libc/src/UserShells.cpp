#include <cstddef>
#include <mutex>

#include "prx/libc/include/General.hpp"

namespace {

// FreeBSD 11 falls back to these when /etc/shells cannot be read; the console has no /etc.
constexpr const char* DefaultShells[] = {"/bin/sh", "/bin/csh", nullptr};

std::mutex shellsLock;
const char* const* cursor = nullptr;

}

extern "C" {

char* APS5_VABI getusershell_nid_postfix(void) {
    std::lock_guard lock(shellsLock);
    if (cursor == nullptr) cursor = DefaultShells;
    const char* shell = *cursor;
    if (shell != nullptr) ++cursor;
    return const_cast<char*>(shell);
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
