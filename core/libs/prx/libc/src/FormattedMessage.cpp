#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "prx/libc/include/General.hpp"

extern "C" char* APS5_VABI getenv_nid_postfix(const char* name);

namespace {

constexpr long PrintFlag = 0x0100;
constexpr long ConsoleFlag = 0x0200;
constexpr int Halt = 1;
constexpr int Error = 2;
constexpr int Warning = 3;
constexpr int Information = 4;
constexpr int Success = 0;
constexpr int NoConsole = 2;
constexpr const char* DefaultVerbs = "label:severity:text:action:tag";

const char* Severity(int severity) {
    switch (severity) {
    case Halt: return "HALT";
    case Error: return "ERROR";
    case Warning: return "WARNING";
    case Information: return "INFO";
    default: return nullptr;
    }
}

std::vector<std::string> Components(const std::string& verbs) {
    std::vector<std::string> components;
    std::size_t start = 0;
    while (start <= verbs.size()) {
        const std::size_t end = verbs.find(':', start);
        const std::string component = verbs.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!component.empty()) components.push_back(component);
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return components;
}

bool ValidVerbs(const std::string& verbs) {
    for (const auto& component : Components(verbs)) {
        if (component != "label" && component != "severity" && component != "text" && component != "action" && component != "tag")
            return false;
    }
    return true;
}

std::string Format(const std::string& verbs, const char* label, int severity, const char* text, const char* action, const char* tag) {
    const char* name = Severity(severity);
    std::size_t size = 32;
    for (const char* part : {label, name, text, action, tag})
        if (part != nullptr) size += std::strlen(part);
    std::string output;
    const auto append = [&](const char* part) {
        if (output.size() + 1 < size) output.append(part, std::min(std::strlen(part), size - 1 - output.size()));
    };
    const auto separate = [&](const char* separator) {
        if (!output.empty()) append(separator);
    };
    for (const auto& component : Components(verbs)) {
        if (component == "label" && label != nullptr) {
            separate(": ");
            append(label);
        } else if (component == "severity" && name != nullptr) {
            separate(": ");
            append(name);
        } else if (component == "text" && text != nullptr) {
            separate(": ");
            append(text);
        } else if (component == "action" && action != nullptr) {
            separate("\n");
            append("TO FIX: ");
            append(action);
        } else if (component == "tag" && tag != nullptr) {
            separate(" ");
            append(tag);
        }
    }
    separate("\n");
    return output;
}

}

extern "C" {

int APS5_VABI fmtmsg_nid_postfix(long classification, const char* label, int severity, const char* text, const char* action, const char* tag) {
    if (classification & PrintFlag) {
        std::string verbs = DefaultVerbs;
        const char* environment = getenv_nid_postfix("MSGVERB");
        if (environment != nullptr && *environment != '\0' && std::strlen(environment) <= std::strlen(DefaultVerbs) && ValidVerbs(environment))
            verbs = environment;
        const std::string output = Format(verbs, label, severity, text, action, tag);
        if (!output.empty()) {
            std::fputs(output.c_str(), stderr);
            std::fflush(stderr);
        }
    }
    if ((classification & ConsoleFlag) && !Format(DefaultVerbs, label, severity, text, action, tag).empty()) return NoConsole;
    return Success;
}

}
