#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

extern "C" {
int APS5_VABI fmtmsg_nid_postfix(long, const char*, int, const char*, const char*, const char*);
int APS5_VABI setenv_nid_postfix(const char*, const char*, int);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stdout, "failed: %s\n", what);
        std::abort();
    }
}

}

int main() {
    const std::string name = "anyps5-fmtmsg-" + std::to_string(std::rand());
    Require(std::freopen(name.c_str(), "w", stderr) != nullptr, "redirect stderr");
    Require(fmtmsg_nid_postfix(0x0100 | 0x0010, "UX:cat", 2, "invalid syntax", "refer to manual", "UX:cat:001") == 0, "full message");
    Require(fmtmsg_nid_postfix(0x0100, nullptr, 0, "text only", nullptr, nullptr) == 0, "text only");
    Require(fmtmsg_nid_postfix(0x0100, nullptr, 0, nullptr, nullptr, nullptr) == 0, "empty message");
    Require(setenv_nid_postfix("MSGVERB", "severity:text", 1) == 0, "MSGVERB");
    Require(fmtmsg_nid_postfix(0x0100, "UX:cat", 4, "subset", "act", "tag") == 0, "MSGVERB subset");
    Require(setenv_nid_postfix("MSGVERB", "bogus", 1) == 0, "invalid MSGVERB");
    Require(fmtmsg_nid_postfix(0x0100, "L", 3, "back to default", nullptr, nullptr) == 0, "invalid MSGVERB uses the default");
    Require(fmtmsg_nid_postfix(0x0200, "L", 1, "console", nullptr, nullptr) == 2, "no console");
    Require(fmtmsg_nid_postfix(0x0200, nullptr, 0, nullptr, nullptr, nullptr) == 0, "nothing for the console");
    std::fclose(stderr);
    std::ifstream file(name);
    std::stringstream text;
    text << file.rdbuf();
    file.close();
    std::filesystem::remove(name);
    const std::string expected =
        "UX:cat: ERROR: invalid syntax\nTO FIX: refer to manual UX:cat:001\n"
        "text only\n"
        "INFO: subset\n"
        "L: WARNING: back to default\n";
    if (text.str() != expected) std::fprintf(stdout, "%s", text.str().c_str());
    Require(text.str() == expected, "output");
    std::puts("guest formatted message tests passed");
    return 0;
}
