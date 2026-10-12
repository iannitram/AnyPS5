#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/Socket/include/SocketRuntime.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

extern "C" int* APS5_VABI __error_nid_postfix();
#ifdef _WIN32
extern "C" _invalid_parameter_handler _set_thread_local_invalid_parameter_handler(_invalid_parameter_handler);
#endif

namespace {
constexpr int BadDescriptor = 9;
constexpr int BadAddress = 14;
constexpr int InvalidArgument = 22;
constexpr int NotTerminal = 25;
constexpr int OperationNotSupported = 45;
constexpr int AddressNotAvailable = 49;
constexpr int SetNow = 0;
constexpr int SetDrain = 1;
constexpr int SetFlush = 2;
constexpr int SetSoft = 0x10;
constexpr int InternetFamily = 2;
constexpr int OutOfRange = 34;

constexpr int FlushInput = 1;
constexpr int FlushOutput = 2;
constexpr int FlushBoth = 3;
constexpr int SuspendOutput = 1;
constexpr int RestartOutput = 2;
constexpr int SuspendInput = 3;
constexpr int RestartInput = 4;

constexpr std::uint32_t IgnoreBreak = 0x00000001;
constexpr std::uint32_t BreakInterrupt = 0x00000002;
constexpr std::uint32_t IgnoreParity = 0x00000004;
constexpr std::uint32_t MarkParity = 0x00000008;
constexpr std::uint32_t CheckParity = 0x00000010;
constexpr std::uint32_t StripHighBit = 0x00000020;
constexpr std::uint32_t MapNewlineToReturn = 0x00000040;
constexpr std::uint32_t IgnoreReturn = 0x00000080;
constexpr std::uint32_t MapReturnToNewline = 0x00000100;
constexpr std::uint32_t OutputFlowControl = 0x00000200;
constexpr std::uint32_t InputFlowControl = 0x00000400;
constexpr std::uint32_t BellOnFullQueue = 0x00002000;
constexpr std::uint32_t PostProcessOutput = 0x00000001;
constexpr std::uint32_t CharacterSizeMask = 0x00000300;
constexpr std::uint32_t EightBits = 0x00000300;
constexpr std::uint32_t EnableReceiver = 0x00000800;
constexpr std::uint32_t EnableParity = 0x00001000;
constexpr std::uint32_t EchoErase = 0x00000002;
constexpr std::uint32_t EchoKill = 0x00000004;
constexpr std::uint32_t Echo = 0x00000008;
constexpr std::uint32_t EchoNewline = 0x00000010;
constexpr std::uint32_t Signals = 0x00000080;
constexpr std::uint32_t Canonical = 0x00000100;
constexpr std::uint32_t Extensions = 0x00000400;
constexpr std::uint32_t StopBackgroundOutput = 0x00400000;
constexpr std::uint32_t PendingInput = 0x20000000;
constexpr std::uint32_t NoFlush = 0x80000000;
constexpr int MinimumIndex = 16;
constexpr int TimeIndex = 17;

// FreeBSD 11 struct termios.
struct GuestTermios {
    std::uint32_t inputFlags;
    std::uint32_t outputFlags;
    std::uint32_t controlFlags;
    std::uint32_t localFlags;
    std::uint8_t controlCharacters[20];
    std::uint32_t inputSpeed;
    std::uint32_t outputSpeed;
};
static_assert(sizeof(GuestTermios) == 44);

int Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

#ifdef _WIN32
void IgnoreInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, std::uintptr_t) {}
bool HostOpen(int descriptor) {
    const auto previous = _set_thread_local_invalid_parameter_handler(IgnoreInvalidParameter);
    const auto handle = ::_get_osfhandle(descriptor);
    _set_thread_local_invalid_parameter_handler(previous);
    return reinterpret_cast<HANDLE>(handle) != INVALID_HANDLE_VALUE;
}
#else
bool HostOpen(int descriptor) { return ::fcntl(descriptor, F_GETFD) != -1; }
#endif

int TerminalError(int descriptor) {
    if (descriptor >= GuestSockets::FirstDescriptor) {
        const int family = GuestSockets::Family(descriptor);
        if (family < 0) return BadDescriptor;
        return family == InternetFamily ? AddressNotAvailable : OperationNotSupported;
    }
    if (descriptor < 0 || !HostOpen(descriptor)) return BadDescriptor;
    return NotTerminal;
}
}

