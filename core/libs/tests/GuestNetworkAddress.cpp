#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

struct InAddress {
    std::uint32_t address;
};

struct EtherAddress {
    std::uint8_t octet[6];
};

struct ControlMessage {
    std::uint32_t length;
    int level;
    int type;
};

}

extern "C" {
std::uint32_t APS5_VABI htonl_nid_postfix(std::uint32_t);
std::uint16_t APS5_VABI ntohs_nid_postfix(std::uint16_t);
int APS5_VABI inet_aton_nid_postfix(const char*, InAddress*);
std::uint32_t APS5_VABI inet_addr_nid_postfix(const char*);
std::uint32_t APS5_VABI inet_network_nid_postfix(const char*);
std::uint32_t APS5_VABI inet_lnaof_nid_postfix(InAddress);
std::uint32_t APS5_VABI inet_netof_nid_postfix(InAddress);
InAddress APS5_VABI inet_makeaddr_nid_postfix(std::uint32_t, std::uint32_t);
char* APS5_VABI inet_ntoa_nid_postfix(InAddress);
unsigned APS5_VABI inet_nsap_addr_nid_postfix(const char*, std::uint8_t*, int);
char* APS5_VABI inet_nsap_ntoa_nid_postfix(int, const std::uint8_t*, char*);
int APS5_VABI ether_line_nid_postfix(const char*, EtherAddress*, char*);
EtherAddress* APS5_VABI ether_aton_nid_postfix(const char*);
char* APS5_VABI ether_ntoa_nid_postfix(const EtherAddress*);
int APS5_VABI ether_hostton_nid_postfix(const char*, EtherAddress*);
int APS5_VABI inet6_option_space_nid_postfix(int);
int APS5_VABI inet6_option_init_nid_postfix(void*, ControlMessage**, int);
int APS5_VABI inet6_option_append_nid_postfix(ControlMessage*, const std::uint8_t*, int, int);
std::uint8_t* APS5_VABI inet6_option_alloc_nid_postfix(ControlMessage*, int, int, int);
int APS5_VABI inet6_option_next_nid_postfix(const ControlMessage*, std::uint8_t**);
int APS5_VABI inet6_option_find_nid_postfix(const ControlMessage*, std::uint8_t**, int);
int APS5_VABI inet6_opt_init_nid_postfix(void*, std::uint32_t);
int APS5_VABI inet6_opt_append_nid_postfix(void*, std::uint32_t, int, std::uint8_t, std::uint32_t, std::uint8_t, void**);
int APS5_VABI inet6_opt_finish_nid_postfix(void*, std::uint32_t, int);
int APS5_VABI inet6_opt_set_val_nid_postfix(void*, int, const void*, std::uint32_t);
int APS5_VABI inet6_opt_get_val_nid_postfix(const void*, int, void*, std::uint32_t);
int APS5_VABI inet6_opt_next_nid_postfix(void*, std::uint32_t, int, std::uint8_t*, std::uint32_t*, void**);
int APS5_VABI inet6_opt_find_nid_postfix(void*, std::uint32_t, int, std::uint8_t, std::uint32_t*, void**);
std::uint32_t APS5_VABI inet6_rth_space_nid_postfix(int, int);
void* APS5_VABI inet6_rth_init_nid_postfix(void*, std::uint32_t, int, int);
int APS5_VABI inet6_rth_add_nid_postfix(void*, const void*);
int APS5_VABI inet6_rth_reverse_nid_postfix(const void*, void*);
int APS5_VABI inet6_rth_segments_nid_postfix(const void*);
std::uint8_t* APS5_VABI inet6_rth_getaddr_nid_postfix(const void*, int);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

std::uint32_t Aton(const char* text) {
    InAddress address{0xdeadbeef};
    return inet_aton_nid_postfix(text, &address) ? htonl_nid_postfix(address.address) : 0xdeadbeef;
}

void CheckIpv4() {
    Require(htonl_nid_postfix(0x01020304) == 0x04030201 && ntohs_nid_postfix(0x1234) == 0x3412, "byte order");
    Require(Aton("192.168.1.2") == 0xc0a80102, "dotted quad");
    Require(Aton("10.1") == 0x0a000001 && Aton("10.1.2") == 0x0a010002, "short forms");
    Require(Aton("0x7f.1") == 0x7f000001 && Aton("012.0") == 0x0a000000, "hex and octal parts");
    Require(Aton("4294967295") == 0xffffffff && Aton("4294967296") == 0, "single part keeps the low 32 bits");
    Require(Aton("1.2.3.4 trailing") == 0x01020304, "whitespace ends the address");
    Require(Aton("1.2.3.256") == 0xdeadbeef && Aton("1.2.3.4.5") == 0xdeadbeef && Aton("08") == 0xdeadbeef, "rejected forms");
    Require(Aton("0x") == 0xdeadbeef && Aton("") == 0xdeadbeef && Aton("1.2.3.4x") == 0xdeadbeef, "rejected digits");
    Require(Aton("0") == 0, "octal zero");
    Require(inet_aton_nid_postfix("1.2.3.4", nullptr) == 1, "null address");
    Require(inet_addr_nid_postfix("127.0.0.1") == 0x0100007f && inet_addr_nid_postfix("bad") == 0xffffffff, "inet_addr");

    Require(inet_network_nid_postfix("10.1.2") == 0x0a0102 && inet_network_nid_postfix("0x10") == 0x10, "inet_network");
    Require(inet_network_nid_postfix("x1f") == 0x1f && inet_network_nid_postfix("1.2.3.4.5") == 0xffffffff, "inet_network edge cases");
    Require(inet_network_nid_postfix("256") == 0xffffffff && inet_network_nid_postfix("") == 0xffffffff, "inet_network rejects");

    const InAddress classA{htonl_nid_postfix(0x0a010203)};
    const InAddress classB{htonl_nid_postfix(0xac100203)};
    const InAddress classC{htonl_nid_postfix(0xc0a80102)};
    Require(inet_netof_nid_postfix(classA) == 0x0a && inet_lnaof_nid_postfix(classA) == 0x010203, "class A");
    Require(inet_netof_nid_postfix(classB) == 0xac10 && inet_lnaof_nid_postfix(classB) == 0x0203, "class B");
    Require(inet_netof_nid_postfix(classC) == 0xc0a801 && inet_lnaof_nid_postfix(classC) == 0x02, "class C");
    Require(inet_makeaddr_nid_postfix(0x0a, 0x010203).address == classA.address, "makeaddr A");
    Require(inet_makeaddr_nid_postfix(0xc0a801, 0x02).address == classC.address, "makeaddr C");
    Require(std::strcmp(inet_ntoa_nid_postfix(classC), "192.168.1.2") == 0, "inet_ntoa");

    std::uint8_t binary[8];
    Require(inet_nsap_addr_nid_postfix("0x47.0005/80+ff", binary, 8) == 5 && binary[0] == 0x47 && binary[4] == 0xff, "nsap_addr");
    Require(inet_nsap_addr_nid_postfix("0x123", binary, 8) == 0 && inet_nsap_addr_nid_postfix("47", binary, 8) == 0, "nsap_addr rejects");
    Require(inet_nsap_addr_nid_postfix("0x0102030405", binary, 2) == 2, "nsap_addr maximum");
    const std::uint8_t nsap[] = {0x47, 0x00, 0x05, 0x80, 0xab};
    char text[32];
    Require(std::strcmp(inet_nsap_ntoa_nid_postfix(5, nsap, text), "0x47.0005.80AB") == 0, "nsap_ntoa");
    Require(std::strcmp(inet_nsap_ntoa_nid_postfix(1, nsap, nullptr), "0x47") == 0, "nsap_ntoa static");
}

void CheckEthernet() {
    const EtherAddress* parsed = ether_aton_nid_postfix("0:1b:21:A:ff:9");
    Require(parsed != nullptr && parsed->octet[1] == 0x1b && parsed->octet[3] == 0x0a && parsed->octet[5] == 0x09, "ether_aton");
    Require(ether_aton_nid_postfix("0:1:2:3:4") == nullptr, "ether_aton rejects");
    Require(std::strcmp(ether_ntoa_nid_postfix(parsed), "00:1b:21:0a:ff:09") == 0, "ether_ntoa");
    EtherAddress address{};
    char host[64];
    Require(ether_line_nid_postfix("8:0:20:1:2:3 sun", &address, host) == 0 && address.octet[0] == 8 && std::strcmp(host, "sun") == 0, "ether_line");
    Require(ether_line_nid_postfix("8:0:20:1:2:3", &address, host) == -1, "ether_line needs a host");
    Require(ether_hostton_nid_postfix("sun", &address) == 1, "no ethers database");
}

void CheckRfc2292Options() {
    alignas(8) std::uint8_t buffer[128] = {};
    Require(inet6_option_space_nid_postfix(6) == 24, "option space");
    ControlMessage* message = nullptr;
    Require(inet6_option_init_nid_postfix(buffer, &message, 99) == -1, "option init type");
    Require(inet6_option_init_nid_postfix(buffer, &message, 49) == 0 && message == reinterpret_cast<ControlMessage*>(buffer), "option init");
    Require(message->level == 41 && message->length == 16, "option init header");
    const std::uint8_t option[] = {0xc2, 4, 1, 2, 3, 4};
    Require(inet6_option_append_nid_postfix(message, option, 4, 2) == 0, "option append");
    Require(message->length == 24 && buffer[17] == 0, "appended length");
    Require(std::memcmp(buffer + 18, option, 6) == 0, "appended option");
    std::uint8_t* data = inet6_option_alloc_nid_postfix(message, 3, 1, 0);
    Require(data == buffer + 24 && message->length == 32 && buffer[17] == 1, "option alloc");
    data[0] = 0;
    std::memcpy(data, "\x05\x01\x07", 3);
    Require(buffer[27] == 1 && buffer[28] == 3, "trailing padding");

    std::uint8_t* cursor = nullptr;
    Require(inet6_option_next_nid_postfix(message, &cursor) == 0 && cursor == buffer + 18, "first option");
    Require(inet6_option_next_nid_postfix(message, &cursor) == 0 && cursor == buffer + 24, "second option");
    cursor = nullptr;
    Require(inet6_option_find_nid_postfix(message, &cursor, 5) == 0 && cursor == buffer + 24, "find option");
    Require(inet6_option_find_nid_postfix(message, &cursor, 0x77) == -1 && cursor == nullptr, "missing option");
}

void CheckRfc3542Options() {
    alignas(8) std::uint8_t buffer[32];
    std::memset(buffer, 0xee, sizeof(buffer));
    void* data = nullptr;
    int offset = inet6_opt_init_nid_postfix(nullptr, 0);
    Require(offset == 2, "opt init size");
    offset = inet6_opt_append_nid_postfix(nullptr, 0, offset, 10, 4, 4, nullptr);
    Require(offset == 8, "opt append size");
    offset = inet6_opt_append_nid_postfix(nullptr, 0, offset, 11, 1, 1, nullptr);
    Require(inet6_opt_finish_nid_postfix(nullptr, 0, offset) == 16, "opt finish size");

    Require(inet6_opt_init_nid_postfix(buffer, 7) == -1, "opt init length");
    offset = inet6_opt_init_nid_postfix(buffer, 16);
    Require(buffer[1] == 1, "opt init header");
    Require(inet6_opt_append_nid_postfix(buffer, 16, offset, 1, 4, 4, &data) == -1, "reserved type");
    Require(inet6_opt_append_nid_postfix(buffer, 16, offset, 10, 2, 4, &data) == -1, "align over length");
    offset = inet6_opt_append_nid_postfix(buffer, 16, offset, 10, 4, 4, &data);
    Require(offset == 8 && data == buffer + 4 && buffer[2] == 10 && buffer[3] == 4, "opt append");
    const std::uint32_t value = 0x11223344;
    Require(inet6_opt_set_val_nid_postfix(data, 0, &value, 4) == 4, "set val");
    offset = inet6_opt_append_nid_postfix(buffer, 16, offset, 11, 1, 1, &data);
    Require(offset == 11 && data == buffer + 10, "second option");
    std::memset(data, 0x5a, 1);
    Require(inet6_opt_append_nid_postfix(buffer, 16, offset, 12, 8, 8, &data) == -1, "option too large");
    Require(inet6_opt_finish_nid_postfix(buffer, 16, offset) == 16, "opt finish");
    Require(buffer[11] == 1 && buffer[12] == 3 && buffer[13] == 0 && buffer[15] == 0, "finish padding");

    std::uint8_t type = 0;
    std::uint32_t length = 0;
    int next = inet6_opt_next_nid_postfix(buffer, 16, 0, &type, &length, &data);
    Require(next == 8 && type == 10 && length == 4, "opt next first");
    std::uint32_t read = 0;
    Require(inet6_opt_get_val_nid_postfix(data, 0, &read, 4) == 4 && read == value, "get val");
    next = inet6_opt_next_nid_postfix(buffer, 16, next, &type, &length, &data);
    Require(next == 11 && type == 11 && length == 1 && *static_cast<std::uint8_t*>(data) == 0x5a, "opt next second");
    Require(inet6_opt_next_nid_postfix(buffer, 16, next, &type, &length, &data) == -1 && data == nullptr, "opt next end");
    Require(inet6_opt_find_nid_postfix(buffer, 16, 0, 11, &length, &data) == 11 && length == 1, "opt find");
    Require(inet6_opt_find_nid_postfix(buffer, 16, 0, 99, &length, &data) == -1, "opt find missing");
}

void CheckRoutingHeader() {
    Require(inet6_rth_space_nid_postfix(0, 3) == 56 && inet6_rth_space_nid_postfix(0, 128) == 0 && inet6_rth_space_nid_postfix(2, 1) == 0, "rth space");
    alignas(8) std::uint8_t buffer[56];
    Require(inet6_rth_init_nid_postfix(buffer, 40, 0, 3) == nullptr, "rth init too small");
    Require(inet6_rth_init_nid_postfix(buffer, sizeof(buffer), 0, 3) == buffer && buffer[1] == 6 && buffer[3] == 0, "rth init");
    std::uint8_t addresses[3][16] = {};
    for (int i = 0; i < 3; i++) addresses[i][15] = static_cast<std::uint8_t>(i + 1);
    for (const auto& address : addresses) Require(inet6_rth_add_nid_postfix(buffer, address) == 0, "rth add");
    Require(inet6_rth_add_nid_postfix(buffer, addresses[0]) == -1, "rth full");
    Require(inet6_rth_segments_nid_postfix(buffer) == 3, "rth segments");
    Require(inet6_rth_getaddr_nid_postfix(buffer, 2)[15] == 3 && inet6_rth_getaddr_nid_postfix(buffer, 3) == nullptr, "rth getaddr");
    alignas(8) std::uint8_t reversed[56];
    Require(inet6_rth_reverse_nid_postfix(buffer, reversed) == 0, "rth reverse");
    Require(reversed[3] == 3 && inet6_rth_getaddr_nid_postfix(reversed, 0)[15] == 3 && inet6_rth_getaddr_nid_postfix(reversed, 2)[15] == 1, "reversed order");
    Require(inet6_rth_reverse_nid_postfix(buffer, buffer) == 0 && inet6_rth_getaddr_nid_postfix(buffer, 0)[15] == 3, "rth reverse in place");
    buffer[2] = 2;
    Require(inet6_rth_segments_nid_postfix(buffer) == -1 && inet6_rth_add_nid_postfix(buffer, addresses[0]) == -1, "unknown routing type");
}

}

int main() {
    CheckIpv4();
    CheckEthernet();
    CheckRfc2292Options();
    CheckRfc3542Options();
    CheckRoutingHeader();
    std::puts("guest network address tests passed");
    return 0;
}
