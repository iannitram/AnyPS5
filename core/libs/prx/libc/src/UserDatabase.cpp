#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "prx/libc/include/General.hpp"

namespace {

struct GuestPasswd {
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
static_assert(sizeof(GuestPasswd) == 80);

struct GuestGroup;

constexpr int GuestNoEntry = 2;

char empty[] = "";

void InitializePasswd(GuestPasswd* entry) {
    if (entry == nullptr) throw std::invalid_argument("getpw_r: null passwd");
    std::memset(entry, 0, sizeof(*entry));
    entry->user = 0xffffffff;
    entry->group = 0xffffffff;
    entry->name = empty;
    entry->password = empty;
    entry->loginClass = empty;
    entry->gecos = empty;
    entry->directory = empty;
    entry->shell = empty;
}

template<typename TEntry>
int Unavailable(TEntry** result) {
    if (result == nullptr) throw std::invalid_argument("user database: null result");
    *result = nullptr;
    return GuestNoEntry;
}

template<typename TEntry>
TEntry* UnavailableEntry() {
    errno = GuestNoEntry;
    return nullptr;
}

}

extern "C" {

int APS5_VABI getpwnam_r_nid_postfix(const char*, GuestPasswd* entry, char*, std::size_t, GuestPasswd** result) {
    InitializePasswd(entry);
    return Unavailable(result);
}

int APS5_VABI getpwuid_r_nid_postfix(std::uint32_t, GuestPasswd* entry, char*, std::size_t, GuestPasswd** result) {
    InitializePasswd(entry);
    return Unavailable(result);
}

int APS5_VABI getpwent_r_nid_postfix(GuestPasswd* entry, char*, std::size_t, GuestPasswd** result) {
    InitializePasswd(entry);
    return Unavailable(result);
}

GuestPasswd* APS5_VABI getpwnam_nid_postfix(const char*) { return UnavailableEntry<GuestPasswd>(); }
GuestPasswd* APS5_VABI getpwuid_nid_postfix(std::uint32_t) { return UnavailableEntry<GuestPasswd>(); }
GuestPasswd* APS5_VABI getpwent_nid_postfix(void) { return UnavailableEntry<GuestPasswd>(); }
void APS5_VABI setpwent_nid_postfix(void) {}
void APS5_VABI endpwent_nid_postfix(void) {}

int APS5_VABI getgrnam_r_nid_postfix(const char*, GuestGroup*, char*, std::size_t, GuestGroup** result) { return Unavailable(result); }
int APS5_VABI getgrgid_r_nid_postfix(std::uint32_t, GuestGroup*, char*, std::size_t, GuestGroup** result) { return Unavailable(result); }
int APS5_VABI getgrent_r_nid_postfix(GuestGroup*, char*, std::size_t, GuestGroup** result) { return Unavailable(result); }
GuestGroup* APS5_VABI getgrnam_nid_postfix(const char*) { return UnavailableEntry<GuestGroup>(); }
GuestGroup* APS5_VABI getgrgid_nid_postfix(std::uint32_t) { return UnavailableEntry<GuestGroup>(); }
GuestGroup* APS5_VABI getgrent_nid_postfix(void) { return UnavailableEntry<GuestGroup>(); }
int APS5_VABI setgrent_nid_postfix(void) { return 1; }
void APS5_VABI endgrent_nid_postfix(void) {}

int APS5_VABI getgrouplist_nid_postfix(const char* user, std::uint32_t group, std::uint32_t* groups, int* count) {
    if (user == nullptr || count == nullptr) throw std::invalid_argument("getgrouplist: null user or count");
    const int maximum = *count;
    if (maximum >= 1) {
        if (groups == nullptr) throw std::invalid_argument("getgrouplist: null groups");
        groups[0] = group;
    }
    *count = 1;
    return maximum < 1 ? -1 : 0;
}

}