extern "C" {
int APS5_VABI tcgetattr_nid_postfix(int descriptor, void* attributes) {
    static_cast<void>(attributes);
    return Fail(TerminalError(descriptor));
}

int APS5_VABI tcsetattr_nid_postfix(int descriptor, int action, const void* attributes) {
    if ((action & SetSoft) != 0 && attributes == nullptr) throw std::invalid_argument("tcsetattr: attributes is null");
    const int mode = action & ~SetSoft;
    if (mode != SetNow && mode != SetDrain && mode != SetFlush) return Fail(InvalidArgument);
    if (attributes == nullptr) return Fail(BadAddress);
    return Fail(TerminalError(descriptor));
}

// The rest of FreeBSD 11's termios.c issues terminal ioctls, which fail as tcgetattr's does.
int APS5_VABI tcdrain_nid_postfix(int descriptor) { return Fail(TerminalError(descriptor)); }
int APS5_VABI tcsendbreak_nid_postfix(int descriptor, int) { return Fail(TerminalError(descriptor)); }
int APS5_VABI tcgetpgrp_nid_postfix(int descriptor) { return Fail(TerminalError(descriptor)); }
int APS5_VABI tcgetsid_nid_postfix(int descriptor) { return Fail(TerminalError(descriptor)); }
int APS5_VABI tcsetpgrp_nid_postfix(int descriptor, int) { return Fail(TerminalError(descriptor)); }

int APS5_VABI tcflush_nid_postfix(int descriptor, int queue) {
    if (queue != FlushInput && queue != FlushOutput && queue != FlushBoth) return Fail(InvalidArgument);
    return Fail(TerminalError(descriptor));
}

int APS5_VABI tcflow_nid_postfix(int descriptor, int action) {
    if (action != SuspendOutput && action != RestartOutput && action != SuspendInput && action != RestartInput) return Fail(InvalidArgument);
    return Fail(TerminalError(descriptor));
}

std::uint32_t APS5_VABI cfgetispeed_nid_postfix(const GuestTermios* attributes) { return attributes->inputSpeed; }
std::uint32_t APS5_VABI cfgetospeed_nid_postfix(const GuestTermios* attributes) { return attributes->outputSpeed; }

int APS5_VABI cfsetispeed_nid_postfix(GuestTermios* attributes, std::uint32_t speed) {
    attributes->inputSpeed = speed;
    return 0;
}

int APS5_VABI cfsetospeed_nid_postfix(GuestTermios* attributes, std::uint32_t speed) {
    attributes->outputSpeed = speed;
    return 0;
}

int APS5_VABI cfsetspeed_nid_postfix(GuestTermios* attributes, std::uint32_t speed) {
    attributes->inputSpeed = attributes->outputSpeed = speed;
    return 0;
}

void APS5_VABI cfmakeraw_nid_postfix(GuestTermios* attributes) {
    attributes->inputFlags &= ~(BellOnFullQueue | InputFlowControl | CheckParity | BreakInterrupt | MarkParity | StripHighBit |
                                MapNewlineToReturn | IgnoreReturn | MapReturnToNewline | OutputFlowControl | IgnoreParity);
    attributes->inputFlags |= IgnoreBreak;
    attributes->outputFlags &= ~PostProcessOutput;
    attributes->localFlags &= ~(Echo | EchoErase | EchoKill | EchoNewline | Canonical | Signals | Extensions | NoFlush |
                                StopBackgroundOutput | PendingInput);
    attributes->controlFlags &= ~(CharacterSizeMask | EnableParity);
    attributes->controlFlags |= EightBits | EnableReceiver;
    attributes->controlCharacters[MinimumIndex] = 1;
    attributes->controlCharacters[TimeIndex] = 0;
}

int APS5_VABI ttyname_r_nid_postfix(int descriptor, char* buffer, std::size_t length) {
    if (length < 1) return OutOfRange;
    if (buffer == nullptr) throw std::invalid_argument("ttyname_r: buffer is null");
    *buffer = '\0';
    return TerminalError(descriptor);
}

char* APS5_VABI ttyname_nid_postfix(int descriptor) {
    static char name[64];
    const int error = ttyname_r_nid_postfix(descriptor, name, sizeof(name));
    if (error != 0) {
        Fail(error);
        return nullptr;
    }
    return name;
}

// FreeBSD 11 copies /dev/tty and replaces the name only when /dev/tty is a character device.
char* APS5_VABI ctermid_nid_postfix(char* buffer) {
    static char name[64];
    char* target = buffer != nullptr ? buffer : name;
    std::strcpy(target, "/dev/tty");
    return target;
}
}
