#include "prx/libc/include/WindowsFormatting.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>

extern "C" {
extern const char* const sys_errlist_nid_postfix[] = {
    "No error: 0", "Operation not permitted", "No such file or directory",
    "No such process", "Interrupted system call", "Input/output error",
    "Device not configured", "Argument list too long", "Exec format error",
    "Bad file descriptor", "No child processes", "Resource deadlock avoided",
    "Cannot allocate memory", "Permission denied", "Bad address",
    "Block device required", "Device busy", "File exists",
    "Cross-device link", "Operation not supported by device", "Not a directory",
    "Is a directory", "Invalid argument", "Too many open files in system",
    "Too many open files", "Inappropriate ioctl for device", "Text file busy",
    "File too large", "No space left on device", "Illegal seek",
    "Read-only file system", "Too many links", "Broken pipe",
    "Numerical argument out of domain", "Result too large", "Resource temporarily unavailable",
    "Operation now in progress", "Operation already in progress", "Socket operation on non-socket",
    "Destination address required", "Message too long", "Protocol wrong type for socket",
    "Protocol not available", "Protocol not supported", "Socket type not supported",
    "Operation not supported", "Protocol family not supported", "Address family not supported by protocol family",
    "Address already in use", "Can't assign requested address", "Network is down",
    "Network is unreachable", "Network dropped connection on reset", "Software caused connection abort",
    "Connection reset by peer", "No buffer space available", "Socket is already connected",
    "Socket is not connected", "Can't send after socket shutdown", "Too many references: can't splice",
    "Operation timed out", "Connection refused", "Too many levels of symbolic links",
    "File name too long", "Host is down", "No route to host",
    "Directory not empty", "Too many processes", "Too many users",
    "Disc quota exceeded", "Stale NFS file handle", "Too many levels of remote in path",
    "RPC struct is bad", "RPC version wrong", "RPC prog. not avail",
    "Program version wrong", "Bad procedure for program", "No locks available",
    "Function not implemented", "Inappropriate file type or format", "Authentication error",
    "Need authenticator", "Identifier removed", "No message of desired type",
    "Value too large to be stored in data type", "Operation canceled", "Illegal byte sequence",
    "Attribute not found", "Programming error", "Bad message",
    "Multihop attempted", "Link has been severed", "Protocol error",
    "Capabilities insufficient", "Not permitted in capability mode", "State not recoverable",
    "Previous owner died"
};
extern const int sys_nerr_nid_postfix = 97;

extern const char* const sys_signame_nid_postfix[] = {
    "Signal 0", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "EMT", "FPE", "KILL", "BUS", "SEGV", "SYS", "PIPE", "ALRM", "TERM",
    "URG", "STOP", "TSTP", "CONT", "CHLD", "TTIN", "TTOU", "IO", "XCPU", "XFSZ", "VTALRM", "PROF", "WINCH", "INFO", "USR1", "USR2"
};

extern const char* const sys_siglist_nid_postfix[] = {
    "Signal 0", "Hangup", "Interrupt", "Quit", "Illegal instruction", "Trace/BPT trap", "Abort trap", "EMT trap",
    "Floating point exception", "Killed", "Bus error", "Segmentation fault", "Bad system call", "Broken pipe", "Alarm clock",
    "Terminated", "Urgent I/O condition", "Suspended (signal)", "Suspended", "Continued", "Child exited", "Stopped (tty input)",
    "Stopped (tty output)", "I/O possible", "Cputime limit exceeded", "Filesize limit exceeded", "Virtual timer expired",
    "Profiling timer expired", "Window size changes", "Information request", "User defined signal 1", "User defined signal 2"
};

extern const int sys_nsig_nid_postfix = 32;
}


