#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

extern "C" {
int APS5_VABI dprintf_nid_postfix(int, const char*, ...);
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI close_nid_postfix(int);
int* APS5_VABI __error_nid_postfix();
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s (errno %d)\n", what, *__error_nid_postfix());
        std::abort();
    }
}

void* APS5_VABI Allocate(std::size_t size) { return std::malloc(size); }
void APS5_VABI Free(void* ptr) { std::free(ptr); }
void* APS5_VABI Reallocate(void* ptr, std::size_t size) { return std::realloc(ptr, size); }
void* APS5_VABI Calloc(std::size_t count, std::size_t size) { return std::calloc(count, size); }
void* APS5_VABI Align(std::size_t, std::size_t size) { return std::malloc(size); }
void* APS5_VABI Realign(void* ptr, std::size_t size, std::size_t) { return std::realloc(ptr, size); }
int APS5_VABI PosixAlign(void** ptr, std::size_t, std::size_t size) {
    *ptr = std::malloc(size);
    return *ptr ? 0 : 12;
}

void InitializeHeap() {
    alignas(16) static std::array<std::byte, 0x40> process{};
    alignas(16) static std::array<std::byte, 0x38> libc{};
    alignas(16) static std::array<std::byte, 0x78> replacement{};
    const auto write = [](auto& data, std::size_t offset, auto value) {
        std::memcpy(data.data() + offset, &value, sizeof(value));
    };
    write(process, 0, std::uint64_t{0x40});
    write(process, 8, std::uint32_t{0x4942524f});
    write(process, 0x38, libc.data());
    write(libc, 0, std::uint64_t{0x38});
    write(libc, 0x30, replacement.data());
    write(replacement, 0, std::uint64_t{0x78});
    write(replacement, 8, std::uint64_t{2});
    write(replacement, 0x20, reinterpret_cast<void*>(&Allocate));
    write(replacement, 0x28, reinterpret_cast<void*>(&Free));
    write(replacement, 0x30, reinterpret_cast<void*>(&Calloc));
    write(replacement, 0x38, reinterpret_cast<void*>(&Reallocate));
    write(replacement, 0x40, reinterpret_cast<void*>(&Align));
    write(replacement, 0x48, reinterpret_cast<void*>(&Realign));
    write(replacement, 0x50, reinterpret_cast<void*>(&PosixAlign));
    ApplicationHeapInitialize_nid_no_patch(process.data());
}

}

int main() {
    InitializeHeap();
    const auto name = "anyps5-dprintf-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const int descriptor = open_nid_postfix(name.c_str(), 0x0202, 0644);
    Require(descriptor >= 0, "open");
    Require(dprintf_nid_postfix(descriptor, "%d %s %.2f\n", 42, "text", 1.5) == 13, "dprintf");
    const std::string longText(1000, 'z');
    Require(dprintf_nid_postfix(descriptor, "<%s>", longText.c_str()) == 1002, "long output");
    Require(dprintf_nid_postfix(descriptor, "") == 0, "empty output");
    Require(close_nid_postfix(descriptor) == 0, "close");
    std::ifstream file(name, std::ios::binary);
    std::stringstream contents;
    contents << file.rdbuf();
    Require(contents.str() == "42 text 1.50\n<" + longText + ">", "file contents");
    file.close();
    std::filesystem::remove(name);
    *__error_nid_postfix() = 0;
    Require(dprintf_nid_postfix(descriptor, "closed") == -1 && *__error_nid_postfix() == 9, "closed descriptor");
    std::puts("guest descriptor printf tests passed");
    return 0;
}
