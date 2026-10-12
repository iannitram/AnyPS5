#undef NDEBUG
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <typeinfo>
#include <cxxabi.h>

extern "C" {
void* __cxa_allocate_exception_nid_postfix(std::size_t);
[[noreturn]] void __cxa_throw_nid_postfix(void*, std::type_info*, void (*)(void*));
extern char _ZTVN10__cxxabiv119__pointer_type_infoE_nid_postfix[];
extern char _ZTVN10__cxxabiv120__function_type_infoE_nid_postfix[];
extern char _ZTVN10__cxxabiv123__fundamental_type_infoE_nid_postfix[];
extern char _ZTIN10__cxxabiv119__pointer_type_infoE_nid_postfix[];
extern char _ZTIN10__cxxabiv120__function_type_infoE_nid_postfix[];
}

#define FUNDAMENTAL_TYPES(X) X(v) X(Dn) X(b) X(w) X(c) X(a) X(h) X(s) X(t) X(i) X(j) X(l) X(m) X(x) X(y) X(n) X(o) X(f) X(d) X(e) X(g) X(Dh) X(Di) X(Ds) X(Du)
#define DECLARE(code) \
    extern "C" char _ZTI##code##_nid_postfix[]; \
    extern "C" char _ZTIP##code##_nid_postfix[]; \
    extern "C" char _ZTIPK##code##_nid_postfix[];
FUNDAMENTAL_TYPES(DECLARE)

namespace {

struct Pointer {
    const void* vtable;
    const char* name;
    unsigned flags;
    const void* pointee;
};
struct Fundamental {
    const void* vtable;
    const char* name;
};

template<class T> std::type_info* Info(T& record) { return reinterpret_cast<std::type_info*>(&record); }

void CheckLayout() {
    const void* const* fundamental = reinterpret_cast<const void* const*>(_ZTVN10__cxxabiv123__fundamental_type_infoE_nid_postfix) + 2;
    const void* const* pointer = reinterpret_cast<const void* const*>(_ZTVN10__cxxabiv119__pointer_type_infoE_nid_postfix) + 2;
    int count = 0;
#define CHECK(code) { \
        auto* plain = reinterpret_cast<Fundamental*>(_ZTI##code##_nid_postfix); \
        auto* pointed = reinterpret_cast<Pointer*>(_ZTIP##code##_nid_postfix); \
        auto* constant = reinterpret_cast<Pointer*>(_ZTIPK##code##_nid_postfix); \
        assert(plain->vtable == fundamental && std::strcmp(plain->name, #code) == 0); \
        assert(pointed->vtable == pointer && std::strcmp(pointed->name, "P" #code) == 0); \
        assert(pointed->flags == 0 && pointed->pointee == plain); \
        assert(constant->vtable == pointer && std::strcmp(constant->name, "PK" #code) == 0); \
        assert(constant->flags == 1 && constant->pointee == plain); \
        ++count; }
    FUNDAMENTAL_TYPES(CHECK)
    assert(count == 25);
    assert(reinterpret_cast<const void* const*>(_ZTVN10__cxxabiv119__pointer_type_infoE_nid_postfix)[1] == _ZTIN10__cxxabiv119__pointer_type_infoE_nid_postfix);
    assert(reinterpret_cast<const void* const*>(_ZTVN10__cxxabiv120__function_type_infoE_nid_postfix)[1] == _ZTIN10__cxxabiv120__function_type_infoE_nid_postfix);
}

void CheckHostAgreement() {
    assert(std::strcmp(typeid(int).name(), reinterpret_cast<Fundamental*>(_ZTIi_nid_postfix)->name) == 0);
    assert(std::strcmp(typeid(const char*).name(), reinterpret_cast<Pointer*>(_ZTIPKc_nid_postfix)->name) == 0);
    assert(std::strcmp(typeid(void*).name(), reinterpret_cast<Pointer*>(_ZTIPv_nid_postfix)->name) == 0);
    assert(std::strcmp(typeid(unsigned long long).name(), reinterpret_cast<Fundamental*>(_ZTIy_nid_postfix)->name) == 0);
    assert(std::strcmp(typeid(std::nullptr_t).name(), reinterpret_cast<Fundamental*>(_ZTIDn_nid_postfix)->name) == 0);
    assert(std::strcmp(typeid(char32_t).name(), reinterpret_cast<Fundamental*>(_ZTIDi_nid_postfix)->name) == 0);
}

int destroyed = 0;

}

#define THROW_AS(record, value) \
    try { \
        auto* object = static_cast<decltype(value)*>(__cxa_allocate_exception_nid_postfix(sizeof(value))); \
        *object = value; \
        __cxa_throw_nid_postfix(object, Info(record), [](void*) { ++destroyed; }); \
    } catch

int main() {
    CheckLayout();
    CheckHostAgreement();
    char text[] = "text";
    int number = 4;
    {
        bool caught = false;
        THROW_AS(*reinterpret_cast<Pointer*>(_ZTIPc_nid_postfix), static_cast<char*>(text)) (const char* value) { assert(value == text); caught = true; }
        assert(caught);
    }
    {
        bool caught = false;
        THROW_AS(*reinterpret_cast<Pointer*>(_ZTIPKc_nid_postfix), static_cast<const char*>(text)) (char*) { assert(false); }
        catch (const char* value) { assert(value == text); caught = true; }
        assert(caught);
    }
    {
        bool caught = false;
        THROW_AS(*reinterpret_cast<Pointer*>(_ZTIPi_nid_postfix), &number) (void* value) { assert(value == &number); caught = true; }
        assert(caught);
    }
    {
        bool caught = false;
        THROW_AS(*reinterpret_cast<Pointer*>(_ZTIPi_nid_postfix), &number) (long*) { assert(false); }
        catch (const int* value) { assert(value == &number); caught = true; }
        assert(caught);
    }
    {
        bool caught = false;
        THROW_AS(*reinterpret_cast<Fundamental*>(_ZTIi_nid_postfix), 42) (long) { assert(false); }
        catch (int value) { assert(value == 42); caught = true; }
        assert(caught);
    }
    {
        bool caught = false;
        THROW_AS(*reinterpret_cast<Pointer*>(_ZTIPv_nid_postfix), static_cast<void*>(&number)) (int*) { assert(false); }
        catch (void* value) { assert(value == &number); caught = true; }
        assert(caught);
    }
    assert(destroyed == 6);
    std::puts("exception fundamental type tests passed");
    return 0;
}
