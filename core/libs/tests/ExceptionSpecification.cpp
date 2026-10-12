#undef NDEBUG
#include "prx/libc/include/exceptions/Runtime.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern "C" {
using Handler = void (*)();
[[noreturn]] void _ZSt10unexpectedv_nid_postfix();
Handler _ZSt14set_unexpectedPFvvE_nid_postfix(Handler);
Handler _ZSt14get_unexpectedv_nid_postfix();
Handler _ZSt13set_terminatePFvvE_nid_postfix(Handler);
extern char _ZTISt13bad_exception_nid_postfix[];
void _ZNSt13bad_exceptionC1Ev_nid_postfix(void*);
const char* _ZNKSt13bad_exception4whatEv_nid_postfix(const void*);
void* __cxa_current_primary_exception_nid_postfix();
void __cxa_rethrow_primary_exception_nid_postfix(void*);
void __cxa_decrement_exception_refcount_nid_postfix(void*);
}

namespace {

using LibcUnwind::Byte;
using LibcUnwind::Word;

void Require(bool value) { if (!value) std::abort(); }

const std::type_info* BadException() { return reinterpret_cast<const std::type_info*>(_ZTISt13bad_exception_nid_postfix); }

void PutPointer(std::vector<Byte>& bytes, const void* pointer) {
    Byte raw[sizeof(pointer)];
    std::memcpy(raw, &pointer, sizeof(pointer));
    bytes.insert(bytes.end(), raw, raw + sizeof(raw));
}

struct Specification {
    std::vector<Byte> lsda;

