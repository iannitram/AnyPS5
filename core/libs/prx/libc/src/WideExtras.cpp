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
std::size_t APS5_VABI wcsrtombs_nid_postfix(char* destination, const std::uint16_t** source, std::size_t capacity, void* state);
std::size_t APS5_VABI mbsrtowcs_nid_postfix(std::uint16_t* destination, const char** source, std::size_t capacity, void* state);
std::size_t APS5_VABI wcrtomb_nid_postfix(char* destination, std::uint16_t value, void* state);
std::size_t APS5_VABI strftime_nid_postfix(char* destination, std::size_t capacity, const char* format, const GuestTm* time);
std::uint32_t APS5_VABI towlower_nid_postfix(std::uint32_t value);
std::uint32_t APS5_VABI towupper_nid_postfix(std::uint32_t value);
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
