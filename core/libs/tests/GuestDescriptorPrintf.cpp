#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
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

}

int main() {
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
