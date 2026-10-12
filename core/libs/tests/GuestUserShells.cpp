#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
char* APS5_VABI getusershell_nid_postfix(void);
void APS5_VABI setusershell_nid_postfix(void);
void APS5_VABI endusershell_nid_postfix(void);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

bool Is(const char* shell, const char* expected) { return shell != nullptr && std::strcmp(shell, expected) == 0; }

}

int main() {
    Require(Is(getusershell_nid_postfix(), "/bin/sh"), "first shell");
    Require(Is(getusershell_nid_postfix(), "/bin/csh"), "second shell");
    Require(getusershell_nid_postfix() == nullptr && getusershell_nid_postfix() == nullptr, "end of list");
    setusershell_nid_postfix();
    Require(Is(getusershell_nid_postfix(), "/bin/sh"), "setusershell rewinds");
    endusershell_nid_postfix();
    Require(Is(getusershell_nid_postfix(), "/bin/sh"), "endusershell restarts the list");
    std::puts("guest user shell tests passed");
    return 0;
}
