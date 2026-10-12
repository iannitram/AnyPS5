#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI getservbyname_r_nid_postfix(const char*, const char*, void*, char*, std::size_t, void**);
int APS5_VABI getservbyport_r_nid_postfix(int, const char*, void*, char*, std::size_t, void**);
int APS5_VABI getservent_r_nid_postfix(void*, char*, std::size_t, void**);
void* APS5_VABI getservbyname_nid_postfix(const char*, const char*);
void* APS5_VABI getservbyport_nid_postfix(int, const char*);
void* APS5_VABI getservent_nid_postfix(void);
void APS5_VABI setservent_nid_postfix(int);
void APS5_VABI endservent_nid_postfix(void);
int APS5_VABI getprotobyname_r_nid_postfix(const char*, void*, char*, std::size_t, void**);
int APS5_VABI getprotobynumber_r_nid_postfix(int, void*, char*, std::size_t, void**);
int APS5_VABI getprotoent_r_nid_postfix(void*, char*, std::size_t, void**);
void* APS5_VABI getprotobyname_nid_postfix(const char*);
void* APS5_VABI getprotobynumber_nid_postfix(int);
void* APS5_VABI getprotoent_nid_postfix(void);
void APS5_VABI setprotoent_nid_postfix(int);
void APS5_VABI endprotoent_nid_postfix(void);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

}

int main() {
    char entry[64];
    char buffer[256];
    void* result = entry;
    Require(getservbyname_r_nid_postfix("http", "tcp", entry, buffer, sizeof(buffer), &result) == ENOENT && result == nullptr, "getservbyname_r");
    result = entry;
    Require(getservbyport_r_nid_postfix(80, "tcp", entry, buffer, sizeof(buffer), &result) == ENOENT && result == nullptr, "getservbyport_r");
    result = entry;
    Require(getservent_r_nid_postfix(entry, buffer, sizeof(buffer), &result) == ENOENT && result == nullptr, "getservent_r");
    setservent_nid_postfix(1);
    errno = 0;
    Require(getservbyname_nid_postfix("http", nullptr) == nullptr && errno == ENOENT, "getservbyname");
    Require(getservbyport_nid_postfix(80, "udp") == nullptr && getservent_nid_postfix() == nullptr, "service lookups");
    endservent_nid_postfix();

    result = entry;
    errno = 0;
    Require(getprotobyname_r_nid_postfix("tcp", entry, buffer, sizeof(buffer), &result) == ENOENT && result == nullptr && errno == ENOENT, "getprotobyname_r");
    result = entry;
    Require(getprotobynumber_r_nid_postfix(6, entry, buffer, sizeof(buffer), &result) == ENOENT && result == nullptr, "getprotobynumber_r");
    result = entry;
    Require(getprotoent_r_nid_postfix(entry, buffer, sizeof(buffer), &result) == ENOENT && result == nullptr, "getprotoent_r");
    setprotoent_nid_postfix(0);
    errno = 0;
    Require(getprotobyname_nid_postfix("udp") == nullptr && errno == ENOENT, "getprotobyname");
    Require(getprotobynumber_nid_postfix(17) == nullptr && getprotoent_nid_postfix() == nullptr, "protocol lookups");
    endprotoent_nid_postfix();
    std::puts("guest network database tests passed");
    return 0;
}
