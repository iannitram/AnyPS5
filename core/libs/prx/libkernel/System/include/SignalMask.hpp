#ifndef CORE_LIBS_PRX_LIBKERNEL_SYSTEM_INCLUDE_SIGNALMASK_HPP
#define CORE_LIBS_PRX_LIBKERNEL_SYSTEM_INCLUDE_SIGNALMASK_HPP

#include <cstdint>

struct GuestSignalSet {
    std::uint32_t bits[4];
};

GuestSignalSet CurrentGuestSignalMask();
void SetCurrentGuestSignalMask(const GuestSignalSet& mask);

#endif
