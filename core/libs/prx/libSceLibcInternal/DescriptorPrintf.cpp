#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {
int APS5_VABI vsnprintf_nid_postfix(char* buffer, std::size_t size, const char* format, VaList* arguments);
std::int64_t APS5_VABI _write_nid_postfix(int descriptor, const void* buffer, std::size_t count);
}

#ifdef _WIN32
#define APS5_GUEST_VA_LIST __builtin_sysv_va_list
#define APS5_GUEST_VA_START __builtin_sysv_va_start
#define APS5_GUEST_VA_END __builtin_sysv_va_end
#else
#define APS5_GUEST_VA_LIST std::va_list
#define APS5_GUEST_VA_START va_start
#define APS5_GUEST_VA_END va_end
#endif

extern "C" {

int APS5_VABI vdprintf_nid_postfix(int descriptor, const char* format, VaList* arguments) {
    if (format == nullptr) throw std::invalid_argument("vdprintf: null format");
    char small[256];
    int length = vsnprintf_nid_postfix(small, sizeof(small), format, arguments);
    if (length < 0) return -1;
    std::vector<char> large;
    const char* text = small;
    if (static_cast<std::size_t>(length) >= sizeof(small)) {
        large.resize(static_cast<std::size_t>(length) + 1);
        length = vsnprintf_nid_postfix(large.data(), large.size(), format, arguments);
        if (length < 0) return -1;
        text = large.data();
    }
    std::size_t written = 0;
    while (written < static_cast<std::size_t>(length)) {
        const std::int64_t result = _write_nid_postfix(descriptor, text + written, static_cast<std::size_t>(length) - written);
        if (result < 0) return -1;
        if (result == 0) break;
        written += static_cast<std::size_t>(result);
    }
    return static_cast<int>(written);
}

int APS5_VABI dprintf_nid_postfix(int descriptor, const char* format, ...) {
    APS5_GUEST_VA_LIST arguments;
    APS5_GUEST_VA_START(arguments, format);
    const int result = vdprintf_nid_postfix(descriptor, format, reinterpret_cast<VaList*>(arguments));
    APS5_GUEST_VA_END(arguments);
    return result;
}

}
