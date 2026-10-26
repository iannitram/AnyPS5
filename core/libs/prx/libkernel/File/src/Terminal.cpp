#include "prx/libkernel/include/Api.hpp"
#include "prx/libc/include/Api.hpp"
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
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef _WIN32
extern "C" _invalid_parameter_handler _set_thread_local_invalid_parameter_handler(_invalid_parameter_handler);
#endif

namespace {
struct GuestTermios {
    std::uint32_t flags[4];
    std::uint8_t controlCharacters[20];
    std::uint32_t inputSpeed;
    std::uint32_t outputSpeed;
};
static_assert(sizeof(GuestTermios) == 44 && alignof(GuestTermios) == 4);
static_assert(offsetof(GuestTermios, inputSpeed) == 36 && offsetof(GuestTermios, outputSpeed) == 40);

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

constexpr int InputQueue = 1;
constexpr int OutputQueue = 2;
constexpr int BothQueues = 3;
constexpr int OutputOff = 1;
constexpr int InputOn = 4;
constexpr int StandardStreams = 3;

int Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

#ifdef _WIN32
void IgnoreInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, std::uintptr_t) {}
HANDLE HostHandle(int descriptor) {
    const auto previous = _set_thread_local_invalid_parameter_handler(IgnoreInvalidParameter);
    const auto handle = ::_get_osfhandle(descriptor);
    _set_thread_local_invalid_parameter_handler(previous);
    return reinterpret_cast<HANDLE>(handle);
}
bool HostOpen(int descriptor) { return HostHandle(descriptor) != INVALID_HANDLE_VALUE; }
bool HostPipe(int descriptor) { return ::GetFileType(HostHandle(descriptor)) == FILE_TYPE_PIPE; }
#else
bool HostOpen(int descriptor) { return ::fcntl(descriptor, F_GETFD) != -1; }
bool HostPipe(int descriptor) {
    struct stat status{};
    return ::fstat(descriptor, &status) == 0 && S_ISFIFO(status.st_mode);
}
#endif

int TerminalError(int descriptor) {
    const int family = GuestSockets::Family(descriptor);
    if (family >= 0) return family == InternetFamily ? AddressNotAvailable : OperationNotSupported;
    if (descriptor < 0 || !HostOpen(descriptor)) return BadDescriptor;
    return NotTerminal;
}

int ProcessGroupError(int descriptor, const char* pipeMessage) {
    const int error = TerminalError(descriptor);
    if (error == NotTerminal && descriptor >= StandardStreams && HostPipe(descriptor)) throw std::runtime_error(pipeMessage);
    return error;
}
}

extern "C" {
int APS5_VABI cfsetspeed_nid_postfix(void* attributes, std::uint32_t speed) {
    if (attributes == nullptr) throw std::invalid_argument("cfsetspeed: attributes is null");
    auto* bytes = static_cast<std::byte*>(attributes);
    std::memcpy(bytes + offsetof(GuestTermios, inputSpeed), &speed, sizeof(speed));
    std::memcpy(bytes + offsetof(GuestTermios, outputSpeed), &speed, sizeof(speed));
    return 0;
}

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

int APS5_VABI tcsendbreak_nid_postfix(int descriptor, int duration) {
    static_cast<void>(duration);
    return Fail(TerminalError(descriptor));
}

int APS5_VABI tcdrain_nid_postfix(int descriptor) { return Fail(TerminalError(descriptor)); }

int APS5_VABI tcgetsid_nid_postfix(int descriptor) { return Fail(TerminalError(descriptor)); }

int APS5_VABI tcflush_nid_postfix(int descriptor, int queue) {
    if (queue != InputQueue && queue != OutputQueue && queue != BothQueues) return Fail(InvalidArgument);
    return Fail(TerminalError(descriptor));
}

int APS5_VABI tcflow_nid_postfix(int descriptor, int action) {
    if (action < OutputOff || action > InputOn) return Fail(InvalidArgument);
    return Fail(TerminalError(descriptor));
}

int APS5_VABI tcgetpgrp_nid_postfix(int descriptor) {
    return Fail(ProcessGroupError(descriptor, "tcgetpgrp: the SIGIO owner of a pipe is not supported"));
}

int APS5_VABI tcsetpgrp_nid_postfix(int descriptor, int group) {
    static_cast<void>(group);
    return Fail(ProcessGroupError(descriptor, "tcsetpgrp: the SIGIO owner of a pipe is not supported"));
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


void APS5_VABI cfmakeraw_nid_postfix(GuestTermios* attributes) {
    attributes->flags[0] &= ~(BellOnFullQueue | InputFlowControl | CheckParity | BreakInterrupt | MarkParity | StripHighBit |
                                MapNewlineToReturn | IgnoreReturn | MapReturnToNewline | OutputFlowControl | IgnoreParity);
    attributes->flags[0] |= IgnoreBreak;
    attributes->flags[1] &= ~PostProcessOutput;
    attributes->flags[3] &= ~(Echo | EchoErase | EchoKill | EchoNewline | Canonical | Signals | Extensions | NoFlush |
                                StopBackgroundOutput | PendingInput);
    attributes->flags[2] &= ~(CharacterSizeMask | EnableParity);
    attributes->flags[2] |= EightBits | EnableReceiver;
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

char* APS5_VABI ctermid_nid_postfix(char* buffer) {
    static char name[64];
    char* target = buffer != nullptr ? buffer : name;
    std::strcpy(target, "/dev/tty");
    return target;
}
}
