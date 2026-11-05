#include "prx/libc/include/Api.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>

#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestTime.hpp"

extern "C" {
}

namespace {

using Wide = std::uint16_t;

constexpr int GuestInvalidArgument = 22;
constexpr int GuestOutOfMemory = 12;
constexpr int TransformError = 0;
constexpr int TransformLower = 1;
constexpr int TransformUpper = 2;

std::size_t Length(const Wide* text) {
    std::size_t length = 0;
    while (text[length] != 0) ++length;
    return length;
}

}

extern "C" {








}
