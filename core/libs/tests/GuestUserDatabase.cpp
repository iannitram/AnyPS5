#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

struct Passwd {
    char* name;
    char* password;
    std::uint32_t user;
    std::uint32_t group;
    std::int64_t change;
    char* loginClass;
    char* gecos;
    char* directory;
    char* shell;
    std::int64_t expire;
    int fields;
};

}

extern "C" {
int APS5_VABI getpwnam_r_nid_postfix(const char*, Passwd*, char*, std::size_t, Passwd**);
int APS5_VABI getpwuid_r_nid_postfix(std::uint32_t, Passwd*, char*, std::size_t, Passwd**);
int APS5_VABI getpwent_r_nid_postfix(Passwd*, char*, std::size_t, Passwd**);
Passwd* APS5_VABI getpwnam_nid_postfix(const char*);
Passwd* APS5_VABI getpwuid_nid_postfix(std::uint32_t);
Passwd* APS5_VABI getpwent_nid_postfix(void);
void APS5_VABI setpwent_nid_postfix(void);
void APS5_VABI endpwent_nid_postfix(void);
int APS5_VABI getgrnam_r_nid_postfix(const char*, void*, char*, std::size_t, void**);
int APS5_VABI getgrgid_r_nid_postfix(std::uint32_t, void*, char*, std::size_t, void**);
void* APS5_VABI getgrnam_nid_postfix(const char*);
void* APS5_VABI getgrgid_nid_postfix(std::uint32_t);
void* APS5_VABI getgrent_nid_postfix(void);
int APS5_VABI setgrent_nid_postfix(void);
void APS5_VABI endgrent_nid_postfix(void);
int APS5_VABI getgrouplist_nid_postfix(const char*, std::uint32_t, std::uint32_t*, int*);
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
    Passwd entry;
    std::memset(&entry, 0x5a, sizeof(entry));
    char buffer[256];
    Passwd* result = &entry;
    Require(getpwnam_r_nid_postfix("root", &entry, buffer, sizeof(buffer), &result) == ENOENT && result == nullptr, "getpwnam_r");
    Require(entry.user == 0xffffffffu && entry.group == 0xffffffffu && entry.name[0] == 0 && entry.shell[0] == 0 && entry.change == 0 && entry.fields == 0, "passwd is reset");
    result = &entry;
    Require(getpwuid_r_nid_postfix(0, &entry, buffer, sizeof(buffer), &result) == ENOENT && result == nullptr, "getpwuid_r");
    Require(getpwent_r_nid_postfix(&entry, buffer, sizeof(buffer), &result) == ENOENT && result == nullptr, "getpwent_r");
    setpwent_nid_postfix();
    errno = 0;
    Require(getpwnam_nid_postfix("root") == nullptr && errno == ENOENT, "getpwnam");
    errno = 0;
    Require(getpwuid_nid_postfix(0) == nullptr && errno == ENOENT, "getpwuid");
    Require(getpwent_nid_postfix() == nullptr, "getpwent");
    endpwent_nid_postfix();

    void* group = &entry;
    Require(getgrnam_r_nid_postfix("wheel", buffer, buffer, sizeof(buffer), &group) == ENOENT && group == nullptr, "getgrnam_r");
    group = &entry;
    Require(getgrgid_r_nid_postfix(0, buffer, buffer, sizeof(buffer), &group) == ENOENT && group == nullptr, "getgrgid_r");
    Require(setgrent_nid_postfix() == 1, "setgrent returns 1, as getgrent.c does");
    errno = 0;
    Require(getgrnam_nid_postfix("wheel") == nullptr && getgrgid_nid_postfix(0) == nullptr && getgrent_nid_postfix() == nullptr && errno == ENOENT, "group lookups");
    endgrent_nid_postfix();

    std::uint32_t groups[4] = {7, 7, 7, 7};
    int count = 4;
    errno = 0;
    Require(getgrouplist_nid_postfix("user", 100, groups, &count) == 0 && count == 1 && groups[0] == 100 && groups[1] == 7, "getgrouplist");
    Require(errno == 0, "getgrouplist keeps errno, as nsdispatch restores it");
    count = 0;
    Require(getgrouplist_nid_postfix("user", 100, nullptr, &count) == -1 && count == 1, "getgrouplist sizing");
    std::puts("guest user database tests passed");
    return 0;
}
