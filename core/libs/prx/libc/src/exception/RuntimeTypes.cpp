#include "prx/libc/include/exceptions/Runtime.hpp"

namespace LibcException {
struct FundamentalRecord { const void* vtable; const char* name; };
struct PointerRecord { const void* vtable; const char* name; unsigned flags; const void* pointee; };

static_assert(sizeof(FundamentalRecord) == sizeof(std::type_info));
static_assert(sizeof(PointerRecord) == sizeof(__cxxabiv1::__pointer_type_info));
static_assert(offsetof(PointerRecord, flags) == 2 * sizeof(void*));
static_assert(offsetof(PointerRecord, pointee) == 3 * sizeof(void*));

inline constexpr unsigned PointerConstMask = 1;
}

#define APS5_TYPE_INFO(record) (reinterpret_cast<const std::type_info*>(&record))

extern "C" {
LibcException::TypeRecord _ZTIN10__cxxabiv116__shim_type_infoE_nid_postfix {LibcException::ClassTypeVtable + 2, "N10__cxxabiv116__shim_type_infoE", nullptr};
LibcException::TypeRecord _ZTIN10__cxxabiv117__class_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv117__class_type_infoE", &_ZTIN10__cxxabiv116__shim_type_infoE_nid_postfix};
LibcException::TypeRecord _ZTIN10__cxxabiv116__enum_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv116__enum_type_infoE", &_ZTIN10__cxxabiv116__shim_type_infoE_nid_postfix};
LibcException::TypeRecord _ZTIN10__cxxabiv117__array_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv117__array_type_infoE", &_ZTIN10__cxxabiv116__shim_type_infoE_nid_postfix};
LibcException::TypeRecord _ZTIN10__cxxabiv117__pbase_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv117__pbase_type_infoE", &_ZTIN10__cxxabiv116__shim_type_infoE_nid_postfix};
LibcException::TypeRecord _ZTIN10__cxxabiv120__function_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv120__function_type_infoE", &_ZTIN10__cxxabiv116__shim_type_infoE_nid_postfix};
LibcException::TypeRecord _ZTIN10__cxxabiv123__fundamental_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv123__fundamental_type_infoE", &_ZTIN10__cxxabiv116__shim_type_infoE_nid_postfix};
LibcException::TypeRecord _ZTIN10__cxxabiv119__pointer_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv119__pointer_type_infoE", &_ZTIN10__cxxabiv117__pbase_type_infoE_nid_postfix};
LibcException::TypeRecord _ZTIN10__cxxabiv129__pointer_to_member_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv129__pointer_to_member_type_infoE", &_ZTIN10__cxxabiv117__pbase_type_infoE_nid_postfix};
LibcException::TypeRecord _ZTIN10__cxxabiv120__si_class_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv120__si_class_type_infoE", &_ZTIN10__cxxabiv117__class_type_infoE_nid_postfix};
LibcException::TypeRecord _ZTIN10__cxxabiv121__vmi_class_type_infoE_nid_postfix {LibcException::SingleTypeVtable + 2, "N10__cxxabiv121__vmi_class_type_infoE", &_ZTIN10__cxxabiv117__class_type_infoE_nid_postfix};

LibcException::TypeInfoVtable _ZTVN10__cxxabiv116__shim_type_infoE_nid_postfix {0, APS5_TYPE_INFO(_ZTIN10__cxxabiv116__shim_type_infoE_nid_postfix)};
LibcException::TypeInfoVtable _ZTVN10__cxxabiv116__enum_type_infoE_nid_postfix {0, APS5_TYPE_INFO(_ZTIN10__cxxabiv116__enum_type_infoE_nid_postfix)};
LibcException::TypeInfoVtable _ZTVN10__cxxabiv117__array_type_infoE_nid_postfix {0, APS5_TYPE_INFO(_ZTIN10__cxxabiv117__array_type_infoE_nid_postfix)};
LibcException::TypeInfoVtable _ZTVN10__cxxabiv117__pbase_type_infoE_nid_postfix {0, APS5_TYPE_INFO(_ZTIN10__cxxabiv117__pbase_type_infoE_nid_postfix)};
LibcException::TypeInfoVtable _ZTVN10__cxxabiv123__fundamental_type_infoE_nid_postfix {0, APS5_TYPE_INFO(_ZTIN10__cxxabiv123__fundamental_type_infoE_nid_postfix)};
LibcException::TypeInfoVtable _ZTVN10__cxxabiv129__pointer_to_member_type_infoE_nid_postfix {0, APS5_TYPE_INFO(_ZTIN10__cxxabiv129__pointer_to_member_type_infoE_nid_postfix)};
LibcException::TypeInfoVtable _ZTVN10__cxxabiv119__pointer_type_infoE_nid_postfix {0, APS5_TYPE_INFO(_ZTIN10__cxxabiv119__pointer_type_infoE_nid_postfix),
    LibcException::DestroyTypeInfo, LibcException::DeleteTypeInfo, LibcException::IsTypeInfoKind, LibcException::IsPointerTypeInfo};
LibcException::TypeInfoVtable _ZTVN10__cxxabiv120__function_type_infoE_nid_postfix {0, APS5_TYPE_INFO(_ZTIN10__cxxabiv120__function_type_infoE_nid_postfix),
    LibcException::DestroyTypeInfo, LibcException::DeleteTypeInfo, LibcException::IsPointerTypeInfo, LibcException::IsTypeInfoKind};

#define APS5_FUNDAMENTAL_TYPE(code) \
    LibcException::FundamentalRecord _ZTI##code##_nid_postfix {&_ZTVN10__cxxabiv123__fundamental_type_infoE_nid_postfix.destroy, #code}; \
    LibcException::PointerRecord _ZTIP##code##_nid_postfix {&_ZTVN10__cxxabiv119__pointer_type_infoE_nid_postfix.destroy, "P" #code, 0, &_ZTI##code##_nid_postfix}; \
    LibcException::PointerRecord _ZTIPK##code##_nid_postfix {&_ZTVN10__cxxabiv119__pointer_type_infoE_nid_postfix.destroy, "PK" #code, LibcException::PointerConstMask, &_ZTI##code##_nid_postfix};

APS5_FUNDAMENTAL_TYPE(v)
APS5_FUNDAMENTAL_TYPE(Dn)
APS5_FUNDAMENTAL_TYPE(b)
APS5_FUNDAMENTAL_TYPE(w)
APS5_FUNDAMENTAL_TYPE(c)
APS5_FUNDAMENTAL_TYPE(a)
APS5_FUNDAMENTAL_TYPE(h)
APS5_FUNDAMENTAL_TYPE(s)
APS5_FUNDAMENTAL_TYPE(t)
APS5_FUNDAMENTAL_TYPE(i)
APS5_FUNDAMENTAL_TYPE(j)
APS5_FUNDAMENTAL_TYPE(l)
APS5_FUNDAMENTAL_TYPE(m)
APS5_FUNDAMENTAL_TYPE(x)
APS5_FUNDAMENTAL_TYPE(y)
APS5_FUNDAMENTAL_TYPE(n)
APS5_FUNDAMENTAL_TYPE(o)
APS5_FUNDAMENTAL_TYPE(f)
APS5_FUNDAMENTAL_TYPE(d)
APS5_FUNDAMENTAL_TYPE(e)
APS5_FUNDAMENTAL_TYPE(g)
APS5_FUNDAMENTAL_TYPE(Dh)
APS5_FUNDAMENTAL_TYPE(Di)
APS5_FUNDAMENTAL_TYPE(Ds)
APS5_FUNDAMENTAL_TYPE(Du)
}
