#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"

namespace {

struct GuestInAddress {
    std::uint32_t address;
};

struct GuestIn6Address {
    std::uint8_t bytes[16];
};

struct GuestEtherAddress {
    std::uint8_t octet[6];
};

struct GuestControlMessage {
    std::uint32_t length;
    int level;
    int type;
};

struct GuestIp6Extension {
    std::uint8_t next;
    std::uint8_t length;
};

struct GuestRoutingHeader0 {
    std::uint8_t next;
    std::uint8_t length;
    std::uint8_t type;
    std::uint8_t segmentsLeft;
    std::uint32_t reserved;
};
static_assert(sizeof(GuestRoutingHeader0) == 8);

constexpr std::uint32_t AddressNone = 0xffffffff;
constexpr int ProtocolIpv6 = 41;
constexpr int HopOptions = 49;
constexpr int DestinationOptions = 50;
constexpr int Rfc2292HopOptions = 22;
constexpr int Rfc2292DestinationOptions = 23;
constexpr std::uint8_t Pad1 = 0;
constexpr std::uint8_t PadN = 1;
constexpr int RoutingType0 = 0;

constexpr std::uint32_t ControlAlign(std::uint32_t length) { return (length + 7) & ~std::uint32_t{7}; }
constexpr std::uint32_t ControlLength(std::uint32_t length) { return ControlAlign(sizeof(GuestControlMessage)) + length; }
constexpr std::uint32_t ControlSpace(std::uint32_t length) { return ControlAlign(sizeof(GuestControlMessage)) + ControlAlign(length); }

std::uint8_t* ControlData(const GuestControlMessage* message) {
    return const_cast<std::uint8_t*>(reinterpret_cast<const std::uint8_t*>(message)) + ControlAlign(sizeof(GuestControlMessage));
}

std::uint32_t Swap32(std::uint32_t value) { return __builtin_bswap32(value); }

bool IsAscii(int c) { return (c & ~0x7f) == 0; }
bool IsDigit(int c) { return c >= '0' && c <= '9'; }
bool IsLower(int c) { return c >= 'a' && c <= 'z'; }
bool IsHexDigit(int c) { return IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
bool IsSpace(int c) { return c == ' ' || (c >= '\t' && c <= '\r'); }

bool IsHopOptions(int type) { return type == HopOptions || type == Rfc2292HopOptions; }
bool IsDestinationOptions(int type) { return type == DestinationOptions || type == Rfc2292DestinationOptions; }

int OptionLength(const std::uint8_t* option, const std::uint8_t* limit) {
    int length;
    if (*option == Pad1) {
        length = 1;
    } else {
        if (option + 2 > limit) return 0;
        length = option[1] + 2;
    }
    return option + length <= limit ? length : 0;
}

void InsertPadding(std::uint8_t* p, int length) {
    switch (length) {
    case 0:
        return;
    case 1:
        p[0] = Pad1;
        return;
    default:
        p[0] = PadN;
        p[1] = static_cast<std::uint8_t>(length - 2);
        std::memset(&p[2], 0, length - 2);
        return;
    }
}

int LeadingPadding(int offset, int multiple, int plus) {
    const int remainder = offset % multiple;
    int padding = ((remainder + multiple - 1) & ~(multiple - 1)) - remainder;
    padding += plus;
    return padding % multiple;
}

bool ValidOptionMessage(const GuestControlMessage* message, std::uint8_t** limit) {
    if (message->level != ProtocolIpv6 || (!IsHopOptions(message->type) && !IsDestinationOptions(message->type))) return false;
    if (message->length < ControlSpace(sizeof(GuestIp6Extension))) return false;
    auto* extension = reinterpret_cast<GuestIp6Extension*>(ControlData(message));
    const int headerLength = (extension->length + 1) << 3;
    if (message->length < ControlSpace(headerLength)) return false;
    *limit = reinterpret_cast<std::uint8_t*>(extension) + headerLength;
    return true;
}

bool ValidRouting0(const GuestRoutingHeader0* header, int* addresses) {
    if (header->length % 2 != 0) return false;
    *addresses = header->length >> 1;
    return *addresses >= header->segmentsLeft;
}

}

extern "C" {

std::uint32_t APS5_VABI htonl_nid_postfix(std::uint32_t value) { return Swap32(value); }
std::uint32_t APS5_VABI ntohl_nid_postfix(std::uint32_t value) { return Swap32(value); }
std::uint16_t APS5_VABI htons_nid_postfix(std::uint16_t value) { return __builtin_bswap16(value); }
std::uint16_t APS5_VABI ntohs_nid_postfix(std::uint16_t value) { return __builtin_bswap16(value); }

int APS5_VABI inet_aton_nid_postfix(const char* text, GuestInAddress* address) {
    if (text == nullptr) throw std::invalid_argument("inet_aton: null string");
    std::uint8_t parts[4];
    std::uint8_t* part = parts;
    std::uint64_t value = 0;
    bool digit = false;
    char c = *text;
    for (;;) {
        if (!IsDigit(static_cast<unsigned char>(c))) return 0;
        value = 0;
        int base = 10;
        digit = false;
        if (c == '0') {
            c = *++text;
            if (c == 'x' || c == 'X') {
                base = 16;
                c = *++text;
            } else {
                base = 8;
                digit = true;
            }
        }
        for (;;) {
            if (IsAscii(c) && IsDigit(c)) {
                if (base == 8 && (c == '8' || c == '9')) return 0;
                value = value * base + (c - '0');
                c = *++text;
                digit = true;
            } else if (base == 16 && IsAscii(c) && IsHexDigit(c)) {
                value = (value << 4) | static_cast<std::uint64_t>(c + 10 - (IsLower(c) ? 'a' : 'A'));
                c = *++text;
                digit = true;
            } else {
                break;
            }
        }
        if (c != '.') break;
        if (part >= parts + 3 || value > 0xff) return 0;
        *part++ = static_cast<std::uint8_t>(value);
        c = *++text;
    }
    if (c != '\0' && (!IsAscii(c) || !IsSpace(c))) return 0;
    if (!digit) return 0;
    switch (part - parts + 1) {
    case 2:
        if (value > 0xffffff) return 0;
        value |= std::uint32_t{parts[0]} << 24;
        break;
    case 3:
        if (value > 0xffff) return 0;
        value |= (std::uint32_t{parts[0]} << 24) | (std::uint32_t{parts[1]} << 16);
        break;
    case 4:
        if (value > 0xff) return 0;
        value |= (std::uint32_t{parts[0]} << 24) | (std::uint32_t{parts[1]} << 16) | (std::uint32_t{parts[2]} << 8);
        break;
    default:
        break;
    }
    if (address != nullptr) address->address = Swap32(static_cast<std::uint32_t>(value));
    return 1;
}

std::uint32_t APS5_VABI inet_addr_nid_postfix(const char* text) {
    GuestInAddress address;
    return inet_aton_nid_postfix(text, &address) ? address.address : AddressNone;
}

std::uint32_t APS5_VABI inet_network_nid_postfix(const char* text) {
    if (text == nullptr) throw std::invalid_argument("inet_network: null string");
    std::uint32_t parts[4];
    std::uint32_t* part = parts;
    std::uint32_t value;
    for (;;) {
        value = 0;
        std::uint32_t base = 10;
        bool digit = false;
        if (*text == '0') {
            digit = true;
            base = 8;
            text++;
        }
        if (*text == 'x' || *text == 'X') {
            base = 16;
            text++;
        }
        for (char c; (c = *text) != 0;) {
            if (IsDigit(static_cast<unsigned char>(c))) {
                if (base == 8 && (c == '8' || c == '9')) return AddressNone;
                value = value * base + static_cast<std::uint32_t>(c - '0');
                text++;
                digit = true;
                continue;
            }
            if (base == 16 && IsHexDigit(static_cast<unsigned char>(c))) {
                value = (value << 4) + static_cast<std::uint32_t>(c + 10 - (IsLower(static_cast<unsigned char>(c)) ? 'a' : 'A'));
                text++;
                digit = true;
                continue;
            }
            break;
        }
        if (!digit) return AddressNone;
        if (part >= parts + 4 || value > 0xff) return AddressNone;
        if (*text != '.') break;
        *part++ = value;
        text++;
    }
    if (*text && !IsSpace(*text & 0xff)) return AddressNone;
    *part++ = value;
    const auto count = static_cast<std::uint32_t>(part - parts);
    value = 0;
    for (std::uint32_t i = 0; i < count; i++) {
        value <<= 8;
        value |= parts[i] & 0xff;
    }
    return value;
}

std::uint32_t APS5_VABI inet_lnaof_nid_postfix(GuestInAddress address) {
    const std::uint32_t value = Swap32(address.address);
    if ((value & 0x80000000) == 0) return value & 0x00ffffff;
    if ((value & 0xc0000000) == 0x80000000) return value & 0x0000ffff;
    return value & 0x000000ff;
}

std::uint32_t APS5_VABI inet_netof_nid_postfix(GuestInAddress address) {
    const std::uint32_t value = Swap32(address.address);
    if ((value & 0x80000000) == 0) return (value & 0xff000000) >> 24;
    if ((value & 0xc0000000) == 0x80000000) return (value & 0xffff0000) >> 16;
    return (value & 0xffffff00) >> 8;
}

GuestInAddress APS5_VABI inet_makeaddr_nid_postfix(std::uint32_t network, std::uint32_t host) {
    std::uint32_t value;
    if (network < 128) value = (network << 24) | (host & 0x00ffffff);
    else if (network < 65536) value = (network << 16) | (host & 0x0000ffff);
    else if (network < 16777216) value = (network << 8) | (host & 0x000000ff);
    else value = network | host;
    return {Swap32(value)};
}

char* APS5_VABI inet_ntoa_nid_postfix(GuestInAddress address) {
    static char buffer[18];
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(&address.address);
    std::snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);
    return buffer;
}

unsigned APS5_VABI inet_nsap_addr_nid_postfix(const char* text, std::uint8_t* binary, int maximum) {
    if (text == nullptr) throw std::invalid_argument("inet_nsap_addr: null string");
    auto fromHex = [](int c) { return static_cast<std::uint8_t>(c - (IsDigit(c) ? '0' : '7')); };
    auto upper = [](std::uint8_t c) { return IsLower(c) ? static_cast<std::uint8_t>(c - 'a' + 'A') : c; };
    if (text[0] != '0' || (text[1] != 'x' && text[1] != 'X')) return 0;
    text += 2;
    unsigned length = 0;
    std::uint8_t c;
    while ((c = static_cast<std::uint8_t>(*text++)) != '\0' && length < static_cast<unsigned>(maximum)) {
        if (c == '.' || c == '+' || c == '/') continue;
        if (!IsAscii(c)) return 0;
        c = upper(c);
        if (!IsHexDigit(c)) return 0;
        const std::uint8_t nibble = fromHex(c);
        c = static_cast<std::uint8_t>(*text++);
        if (c == '\0') return 0;
        c = upper(c);
        if (!IsHexDigit(c)) return 0;
        *binary++ = static_cast<std::uint8_t>((nibble << 4) | fromHex(c));
        length++;
    }
    return length;
}

char* APS5_VABI inet_nsap_ntoa_nid_postfix(int length, const std::uint8_t* binary, char* text) {
    thread_local char buffer[255 * 3];
    char* start = text != nullptr ? text : buffer;
    text = start;
    *text++ = '0';
    *text++ = 'x';
    if (length > 255) length = 255;
    for (int i = 0; i < length; i++) {
        int nibble = *binary >> 4;
        *text++ = static_cast<char>(nibble + (nibble < 10 ? '0' : '7'));
        nibble = *binary++ & 0x0f;
        *text++ = static_cast<char>(nibble + (nibble < 10 ? '0' : '7'));
        if (i % 2 == 0 && i + 1 < length) *text++ = '.';
    }
    *text = '\0';
    return start;
}

int APS5_VABI ether_line_nid_postfix(const char* line, GuestEtherAddress* address, char* hostname) {
    if (line == nullptr || address == nullptr || hostname == nullptr) throw std::invalid_argument("ether_line: null argument");
    int octets[6];
    if (std::sscanf(line, "%x:%x:%x:%x:%x:%x %s", &octets[0], &octets[1], &octets[2], &octets[3], &octets[4], &octets[5], hostname) != 7)
        return -1;
    for (int i = 0; i < 6; i++) address->octet[i] = static_cast<std::uint8_t>(octets[i]);
    return 0;
}

GuestEtherAddress* APS5_VABI ether_aton_r_nid_postfix(const char* text, GuestEtherAddress* address) {
    if (text == nullptr || address == nullptr) throw std::invalid_argument("ether_aton_r: null argument");
    unsigned octets[6];
    if (std::sscanf(text, "%x:%x:%x:%x:%x:%x", &octets[0], &octets[1], &octets[2], &octets[3], &octets[4], &octets[5]) != 6)
        return nullptr;
    for (int i = 0; i < 6; i++) address->octet[i] = static_cast<std::uint8_t>(octets[i]);
    return address;
}

GuestEtherAddress* APS5_VABI ether_aton_nid_postfix(const char* text) {
    static GuestEtherAddress address;
    return ether_aton_r_nid_postfix(text, &address);
}

char* APS5_VABI ether_ntoa_r_nid_postfix(const GuestEtherAddress* address, char* text) {
    if (address == nullptr || text == nullptr) throw std::invalid_argument("ether_ntoa_r: null argument");
    const std::uint8_t* o = address->octet;
    if (std::sprintf(text, "%02x:%02x:%02x:%02x:%02x:%02x", o[0], o[1], o[2], o[3], o[4], o[5]) < 17) return nullptr;
    return text;
}

char* APS5_VABI ether_ntoa_nid_postfix(const GuestEtherAddress* address) {
    static char text[18];
    return ether_ntoa_r_nid_postfix(address, text);
}

FileStream* APS5_VABI fopen_nid_postfix(const char* filename, const char* mode);
int APS5_VABI fclose_nid_postfix(FileStream* stream);
char* APS5_VABI fgets_nid_postfix(char* buffer, int size, FileStream* stream);
int APS5_VABI ether_line_nid_postfix(const char* line, GuestEtherAddress* address, char* hostname);

}