namespace {
// Guest FreeBSD errno numbering, not the host CRT's numbering.
constexpr const char* messages[] = {
    "No error: 0", "Operation not permitted", "No such file or directory",
    "No such process", "Interrupted system call", "Input/output error",
    "Device not configured", "Argument list too long", "Exec format error",
    "Bad file descriptor", "No child processes", "Resource deadlock avoided",
    "Cannot allocate memory", "Permission denied", "Bad address",
    "Block device required", "Device busy", "File exists",
    "Cross-device link", "Operation not supported by device", "Not a directory",
    "Is a directory", "Invalid argument", "Too many open files in system",
    "Too many open files", "Inappropriate ioctl for device", "Text file busy",
    "File too large", "No space left on device", "Illegal seek",
    "Read-only filesystem", "Too many links", "Broken pipe",
    "Numerical argument out of domain", "Result too large", "Resource temporarily unavailable",
    "Operation now in progress", "Operation already in progress", "Socket operation on non-socket",
    "Destination address required", "Message too long", "Protocol wrong type for socket",
    "Protocol not available", "Protocol not supported", "Socket type not supported",
    "Operation not supported", "Protocol family not supported", "Address family not supported by protocol family",
    "Address already in use", "Can't assign requested address", "Network is down",
    "Network is unreachable", "Network dropped connection on reset", "Software caused connection abort",
    "Connection reset by peer", "No buffer space available", "Socket is already connected",
    "Socket is not connected", "Can't send after socket shutdown", "Too many references: can't splice",
    "Operation timed out", "Connection refused", "Too many levels of symbolic links",
    "File name too long", "Host is down", "No route to host",
    "Directory not empty", "Too many processes", "Too many users",
    "Disc quota exceeded", "Stale NFS file handle", "Too many levels of remote in path",
    "RPC struct is bad", "RPC version wrong", "RPC prog. not avail",
    "Program version wrong", "Bad procedure for program", "No locks available",
    "Function not implemented", "Inappropriate file type or format", "Authentication error",
    "Need authenticator", "Identifier removed", "No message of desired type",
    "Value too large to be stored in data type", "Operation canceled", "Illegal byte sequence",
    "Attribute not found", "Programming error", "Bad message",
    "Multihop attempted", "Link has been severed", "Protocol error",
    "Capabilities insufficient", "Not permitted in capability mode", "State not recoverable",
    "Previous owner died"
};
static_assert(sizeof(messages) / sizeof(messages[0]) == 97);
static_assert(sizeof(sys_errlist_nid_postfix) / sizeof(sys_errlist_nid_postfix[0]) == 97);
static_assert(sizeof(sys_siglist_nid_postfix) / sizeof(sys_siglist_nid_postfix[0]) == 32);
bool NamedByStrerror(int error) { return error > 0 && error < 95; }

void Describe(int error, char (&text)[128]) {
    if (error == 0) std::snprintf(text, sizeof(text), "No error");
    else if (NamedByStrerror(error)) std::snprintf(text, sizeof(text), "%s", messages[error]);
    else std::snprintf(text, sizeof(text), "error_code=0x%08x", static_cast<unsigned>(error));
}
}

extern "C" {
char* APS5_VABI strerror_nid_postfix(int error) {
    thread_local char buffer[128];
    Describe(error, buffer);
    return buffer;
}

int APS5_VABI strerror_r_nid_postfix(int error, char* buffer, std::size_t length) {
    if (length < 23) return 34;
    if (!buffer) throw std::invalid_argument("strerror_r: a null buffer of 23 bytes or more, which the console writes through");
    const int saved = *__error_nid_postfix();
    char text[128];
    Describe(error, text);
    *__error_nid_postfix() = saved;
    std::strncpy(buffer, text, length - 1);
    buffer[length - 1] = '\0';
    return 0;
}

void APS5_VABI perror_nid_postfix(const char* prefix) {
    const int saved = *__error_nid_postfix();
    char message[128];
    strerror_r_nid_postfix(saved, message, sizeof(message));

    auto* stream = GetNativeStream(&_Stderr_nid_postfix);
    if (prefix && *prefix) std::fprintf(stream, "%s: %s\n", prefix, message);
    else std::fprintf(stream, "%s\n", message);
    *__error_nid_postfix() = saved;
}
void APS5_VABI psignal_nid_postfix(int signal, const char* prefix) {
    std::FILE* const stream = stderr;
    const char* text = signal >= 0 && signal < sys_nsig_nid_postfix ? sys_siglist_nid_postfix[signal] : "Unknown signal";
    if (prefix != nullptr && *prefix != '\0') {
        std::fputs(prefix, stream);
        std::fputs(": ", stream);
    }
    std::fputs(text, stream);
    std::fputs("\n", stream);
    std::fflush(stream);
}
}
