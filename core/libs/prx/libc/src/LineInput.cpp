#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>

#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

extern "C" void* APS5_VABI realloc_nid_postfix(void* pointer, std::size_t size);

namespace {

constexpr int GuestInvalidArgument = 22;
constexpr int GuestOverflow = 84;
constexpr std::size_t SignedMaximum = static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max());

class StreamLock {
    std::FILE* handle;

public:
    explicit StreamLock(std::FILE* stream) : handle(stream) {
#ifdef _WIN32
        _lock_file(handle);
#else
        flockfile(handle);
#endif
    }
    ~StreamLock() {
#ifdef _WIN32
        _unlock_file(handle);
#else
        funlockfile(handle);
#endif
    }
    StreamLock(const StreamLock&) = delete;
    StreamLock& operator=(const StreamLock&) = delete;
};

int ReadUnlocked(std::FILE* stream) {
#ifdef _WIN32
    return _fgetc_nolock(stream);
#else
    return ::getc_unlocked(stream);
#endif
}

std::size_t RoundUpToPowerOfTwo(std::size_t n) {
    if ((n & (n - 1)) == 0) return n;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

bool ExpandToFit(char** line, std::size_t length, std::size_t* capacity) {
    if (length > SignedMaximum + 1) {
        errno = GuestOverflow;
        return false;
    }
    if (length > *capacity) {
        const std::size_t newCapacity = length == SignedMaximum + 1 ? SignedMaximum + 1 : RoundUpToPowerOfTwo(length);
        auto* grown = static_cast<char*>(realloc_nid_postfix(*line, newCapacity));
        if (grown == nullptr) return false;
        *capacity = newCapacity;
        *line = grown;
    }
    return true;
}

std::int64_t ReadDelimited(char** line, std::size_t* capacity, int delimiter, FileStream* stream) {
    std::FILE* handle = GetNativeStream(stream);
    std::int64_t result = -1;
    bool failed = false;
    {
        StreamLock lock(handle);
        if (line == nullptr || capacity == nullptr) {
            errno = GuestInvalidArgument;
            failed = true;
        } else {
            if (*line == nullptr) *capacity = 0;
            int character = ReadUnlocked(handle);
            if (character == EOF) {
                if (std::ferror(handle) || !ExpandToFit(line, 1, capacity)) {
                    failed = true;
                } else {
                    (*line)[0] = '\0';
                }
            } else {
                std::size_t length = 0;
                for (;;) {
                    if (!ExpandToFit(line, length + 2, capacity)) {
                        failed = true;
                        break;
                    }
                    (*line)[length++] = static_cast<char>(character);
                    if (static_cast<unsigned char>(character) == static_cast<unsigned char>(delimiter)) break;
                    character = ReadUnlocked(handle);
                    if (character == EOF) {
                        if (std::ferror(handle)) failed = true;
                        break;
                    }
                }
                if (!failed) {
                    (*line)[length] = '\0';
                    result = static_cast<std::int64_t>(length);
                }
            }
        }
    }
    if (failed) {
        stream->SetError();
        return -1;
    }
    stream->SyncStatus();
    return result;
}

}

extern "C" {

std::int64_t APS5_VABI getdelim_nid_postfix(char** line, std::size_t* capacity, int delimiter, FileStream* stream) {
    if (stream == nullptr) throw std::invalid_argument("getdelim: null stream");
    return ReadDelimited(line, capacity, delimiter, stream);
}

std::int64_t APS5_VABI getline_nid_postfix(char** line, std::size_t* capacity, FileStream* stream) {
    if (stream == nullptr) throw std::invalid_argument("getline: null stream");
    return ReadDelimited(line, capacity, '\n', stream);
}

}