    Specification(std::initializer_list<const std::type_info*> allowed) {
        std::vector<Byte> body;
        for (auto type = std::rbegin(allowed); type != std::rend(allowed); ++type) PutPointer(body, *type);
        const std::size_t tableSize = body.size();
        for (std::size_t index = 1; index <= allowed.size(); ++index) body.push_back(static_cast<Byte>(index));
        body.push_back(0);
        lsda = {0xff, 0x00, static_cast<Byte>(tableSize)};
        lsda.insert(lsda.end(), body.begin(), body.end());
    }
};

std::vector<Byte> PersonalityTable(const Specification& specification) {
    constexpr Byte actions[] = {0x7f, 0x00};
    std::vector<Byte> table {0xff, 0x00};
    const std::size_t tableSize = specification.lsda[2];
    constexpr Byte callSites[] = {0x01, 0x04, 0x10, 0x20, 0x40, 0x01};
    table.push_back(static_cast<Byte>(sizeof(callSites) + sizeof(actions) + tableSize));
    table.insert(table.end(), std::begin(callSites), std::end(callSites));
    table.insert(table.end(), std::begin(actions), std::end(actions));
    table.insert(table.end(), specification.lsda.begin() + 3, specification.lsda.end());
    return table;
}

struct NativeException {
    LibcException::Header header;
    long value;
    NativeException(const std::type_info* type) {
        header.type = const_cast<std::type_info*>(type);
        header.unwind.exception_class = LibcException::PrimaryClass;
    }
};

struct Frame {
    _Unwind_Context context {};
    explicit Frame(const std::vector<Byte>& table) {
        context.region = 0x10000;
        context.lsda = reinterpret_cast<std::uintptr_t>(table.data());
        context.registers[16] = context.region + 0x18;
    }
};

_Unwind_Reason_Code Personality(Frame& frame, _Unwind_Action actions, std::uint64_t exceptionClass, _Unwind_Exception* exception) {
    return __gxx_personality_v0_nid_postfix(1, actions, exceptionClass, exception, &frame.context);
}

void CheckPersonality() {
    const std::type_info* longType = &typeid(long);
    Specification onlyInt {&typeid(int)};
    auto table = PersonalityTable(onlyInt);
    {
        NativeException allowed(&typeid(int));
        Frame frame(table);
        Require(Personality(frame, _UA_SEARCH_PHASE, LibcException::PrimaryClass, &allowed.header.unwind) == _URC_CONTINUE_UNWIND);
    }
    {
        NativeException violating(longType);
        Frame search(table);
        Require(Personality(search, _UA_SEARCH_PHASE, LibcException::PrimaryClass, &violating.header.unwind) == _URC_HANDLER_FOUND);
        Require(violating.header.selector == -1);
        Require(violating.header.lsda == reinterpret_cast<const Byte*>(search.context.lsda));
        Require(violating.header.adjusted == &violating.value);
        Frame cleanup(table);
        Require(Personality(cleanup, _UA_CLEANUP_PHASE | _UA_HANDLER_FRAME, LibcException::PrimaryClass, &violating.header.unwind) == _URC_INSTALL_CONTEXT);
        Require(cleanup.context.registers[0] == reinterpret_cast<Word>(&violating.header.unwind));
        Require(cleanup.context.registers[1] == static_cast<Word>(-1));
        Require(cleanup.context.registers[16] == cleanup.context.region + 0x40);
    }
    {
        NativeException violating(longType);
        Frame frame(table);
        Require(Personality(frame, _UA_CLEANUP_PHASE | _UA_FORCE_UNWIND, LibcException::PrimaryClass, &violating.header.unwind) == _URC_CONTINUE_UNWIND);
        Require(Personality(frame, _UA_SEARCH_PHASE | _UA_FORCE_UNWIND, LibcException::PrimaryClass, &violating.header.unwind) == _URC_CONTINUE_UNWIND);
    }
    {
        _Unwind_Exception foreign {};
        foreign.exception_class = 0x464f524549474e00;
        Frame frame(table);
        Require(Personality(frame, _UA_SEARCH_PHASE, foreign.exception_class, &foreign) == _URC_HANDLER_FOUND);
    }
    {
        Specification none {};
        auto emptyTable = PersonalityTable(none);
        NativeException violating(&typeid(int));
        Frame frame(emptyTable);
        Require(Personality(frame, _UA_SEARCH_PHASE, LibcException::PrimaryClass, &violating.header.unwind) == _URC_HANDLER_FOUND);
    }
}

void ThrowInt() { throw 7; }
void ThrowDouble() { throw 1.5; }
void Rethrow() { throw; }

struct Caught {
    const std::type_info* type = nullptr;
    long value = 0;
};

template<class Raise>
Caught CallUnexpected(const Specification& specification, Raise handler, Handler terminate = nullptr) {
    static std::vector<Byte> lsda;
    lsda = specification.lsda;
    _ZSt14set_unexpectedPFvvE_nid_postfix(handler);
    if (terminate) _ZSt13set_terminatePFvvE_nid_postfix(terminate);
    Caught caught;
    try {
        try { throw 5L; }
        catch (...) {
            auto* header = LibcException::globals.caught;
            header->selector = -1;
            header->lsda = lsda.data();
            header->dataBase = 0;
            __cxa_call_unexpected_nid_postfix(&header->unwind);
        }
    } catch (int value) { caught = {&typeid(int), value}; }
    catch (double value) { caught = {&typeid(double), static_cast<long>(value * 2)}; }
    catch (const std::exception& error) {
        caught.type = &typeid(error);
        caught.value = std::strcmp(error.what(), "std::bad_exception") == 0 ? 1 : 0;
    }
    return caught;
}

bool Child(void (*function)(), int code) {
    pid_t child = fork();
    Require(child >= 0);
    if (child == 0) {
        function();
        _exit(120);
    }
    int status;
    Require(waitpid(child, &status, 0) == child);
    if (code < 0) return WIFSIGNALED(status) && WTERMSIG(status) == -code;
    return WIFEXITED(status) && WEXITSTATUS(status) == code;
}

void CheckHandlers() {
    Handler previous = _ZSt14get_unexpectedv_nid_postfix();
    Require(previous != nullptr);
    Require(_ZSt14set_unexpectedPFvvE_nid_postfix(ThrowInt) == previous);
    Require(_ZSt14get_unexpectedv_nid_postfix() == ThrowInt);
    try { _ZSt10unexpectedv_nid_postfix(); Require(false); }
    catch (int value) { Require(value == 7); }
    Require(_ZSt14set_unexpectedPFvvE_nid_postfix(nullptr) == ThrowInt);
    Require(_ZSt14get_unexpectedv_nid_postfix() == previous);
    Require(Child([] { _ZSt13set_terminatePFvvE_nid_postfix([] { _exit(61); }); _ZSt10unexpectedv_nid_postfix(); }, 61));
    Require(Child([] { _ZSt14set_unexpectedPFvvE_nid_postfix([] {}); _ZSt10unexpectedv_nid_postfix(); }, -SIGABRT));
}

void CheckBadException() {
    alignas(void*) char object[2 * sizeof(void*)] {};
    _ZNSt13bad_exceptionC1Ev_nid_postfix(object);
    Require(std::strcmp(_ZNKSt13bad_exception4whatEv_nid_postfix(object), "std::bad_exception") == 0);
    Require(std::strcmp(BadException()->name(), "St13bad_exception") == 0);
}

void CheckCallUnexpected() {
    Specification intOnly {&typeid(int)};
    Specification intOrBad {&typeid(int), BadException()};

    auto allowed = CallUnexpected(intOnly, ThrowInt);
    Require(allowed.type == &typeid(int) && allowed.value == 7);

    auto replaced = CallUnexpected(intOrBad, ThrowDouble);
    Require(replaced.type != nullptr && replaced.value == 1);

    auto rethrown = CallUnexpected(intOrBad, Rethrow);
    Require(rethrown.type != nullptr && rethrown.value == 1);

    auto listed = CallUnexpected(Specification {&typeid(double), BadException()}, ThrowDouble);
    Require(listed.type == &typeid(double) && listed.value == 3);

    Require(Child([] {
        Specification none {};
        CallUnexpected(none, ThrowDouble, [] { _exit(61); });
    }, 61));
    Require(Child([] {
        Specification onlyInt {&typeid(int)};
        CallUnexpected(onlyInt, ThrowDouble, [] { _exit(62); });
    }, 62));
    Require(Child([] {
        Specification onlyInt {&typeid(int)};
        CallUnexpected(onlyInt, Rethrow, [] { _exit(63); });
    }, 63));
    Require(Child([] {
        Specification onlyInt {&typeid(int)};
        CallUnexpected(onlyInt, [] {}, [] { _exit(64); });
    }, -SIGABRT));
    Require(Child([] {
        Specification onlyInt {&typeid(int)};
        Caught caught = CallUnexpected(onlyInt, ThrowInt);
        _exit(caught.value == 7 ? 65 : 66);
    }, 65));
}

void CheckRethrownPrimary() {
    static std::vector<Byte> lsda;
    lsda = Specification {&typeid(int)}.lsda;
    _ZSt14set_unexpectedPFvvE_nid_postfix(ThrowInt);
    void* primary = nullptr;
    try { throw 1.5; } catch (...) { primary = __cxa_current_primary_exception_nid_postfix(); }
    Require(primary != nullptr);
    long caught = 0;
    try {
        try { __cxa_rethrow_primary_exception_nid_postfix(primary); }
        catch (...) {
            auto* header = LibcException::globals.caught;
            Require(header->unexpected == ThrowInt);
            header->selector = -1;
            header->lsda = lsda.data();
            header->dataBase = 0;
            __cxa_call_unexpected_nid_postfix(&header->unwind);
        }
    } catch (int value) { caught = value; }
    Require(caught == 7);
    __cxa_decrement_exception_refcount_nid_postfix(primary);
}

}

int main() {
    CheckPersonality();
    CheckHandlers();
    CheckBadException();
    CheckCallUnexpected();
    CheckRethrownPrimary();
    std::puts("exception specification tests passed");
    return 0;
}
