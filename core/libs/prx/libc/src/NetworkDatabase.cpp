#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <stdexcept>

#include "prx/libc/include/General.hpp"

namespace {

struct GuestServiceEntry;
struct GuestProtocolEntry;

constexpr int GuestNoEntry = 2;

template<typename TEntry>
int UnavailableKeepingErrno(TEntry** result) {
    if (result == nullptr) throw std::invalid_argument("network database: null result");
    *result = nullptr;
    return GuestNoEntry;
}

template<typename TEntry>
int Unavailable(TEntry** result) {
    const int error = UnavailableKeepingErrno(result);
    errno = error;
    return error;
}

template<typename TEntry>
TEntry* UnavailableEntry() {
    errno = GuestNoEntry;
    return nullptr;
}

}

extern "C" {

int APS5_VABI getservbyname_r_nid_postfix(const char*, const char*, GuestServiceEntry*, char*, std::size_t, GuestServiceEntry** result) {
    return UnavailableKeepingErrno(result);
}

int APS5_VABI getservbyport_r_nid_postfix(int, const char*, GuestServiceEntry*, char*, std::size_t, GuestServiceEntry** result) {
    return UnavailableKeepingErrno(result);
}

int APS5_VABI getservent_r_nid_postfix(GuestServiceEntry*, char*, std::size_t, GuestServiceEntry** result) { return UnavailableKeepingErrno(result); }
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

extern const char* const h_errlist_nid_postfix[] = {
    "Resolver Error 0 (no error)", "Unknown host", "Host name lookup failure", "Unknown server error", "No address associated with name"
};
extern const int h_nerr_nid_postfix = 5;

const char* APS5_VABI hstrerror_nid_postfix(int error) {
    if (error < 0) return "Resolver internal error";
    if (error < h_nerr_nid_postfix) return h_errlist_nid_postfix[error];
    return "Unknown resolver error";
}

void APS5_VABI herror_nid_postfix(const char* prefix) {
    if (prefix != nullptr && *prefix != '\0') {
        std::fputs(prefix, stderr);
        std::fputs(": ", stderr);
    }
    std::fputs(hstrerror_nid_postfix(0), stderr);
    std::fputs("\n", stderr);
    std::fflush(stderr);
}

}
