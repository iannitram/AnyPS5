#include "prx/libc/include/general/VabiMacros.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

extern "C" {
int APS5_VABI tcgetattr_nid_postfix(int, void*);
int APS5_VABI tcsetattr_nid_postfix(int, int, const void*);
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI pipe_nid_postfix(int*);
int APS5_VABI socket_nid_postfix(int, int, int);
int APS5_VABI close_nid_postfix(int);
int* APS5_VABI __error_nid_postfix();
int APS5_VABI tcdrain_nid_postfix(int);
int APS5_VABI tcflush_nid_postfix(int, int);
int APS5_VABI tcflow_nid_postfix(int, int);
int APS5_VABI tcsendbreak_nid_postfix(int, int);
int APS5_VABI tcgetpgrp_nid_postfix(int);
int APS5_VABI tcsetpgrp_nid_postfix(int, int);
int APS5_VABI tcgetsid_nid_postfix(int);
std::uint32_t APS5_VABI cfgetispeed_nid_postfix(const void*);
std::uint32_t APS5_VABI cfgetospeed_nid_postfix(const void*);
int APS5_VABI cfsetispeed_nid_postfix(void*, std::uint32_t);
int APS5_VABI cfsetospeed_nid_postfix(void*, std::uint32_t);
int APS5_VABI cfsetspeed_nid_postfix(void*, std::uint32_t);
void APS5_VABI cfmakeraw_nid_postfix(void*);
int APS5_VABI ttyname_r_nid_postfix(int, char*, std::size_t);
char* APS5_VABI ttyname_nid_postfix(int);
char* APS5_VABI ctermid_nid_postfix(char*);
}

static void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "terminal check failed at line %d, errno %d\n", line, *__error_nid_postfix());
        std::abort();
    }
}
#define Require(value) Check((value), __LINE__)

constexpr int Now = 0;
constexpr int Drain = 1;
constexpr int Flush = 2;
constexpr int Soft = 0x10;
constexpr int NotTerminal = 25;
constexpr int BadDescriptor = 9;
constexpr int InvalidArgument = 22;
constexpr int BadAddress = 14;
constexpr int AddressNotAvailable = 49;

static bool Fails(int result, int error) { return result == -1 && *__error_nid_postfix() == error; }

static void RequireTerminalCalls(int descriptor, int error) {
    Require(Fails(tcdrain_nid_postfix(descriptor), error));
    Require(Fails(tcsendbreak_nid_postfix(descriptor, 0), error));
    Require(Fails(tcgetpgrp_nid_postfix(descriptor), error));
    Require(Fails(tcsetpgrp_nid_postfix(descriptor, 1), error));
    Require(Fails(tcgetsid_nid_postfix(descriptor), error));
    for (const int queue : {1, 2, 3}) Require(Fails(tcflush_nid_postfix(descriptor, queue), error));
    for (const int action : {1, 2, 3, 4}) Require(Fails(tcflow_nid_postfix(descriptor, action), error));
    char name[32];
    std::memset(name, 'x', sizeof(name));
    Require(ttyname_r_nid_postfix(descriptor, name, sizeof(name)) == error && name[0] == 0);
    *__error_nid_postfix() = 0;
    Require(ttyname_nid_postfix(descriptor) == nullptr && *__error_nid_postfix() == error);
}

static void CheckAttributeHelpers() {
    struct {
        std::uint32_t input, output, control, local;
        unsigned char characters[20];
        std::uint32_t inputSpeed, outputSpeed;
    } attributes;
    std::memset(&attributes, 0xff, sizeof(attributes));
    static_assert(sizeof(attributes) == 44);
    Require(cfsetispeed_nid_postfix(&attributes, 9600) == 0 && cfgetispeed_nid_postfix(&attributes) == 9600);
    Require(cfsetospeed_nid_postfix(&attributes, 38400) == 0 && cfgetospeed_nid_postfix(&attributes) == 38400);
    Require(cfgetispeed_nid_postfix(&attributes) == 9600);
    Require(cfsetspeed_nid_postfix(&attributes, 115200) == 0 && attributes.inputSpeed == 115200 && attributes.outputSpeed == 115200);
    cfmakeraw_nid_postfix(&attributes);
    Require(attributes.input == (0xffffffffu & ~0x27feu));
    Require(attributes.output == 0xfffffffeu);
    Require(attributes.local == (0xffffffffu & ~0xa040059eu));
    Require(attributes.control == (0xffffffffu & ~0x1000u));
    Require(attributes.characters[16] == 1 && attributes.characters[17] == 0 && attributes.characters[15] == 0xff && attributes.characters[18] == 0xff);
    Require(attributes.inputSpeed == 115200);
    std::memset(&attributes, 0, sizeof(attributes));
    cfmakeraw_nid_postfix(&attributes);
    Require(attributes.input == 0x1u && attributes.output == 0 && attributes.control == 0xb00u && attributes.local == 0);
    Require(attributes.characters[16] == 1 && attributes.characters[17] == 0);

    char buffer[32];
    Require(std::strcmp(ctermid_nid_postfix(buffer), "/dev/tty") == 0 && ctermid_nid_postfix(buffer) == buffer);
    Require(std::strcmp(ctermid_nid_postfix(nullptr), "/dev/tty") == 0);
    Require(ttyname_r_nid_postfix(0, buffer, 0) == 34);
}

