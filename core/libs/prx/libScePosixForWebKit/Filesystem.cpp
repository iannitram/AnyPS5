#include "prx/libc/include/GuestArena.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/Socket/include/SocketRuntime.hpp"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
extern "C" _invalid_parameter_handler _set_thread_local_invalid_parameter_handler(_invalid_parameter_handler);
#else
#include <unistd.h>
#endif

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI mkdir_nid_postfix(const char*, std::uint16_t);
}

namespace {
constexpr char Characters[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int Failure(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

#ifdef _WIN32
void IgnoreInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, std::uintptr_t) {}
#endif
}

namespace {
int Temporary(char* pattern, int suffixLength, bool directory) {
    if (!pattern) return Failure(14);
    const auto length = std::strlen(pattern);
    if (suffixLength < 0) return Failure(22);
    if (length >= 1024) return Failure(63);
    if (static_cast<std::size_t>(suffixLength) >= length || std::strchr(pattern + length - suffixLength, '/') != nullptr) return Failure(22);
    const auto end = length - static_cast<std::size_t>(suffixLength);
    auto start = end;
    while (start > 0 && pattern[start - 1] == 'X') --start;
    const GuestArena::HostWrite destination(pattern + start, end - start);
    if (!destination.Open()) return Failure(14);
    if (start != end) {
        std::random_device random;
        std::uniform_int_distribution<unsigned int> character(0, sizeof(Characters) - 2);
        for (auto position = start; position < end; ++position)
            pattern[position] = Characters[character(random)];
    }
    const std::string initial(pattern + start, end - start);
    for (;;) {
        const int result = directory ? mkdir_nid_postfix(pattern, 0700)
                                     : open_nid_postfix(pattern, SCE_KERNEL_O_RDWR | SCE_KERNEL_O_CREAT | SCE_KERNEL_O_EXCL, 0600);
        if (result >= 0 || *__error_nid_postfix() != 17) return result;
        auto position = start;
        for (; position < end; ++position) {
            const auto index = std::strchr(Characters, pattern[position]) - Characters;
            pattern[position] = Characters[(index + 1) % (sizeof(Characters) - 1)];
            if (pattern[position] != initial[position - start]) break;
        }
        if (position == end) return -1;
    }
}
}

extern "C" int APS5_VABI mkstemp_nid_postfix(char* pattern) {
    return Temporary(pattern, 0, false);
}

extern "C" int APS5_VABI mkstemps_nid_postfix(char* pattern, int suffixLength) {
    return Temporary(pattern, suffixLength, false);
}

extern "C" char* APS5_VABI mkdtemp_nid_postfix(char* pattern) {
    return Temporary(pattern, 0, true) == 0 ? pattern : nullptr;
}

extern "C" int APS5_VABI isatty_nid_postfix(int descriptor) {
    if (descriptor >= GuestSockets::FirstDescriptor) {
        Failure(GuestSocketIsOpen_nid_no_patch(descriptor) ? 25 : 9);
        return 0;
    }
#ifdef _WIN32
    const auto previous = _set_thread_local_invalid_parameter_handler(IgnoreInvalidParameter);
    const auto handle = reinterpret_cast<HANDLE>(::_get_osfhandle(descriptor));
    _set_thread_local_invalid_parameter_handler(previous);
    if (handle == INVALID_HANDLE_VALUE) {
        Failure(9);
        return 0;
    }
    DWORD mode = 0;
    if (::GetConsoleMode(handle, &mode)) return 1;
    Failure(25);
#else
    if (::isatty(descriptor)) return 1;
    Failure(errno == EBADF ? 9 : 25);
#endif
    return 0;
}