namespace {

template<typename Match>
int SearchEthers(Match match) {
    FileStream* file = fopen_nid_postfix("/etc/ethers", "r");
    if (file == nullptr) return 1;
    char line[1024 + 2];
    GuestEtherAddress address {};
    char host[256];
    while (fgets_nid_postfix(line, 1024, file) != nullptr) {
        if (line[0] == '#') continue;
        if (ether_line_nid_postfix(line, &address, host) == 0 && match(address, host)) {
            fclose_nid_postfix(file);
            return 0;
        }
    }
    fclose_nid_postfix(file);
    return 1;
}

}

extern "C" {

int APS5_VABI ether_ntohost_nid_postfix(char* hostname, const GuestEtherAddress* address) {
    if (hostname == nullptr || address == nullptr) throw std::invalid_argument("ether_ntohost: null argument");
    return SearchEthers([&](const GuestEtherAddress& entry, const char* host) {
        if (std::memcmp(entry.octet, address->octet, 6) != 0) return false;
        std::strcpy(hostname, host);
        return true;
    });
}

int APS5_VABI ether_hostton_nid_postfix(const char* hostname, GuestEtherAddress* address) {
    if (hostname == nullptr || address == nullptr) throw std::invalid_argument("ether_hostton: null argument");
    return SearchEthers([&](const GuestEtherAddress& entry, const char* host) {
        if (std::strcmp(hostname, host) != 0) return false;
        std::memcpy(address->octet, entry.octet, 6);
        return true;
    });
}

int APS5_VABI inet6_option_space_nid_postfix(int bytes) {
    bytes += 2;
    return static_cast<int>(ControlSpace((bytes + 7) & ~7));
}

int APS5_VABI inet6_option_init_nid_postfix(void* buffer, GuestControlMessage** message, int type) {
    if (buffer == nullptr || message == nullptr) throw std::invalid_argument("inet6_option_init: null argument");
    auto* header = static_cast<GuestControlMessage*>(buffer);
    if (!IsHopOptions(type) && !IsDestinationOptions(type)) return -1;
    header->level = ProtocolIpv6;
    header->type = type;
    header->length = ControlLength(0);
    *message = header;
    return 0;
}

std::uint8_t* APS5_VABI inet6_option_alloc_nid_postfix(GuestControlMessage* message, int dataLength, int multiple, int plus) {
    if (message == nullptr) throw std::invalid_argument("inet6_option_alloc: null message");
    if (multiple != 1 && multiple != 2 && multiple != 4 && multiple != 8) return nullptr;
    if (plus < 0 || plus > 7) return nullptr;
    std::uint8_t* p = reinterpret_cast<std::uint8_t*>(message) + message->length;
    auto* extension = reinterpret_cast<GuestIp6Extension*>(ControlData(message));
    auto* start = reinterpret_cast<std::uint8_t*>(extension);
    if (p == start) {
        p += 2;
        message->length += 2;
    }
    int padding = LeadingPadding(static_cast<int>(p - start), multiple, plus);
    InsertPadding(p, padding);
    message->length += padding;
    p += padding;
    std::uint8_t* result = p;
    p += dataLength;
    message->length += dataLength;
    const int offset = static_cast<int>(p - start);
    padding = ((offset + 7) & ~7) - offset;
    InsertPadding(p, padding);
    p += padding;
    message->length += padding;
    extension->length = static_cast<std::uint8_t>(((p - start) >> 3) - 1);
    return result;
}

int APS5_VABI inet6_option_append_nid_postfix(GuestControlMessage* message, const std::uint8_t* option, int multiple, int plus) {
    if (message == nullptr || option == nullptr) throw std::invalid_argument("inet6_option_append: null argument");
    if (multiple != 1 && multiple != 2 && multiple != 4 && multiple != 8) return -1;
    if (plus < 0 || plus > 7) return -1;
    std::uint8_t* p = reinterpret_cast<std::uint8_t*>(message) + message->length;
    auto* extension = reinterpret_cast<GuestIp6Extension*>(ControlData(message));
    auto* start = reinterpret_cast<std::uint8_t*>(extension);
    if (p == start) {
        p += 2;
        message->length += 2;
    }
    int padding = LeadingPadding(static_cast<int>(p - start), multiple, plus);
    InsertPadding(p, padding);
    message->length += padding;
    p += padding;
    const int optionLength = option[0] == Pad1 ? 1 : option[1] + 2;
    std::memcpy(p, option, optionLength);
    p += optionLength;
    message->length += optionLength;
    const int offset = static_cast<int>(p - start);
    padding = ((offset + 7) & ~7) - offset;
    InsertPadding(p, padding);
    p += padding;
    message->length += padding;
    extension->length = static_cast<std::uint8_t>(((p - start) >> 3) - 1);
    return 0;
}

int APS5_VABI inet6_option_next_nid_postfix(const GuestControlMessage* message, std::uint8_t** cursor) {
    if (message == nullptr || cursor == nullptr) throw std::invalid_argument("inet6_option_next: null argument");
    std::uint8_t* limit;
    if (!ValidOptionMessage(message, &limit)) return -1;
    if (*cursor == nullptr) {
        *cursor = ControlData(message) + sizeof(GuestIp6Extension);
    } else {
        const int length = OptionLength(*cursor, limit);
        if (length == 0) return -1;
        *cursor += length;
    }
    if (*cursor >= limit) {
        *cursor = nullptr;
        return -1;
    }
    return OptionLength(*cursor, limit) == 0 ? -1 : 0;
}

int APS5_VABI inet6_option_find_nid_postfix(const GuestControlMessage* message, std::uint8_t** cursor, int type) {
    if (message == nullptr || cursor == nullptr) throw std::invalid_argument("inet6_option_find: null argument");
    std::uint8_t* limit;
    if (!ValidOptionMessage(message, &limit)) return -1;
    if (*cursor == nullptr) {
        *cursor = ControlData(message) + sizeof(GuestIp6Extension);
    } else {
        const int length = OptionLength(*cursor, limit);
        if (length == 0) return -1;
        *cursor += length;
    }
    for (std::uint8_t* option = *cursor; option < limit;) {
        if (*option == type) {
            *cursor = option;
            return 0;
        }
        const int length = OptionLength(option, limit);
        if (length == 0) return -1;
        option += length;
    }
    *cursor = nullptr;
    return -1;
}

int APS5_VABI inet6_opt_init_nid_postfix(void* buffer, std::uint32_t length) {
    if (buffer != nullptr) {
        if (length == 0 || length % 8) return -1;
        static_cast<GuestIp6Extension*>(buffer)->length = static_cast<std::uint8_t>((length >> 3) - 1);
    }
    return 2;
}

int APS5_VABI inet6_opt_append_nid_postfix(void* buffer, std::uint32_t bufferLength, int offset, std::uint8_t type, std::uint32_t length,
                                           std::uint8_t align, void** data) {
    if (type < 2 || length > 255) return -1;
    if (align != 1 && align != 2 && align != 4 && align != 8) return -1;
    if (align > length) return -1;
    int current = offset + 2 + static_cast<int>(length);
    const int padding = current % align ? align - current % align : 0;
    current += padding;
    if (bufferLength && static_cast<std::uint32_t>(current) > bufferLength) return -1;
    if (buffer != nullptr) {
        if (data == nullptr) throw std::invalid_argument("inet6_opt_append: null data pointer");
        auto* p = static_cast<std::uint8_t*>(buffer) + offset;
        if (padding == 1) {
            *p++ = Pad1;
        } else if (padding > 0) {
            *p++ = PadN;
            *p++ = static_cast<std::uint8_t>(padding - 2);
            std::memset(p, 0, padding - 2);
            p += padding - 2;
        }
        *p++ = type;
        *p++ = static_cast<std::uint8_t>(length);
        *data = p;
    }
    return current;
}

int APS5_VABI inet6_opt_finish_nid_postfix(void* buffer, std::uint32_t bufferLength, int offset) {
    const int length = offset > 0 ? 1 + ((offset - 1) | 7) : 0;
    if (buffer != nullptr) {
        if (static_cast<std::uint32_t>(length) > bufferLength) return -1;
        auto* p = static_cast<std::uint8_t*>(buffer) + offset;
        const int padding = length - offset;
        if (padding == 1) {
            *p = Pad1;
        } else if (padding > 0) {
            *p++ = PadN;
            *p++ = static_cast<std::uint8_t>(padding - 2);
            std::memset(p, 0, padding - 2);
        }
    }
    return length;
}

int APS5_VABI inet6_opt_set_val_nid_postfix(void* data, int offset, const void* value, std::uint32_t length) {
    std::memcpy(static_cast<std::uint8_t*>(data) + offset, value, length);
    return offset + static_cast<int>(length);
}

int APS5_VABI inet6_opt_get_val_nid_postfix(const void* data, int offset, void* value, std::uint32_t length) {
    std::memcpy(value, static_cast<const std::uint8_t*>(data) + offset, length);
    return offset + static_cast<int>(length);
}

int APS5_VABI inet6_opt_next_nid_postfix(void* buffer, std::uint32_t bufferLength, int offset, std::uint8_t* type, std::uint32_t* length, void** data) {
    if (bufferLength == 0 || bufferLength % 8) return -1;
    auto* base = static_cast<std::uint8_t*>(buffer);
    const std::uint8_t* limit = base + bufferLength;
    std::uint8_t* p = offset == 0 ? base + sizeof(GuestIp6Extension) : base + offset;
    while (p < limit) {
        if (*p == Pad1) {
            p++;
            continue;
        }
        const int optionLength = OptionLength(p, limit);
        if (optionLength == 0) break;
        if (*p == PadN) {
            p += optionLength;
            continue;
        }
        *type = *p;
        *length = static_cast<std::uint32_t>(optionLength - 2);
        *data = p + 2;
        return static_cast<int>(p + optionLength - base);
    }
    *data = nullptr;
    return -1;
}

int APS5_VABI inet6_opt_find_nid_postfix(void* buffer, std::uint32_t bufferLength, int offset, std::uint8_t type, std::uint32_t* length, void** data) {
    if (bufferLength == 0 || bufferLength % 8) return -1;
    auto* base = static_cast<std::uint8_t*>(buffer);
    const std::uint8_t* limit = base + bufferLength;
    std::uint8_t* p = offset == 0 ? base + sizeof(GuestIp6Extension) : base + offset;
    while (p < limit) {
        const int optionLength = OptionLength(p, limit);
        if (optionLength == 0) break;
        if (*p == type) {
            *length = static_cast<std::uint32_t>(optionLength - 2);
            *data = p + 2;
            return static_cast<int>(p + optionLength - base);
        }
        p += optionLength;
    }
    *data = nullptr;
    return -1;
}

std::uint32_t APS5_VABI inet6_rth_space_nid_postfix(int type, int segments) {
    if (type == RoutingType0 && segments >= 0 && segments <= 127) return static_cast<std::uint32_t>((segments * 2 + 1) << 3);
    return 0;
}

void* APS5_VABI inet6_rth_init_nid_postfix(void* buffer, std::uint32_t length, int type, int segments) {
    if (type != RoutingType0) return nullptr;
    if (length < inet6_rth_space_nid_postfix(RoutingType0, segments)) return nullptr;
    if (segments < 0 || segments > 127) return nullptr;
    std::memset(buffer, 0, length);
    auto* header = static_cast<GuestRoutingHeader0*>(buffer);
    header->length = static_cast<std::uint8_t>(segments * 2);
    header->type = RoutingType0;
    header->segmentsLeft = 0;
    header->reserved = 0;
    return buffer;
}

int APS5_VABI inet6_rth_add_nid_postfix(void* buffer, const GuestIn6Address* address) {
    auto* header = static_cast<GuestRoutingHeader0*>(buffer);
    if (header->type != RoutingType0) return -1;
    if (header->segmentsLeft == header->length / 2) return -1;
    reinterpret_cast<GuestIn6Address*>(header + 1)[header->segmentsLeft] = *address;
    header->segmentsLeft++;
    return 0;
}

int APS5_VABI inet6_rth_reverse_nid_postfix(const void* in, void* out) {
    const auto* input = static_cast<const GuestRoutingHeader0*>(in);
    if (input->type != RoutingType0) return -1;
    if (input->length % 2) return -1;
    const int segments = input->length / 2;
    std::memmove(out, in, (input->length + 1) << 3);
    auto* output = static_cast<GuestRoutingHeader0*>(out);
    output->segmentsLeft = static_cast<std::uint8_t>(segments);
    auto* addresses = reinterpret_cast<GuestIn6Address*>(output + 1);
    for (int i = 0; i < segments / 2; i++) {
        const GuestIn6Address swap = addresses[i];
        addresses[i] = addresses[segments - i - 1];
        addresses[segments - i - 1] = swap;
    }
    return 0;
}

int APS5_VABI inet6_rth_segments_nid_postfix(const void* buffer) {
    const auto* header = static_cast<const GuestRoutingHeader0*>(buffer);
    int addresses;
    if (header->type != RoutingType0 || !ValidRouting0(header, &addresses)) return -1;
    return addresses;
}

GuestIn6Address* APS5_VABI inet6_rth_getaddr_nid_postfix(const void* buffer, int index) {
    const auto* header = static_cast<const GuestRoutingHeader0*>(buffer);
    int addresses;
    if (header->type != RoutingType0 || !ValidRouting0(header, &addresses)) return nullptr;
    if (index < 0 || addresses <= index) return nullptr;
    return const_cast<GuestIn6Address*>(reinterpret_cast<const GuestIn6Address*>(header + 1)) + index;
}

}