static void RequireNotTerminal(int descriptor) {
    unsigned char attributes[44];
    std::memset(attributes, 0xaa, sizeof(attributes));
    unsigned char untouched[44];
    std::memset(untouched, 0xaa, sizeof(untouched));
    *__error_nid_postfix() = 0;
    Require(Fails(tcgetattr_nid_postfix(descriptor, attributes), NotTerminal));
    Require(std::memcmp(attributes, untouched, sizeof(attributes)) == 0);
    for (const int action : {Now, Drain, Flush, Now | Soft, Flush | Soft}) {
        *__error_nid_postfix() = 0;
        Require(Fails(tcsetattr_nid_postfix(descriptor, action, attributes), NotTerminal));
    }
    RequireTerminalCalls(descriptor, NotTerminal);
}

int main() {
    const auto name = "anyps5-terminal-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const int file = open_nid_postfix(name.c_str(), 0x202, 0644);
    Require(file >= 0);
    RequireNotTerminal(file);
    RequireNotTerminal(0);
    RequireNotTerminal(1);
    RequireNotTerminal(2);

    int ends[2] = {-1, -1};
    Require(pipe_nid_postfix(ends) == 0);
    RequireNotTerminal(ends[0]);
    RequireNotTerminal(ends[1]);

    const int socket = socket_nid_postfix(2, 2, 0);
    Require(socket >= 0);
    unsigned char untouched[44];
    std::memset(untouched, 0xaa, sizeof(untouched));
    unsigned char probe[44];
    std::memset(probe, 0xaa, sizeof(probe));
    Require(Fails(tcgetattr_nid_postfix(socket, probe), AddressNotAvailable));
    Require(std::memcmp(probe, untouched, sizeof(probe)) == 0);
    Require(Fails(tcsetattr_nid_postfix(socket, Flush, probe), AddressNotAvailable));

    RequireTerminalCalls(socket, AddressNotAvailable);
    CheckAttributeHelpers();
    Require(Fails(tcflush_nid_postfix(file, 0), InvalidArgument) && Fails(tcflush_nid_postfix(-1, 4), InvalidArgument));
    Require(Fails(tcflow_nid_postfix(file, 0), InvalidArgument) && Fails(tcflow_nid_postfix(-1, 5), InvalidArgument));

    unsigned char attributes[44]{};
    Require(Fails(tcsetattr_nid_postfix(file, 3, attributes), InvalidArgument));
    Require(Fails(tcsetattr_nid_postfix(file, -1, attributes), InvalidArgument));
    Require(Fails(tcsetattr_nid_postfix(file, Soft | 3, attributes), InvalidArgument));
    Require(Fails(tcsetattr_nid_postfix(-1, 3, attributes), InvalidArgument));
    Require(Fails(tcsetattr_nid_postfix(file, Now, nullptr), BadAddress));
    Require(Fails(tcsetattr_nid_postfix(-1, Now, nullptr), BadAddress));
    Require(Fails(tcgetattr_nid_postfix(-1, attributes), BadDescriptor));
    Require(Fails(tcsetattr_nid_postfix(-1, Now, attributes), BadDescriptor));
    RequireTerminalCalls(-1, BadDescriptor);

    Require(close_nid_postfix(socket) == 0);
    Require(Fails(tcgetattr_nid_postfix(socket, attributes), BadDescriptor));
    Require(Fails(tcsetattr_nid_postfix(socket, Now, attributes), BadDescriptor));
    Require(close_nid_postfix(ends[0]) == 0 && close_nid_postfix(ends[1]) == 0);
    Require(close_nid_postfix(file) == 0);
    Require(Fails(tcgetattr_nid_postfix(file, attributes), BadDescriptor));
    Require(Fails(tcsetattr_nid_postfix(file, Flush, attributes), BadDescriptor));
    std::filesystem::remove(name);
    return 0;
}
