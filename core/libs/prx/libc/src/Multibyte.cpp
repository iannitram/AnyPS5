#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/SonyCrt.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <climits>

extern "C" {

int __mb_cur_max_nid_postfix = 1;

int APS5_VABI ___mb_cur_max_nid_postfix() {
    return 1;
}

int APS5_VABI _Getmbcurmax_nid_postfix() { return ___mb_cur_max_nid_postfix(); }

int APS5_VABI mbsinit_nid_postfix(const void*) {
    return 1;
}

std::size_t APS5_VABI mbrtowc_nid_postfix(std::uint16_t* destination, const char* source, std::size_t count, void*) {
    if (!source) return 0;
    if (!count) return static_cast<std::size_t>(-2);
    const auto value = static_cast<unsigned char>(*source);
    if (destination) *destination = value;
    return value != 0;
}

int APS5_VABI mblen_nid_postfix(const char* source, std::size_t count) {
    if (source == nullptr) return 0;
    const std::size_t result = mbrtowc_nid_postfix(nullptr, source, count, nullptr);
    return result == static_cast<std::size_t>(-1) || result == static_cast<std::size_t>(-2) ? -1 : static_cast<int>(result);
}

std::size_t APS5_VABI mbrtoc32_nid_postfix(std::uint32_t* destination, const char* source, std::size_t count, void*) {
    if (!source) return 0;
    if (!count) return static_cast<std::size_t>(-2);
    const auto value = static_cast<unsigned char>(*source);
    if (destination) *destination = value;
    return value != 0;
}

std::size_t APS5_VABI c32rtomb_nid_postfix(char* destination, std::uint32_t value, void*) {
    if (!destination) return 1;
    if (value > 255) {
        errno = 86;
        return static_cast<std::size_t>(-1);
    }
    *destination = static_cast<char>(value);
    return 1;
}

// FreeBSD 11 keeps the pending surrogate in the first bytes of the caller's mbstate_t, or in
// a private state when it is null.
std::size_t APS5_VABI mbrtoc16_nid_postfix(std::uint16_t* destination, const char* source, std::size_t count, void* state) {
    static std::uint16_t privateState[2];
    auto* trail = state != nullptr ? static_cast<std::uint16_t*>(state) : privateState;
    if (*trail >= 0xdc00 && *trail <= 0xdfff) {
        if (destination) *destination = *trail;
        *trail = 0;
        return static_cast<std::size_t>(-3);
    }
    std::uint32_t value = 0;
    const std::size_t result = mbrtoc32_nid_postfix(&value, source, count, nullptr);
    if (result <= 4) {
        *trail = 0;
        if (destination) *destination = static_cast<std::uint16_t>(value);
    }
    return result;
}

std::size_t APS5_VABI c16rtomb_nid_postfix(char* destination, std::uint16_t value, void* state) {
    static std::uint16_t privateState[2];
    auto* lead = state != nullptr ? static_cast<std::uint16_t*>(state) : privateState;
    std::uint32_t full;
    if (destination == nullptr) {
        full = 0;
    } else if (*lead != 0) {
        if (value < 0xdc00 || value > 0xdfff) {
            errno = 86;
            return static_cast<std::size_t>(-1);
        }
        full = 0x10000 + (((*lead & 0x3ffu) << 10) | (value & 0x3ffu));
    } else if (value >= 0xd800 && value <= 0xdbff) {
        *lead = value;
        return 0;
    } else {
        full = value;
    }
    *lead = 0;
    return c32rtomb_nid_postfix(destination, full, nullptr);
}

std::size_t APS5_VABI mbrlen_nid_postfix(const char* source, std::size_t count, void* state) {
    return mbrtowc_nid_postfix(nullptr, source, count, state);
}

int APS5_VABI mbtowc_nid_postfix(std::uint16_t* destination, const char* source, std::size_t count) {
    const auto result = mbrtowc_nid_postfix(destination, source, count, nullptr);
    if (result == static_cast<std::size_t>(-2)) {
        errno = 86;
        return -1;
    }
    return static_cast<int>(result);
}

std::size_t APS5_VABI wcrtomb_nid_postfix(char* destination, std::uint16_t value, void*) {
    if (!destination) return 1;
    if (value > 255) {
        errno = 86;
        return static_cast<std::size_t>(-1);
    }
    *destination = static_cast<char>(value);
    return 1;
}

std::size_t APS5_VABI wcsrtombs_nid_postfix(char* destination, const std::uint16_t** source, std::size_t capacity, void* state) {
    const auto* wide = *source;
    std::size_t converted = 0;
    while (!destination || converted < capacity) {
        char byte;
        if (wcrtomb_nid_postfix(&byte, wide[converted], state) == static_cast<std::size_t>(-1)) {
            if (destination) *source = wide + converted;
            return static_cast<std::size_t>(-1);
        }
        if (destination) destination[converted] = byte;
        if (byte == '\0') {
            if (destination) *source = nullptr;
            return converted;
        }
        ++converted;
    }
    *source = wide + converted;
    return converted;
}

int APS5_VABI wcsrtombs_s_nid_postfix(
    std::size_t* result, char* destination, std::size_t capacity, const std::uint16_t** source, std::size_t limit,
    void* state
) {
    constexpr int GuestEinval = 22;
    constexpr int GuestErange = 34;
    constexpr int GuestEilseq = 86;
    constexpr auto Failed = static_cast<std::size_t>(-1);
    if (!result || !source || !state || !*source || (destination && (static_cast<std::int64_t>(capacity) <= 0
            || static_cast<std::int64_t>(limit) < 0))) {
        if (destination && static_cast<std::int64_t>(capacity) > 0) destination[0] = '\0';
        if (result) *result = Failed;
        return GuestEinval;
    }
    if (!destination && capacity) {
        *result = Failed;
        return GuestEinval;
    }
    const auto* wide = *source;
    char bytes[MB_LEN_MAX];
    if (!destination) {
        std::size_t total = 0;
        for (;; ++wide) {
            const auto length = wcrtomb_nid_postfix(bytes, *wide, state);
            if (length == Failed) {
                *result = Failed;
                return GuestEilseq;
            }
            if (length && bytes[length - 1] == '\0') {
                *result = total + length - 1;
                return 0;
            }
            total += length;
        }
    }
    std::size_t written = 0;
    while (limit) {
        const auto length = wcrtomb_nid_postfix(bytes, *wide, state);
        if (length == Failed) {
            destination[written] = '\0';
            *result = Failed;
            return GuestEilseq;
        }
        if (length > limit || length > capacity) {
            *source = wide;
            destination[0] = '\0';
            *result = Failed;
            return GuestErange;
        }
        std::memcpy(destination + written, bytes, length);
        written += length;
        if (length && destination[written - 1] == '\0') {
            *source = nullptr;
            *result = written - 1;
            return 0;
        }
        ++wide;
        capacity -= length;
        if (!capacity) {
            *source = wide;
            destination[0] = '\0';
            *result = Failed;
            return GuestErange;
        }
        limit -= length;
    }
    *source = wide;
    destination[written] = '\0';
    *result = written;
    return 0;
}

std::size_t APS5_VABI mbsrtowcs_nid_postfix(std::uint16_t* destination, const char** source, std::size_t capacity, void*) {
    if (!destination) return std::strlen(*source);
    std::size_t converted = 0;
    while (converted < capacity) {
        const auto value = static_cast<unsigned char>((*source)[converted]);
        destination[converted] = value;
        if (value == 0) {
            *source = nullptr;
            return converted;
        }
        ++converted;
    }
    *source += converted;
    return converted;
}

std::size_t APS5_VABI mbstowcs_nid_postfix(std::uint16_t* dest, const char* src, std::size_t size) {
    return SonyMbstowcs(dest, src, size);
}

}
