#include <cerrno>
#include <cstddef>
#include <stdexcept>

#include "prx/libc/include/General.hpp"

namespace {

struct GuestServiceEntry;
struct GuestProtocolEntry;

constexpr int GuestNoEntry = 2;

// The console has no /etc/services or /etc/protocols: every lookup fails as FreeBSD 11's
// files backend does when fopen reports ENOENT.
template<typename TEntry>
int Unavailable(TEntry** result) {
    if (result == nullptr) throw std::invalid_argument("network database: null result");
    *result = nullptr;
    errno = GuestNoEntry;
    return GuestNoEntry;
}

template<typename TEntry>
TEntry* UnavailableEntry() {
    errno = GuestNoEntry;
    return nullptr;
}

}

extern "C" {

int APS5_VABI getservbyname_r_nid_postfix(const char*, const char*, GuestServiceEntry*, char*, std::size_t, GuestServiceEntry** result) {
    return Unavailable(result);
}

int APS5_VABI getservbyport_r_nid_postfix(int, const char*, GuestServiceEntry*, char*, std::size_t, GuestServiceEntry** result) {
    return Unavailable(result);
}

int APS5_VABI getservent_r_nid_postfix(GuestServiceEntry*, char*, std::size_t, GuestServiceEntry** result) { return Unavailable(result); }
GuestServiceEntry* APS5_VABI getservbyname_nid_postfix(const char*, const char*) { return UnavailableEntry<GuestServiceEntry>(); }
GuestServiceEntry* APS5_VABI getservbyport_nid_postfix(int, const char*) { return UnavailableEntry<GuestServiceEntry>(); }
GuestServiceEntry* APS5_VABI getservent_nid_postfix(void) { return UnavailableEntry<GuestServiceEntry>(); }
void APS5_VABI setservent_nid_postfix(int) {}
void APS5_VABI endservent_nid_postfix(void) {}

int APS5_VABI getprotobyname_r_nid_postfix(const char*, GuestProtocolEntry*, char*, std::size_t, GuestProtocolEntry** result) {
    return Unavailable(result);
}

int APS5_VABI getprotobynumber_r_nid_postfix(int, GuestProtocolEntry*, char*, std::size_t, GuestProtocolEntry** result) {
    return Unavailable(result);
}

int APS5_VABI getprotoent_r_nid_postfix(GuestProtocolEntry*, char*, std::size_t, GuestProtocolEntry** result) { return Unavailable(result); }
GuestProtocolEntry* APS5_VABI getprotobyname_nid_postfix(const char*) { return UnavailableEntry<GuestProtocolEntry>(); }
GuestProtocolEntry* APS5_VABI getprotobynumber_nid_postfix(int) { return UnavailableEntry<GuestProtocolEntry>(); }
GuestProtocolEntry* APS5_VABI getprotoent_nid_postfix(void) { return UnavailableEntry<GuestProtocolEntry>(); }
void APS5_VABI setprotoent_nid_postfix(int) {}
void APS5_VABI endprotoent_nid_postfix(void) {}

}
