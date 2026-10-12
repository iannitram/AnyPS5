#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

extern "C" {
std::int64_t APS5_VABI getline_nid_postfix(char**, std::size_t*, FileStream*);
std::int64_t APS5_VABI getdelim_nid_postfix(char**, std::size_t*, int, FileStream*);
int APS5_VABI feof_nid_postfix(FileStream*);
int APS5_VABI ferror_nid_postfix(FileStream*);
void APS5_VABI clearerr_nid_postfix(FileStream*);
void APS5_VABI free_nid_postfix(void*);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

std::FILE* Contents(const std::string& text) {
    std::FILE* file = std::tmpfile();
    Require(file != nullptr, "tmpfile");
    std::fwrite(text.data(), 1, text.size(), file);
    std::rewind(file);
    return file;
}

void CheckLines() {
    FileStream stream(Contents("first\nsecond line\n\nlast"));
    char* line = nullptr;
    std::size_t capacity = 999;
    Require(getline_nid_postfix(&line, &capacity, &stream) == 6 && std::strcmp(line, "first\n") == 0 && capacity == 8, "first line");
    Require(getline_nid_postfix(&line, &capacity, &stream) == 12 && std::strcmp(line, "second line\n") == 0 && capacity == 16, "growth to a power of two");
    Require(getline_nid_postfix(&line, &capacity, &stream) == 1 && std::strcmp(line, "\n") == 0 && capacity == 16, "empty line");
    Require(getline_nid_postfix(&line, &capacity, &stream) == 4 && std::strcmp(line, "last") == 0, "last line without newline");
    Require(!ferror_nid_postfix(&stream) && feof_nid_postfix(&stream), "end of file reached");
    Require(getline_nid_postfix(&line, &capacity, &stream) == -1 && line[0] == '\0' && capacity == 16, "end of file");
    Require(!ferror_nid_postfix(&stream), "end of file is not an error");
    free_nid_postfix(line);
}

void CheckLongLine() {
    const std::string text(1000, 'x');
    FileStream stream(Contents(text + "\nshort\n"));
    char* line = nullptr;
    std::size_t capacity = 0;
    Require(getline_nid_postfix(&line, &capacity, &stream) == 1001 && capacity == 1024, "long line");
    Require(std::string(line) == text + "\n", "long line contents");
    Require(getline_nid_postfix(&line, &capacity, &stream) == 6 && capacity == 1024, "capacity kept");
    free_nid_postfix(line);
}

void CheckDelimiters() {
    FileStream stream(Contents("a:b\xc3z:"));
    char* line = nullptr;
    std::size_t capacity = 0;
    Require(getdelim_nid_postfix(&line, &capacity, ':', &stream) == 2 && std::strcmp(line, "a:") == 0, "colon");
    Require(getdelim_nid_postfix(&line, &capacity, 0xc3, &stream) == 2 && std::strcmp(line, "b\xc3") == 0, "high byte as unsigned");
    Require(getdelim_nid_postfix(&line, &capacity, -1, &stream) == 2 && std::strcmp(line, "z:") == 0, "delimiter not found");
    free_nid_postfix(line);

    FileStream signedStream(Contents("p\xc3q"));
    line = nullptr;
    Require(getdelim_nid_postfix(&line, &capacity, static_cast<signed char>(0xc3), &signedStream) == 2, "negative delimiter matches its byte");
    free_nid_postfix(line);
}

void CheckEmptyAndErrors() {
    FileStream empty(Contents(""));
    char* line = nullptr;
    std::size_t capacity = 77;
    Require(getline_nid_postfix(&line, &capacity, &empty) == -1 && line != nullptr && line[0] == '\0' && capacity == 1, "immediate end of file allocates the terminator");
    Require(!ferror_nid_postfix(&empty), "no error at end of file");
    free_nid_postfix(line);

    std::size_t size = 0;
    errno = 0;
    Require(getline_nid_postfix(nullptr, &size, &empty) == -1 && errno == 22 && ferror_nid_postfix(&empty), "null line pointer");
    clearerr_nid_postfix(&empty);
    line = nullptr;
    errno = 0;
    Require(getline_nid_postfix(&line, nullptr, &empty) == -1 && errno == 22 && ferror_nid_postfix(&empty), "null capacity");

    const auto path = std::filesystem::temp_directory_path() / ("anyps5-getline-" + std::to_string(std::rand()));
    std::FILE* writeOnly = std::fopen(path.string().c_str(), "w");
    Require(writeOnly != nullptr, "write-only file");
    FileStream output(writeOnly);
    line = nullptr;
    capacity = 0;
    Require(getline_nid_postfix(&line, &capacity, &output) == -1 && ferror_nid_postfix(&output), "read error");
    output.Close();
    std::filesystem::remove(path);
}

}

int main() {
    std::array<void*, 10> api{};
    ApplicationHeapRegister_nid_no_patch(api.data());
    CheckLines();
    CheckLongLine();
    CheckDelimiters();
    CheckEmptyAndErrors();
    std::puts("guest getline tests passed");
    return 0;
}
