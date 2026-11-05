#include "prx/libkernel/Time/include/Time.hpp"
#include <chrono>
#include <cstdint>
#include <thread>

namespace {
    constexpr std::uint64_t realWaitNanos = 3000000ULL;
    constexpr auto reportGap = std::chrono::milliseconds(3100);
    char siteMarks[8];

    void RecordWait(std::size_t site, std::uint64_t waitedNanos) {
        KernelTraceWait_nid_postfix("cond", &siteMarks[site], waitedNanos, false);
    }
}

int main() {
    for (std::size_t site = 0; site < 8; ++site) RecordWait(site, realWaitNanos);
    std::this_thread::sleep_for(reportGap);
    RecordWait(0, realWaitNanos);
    RecordWait(0, realWaitNanos);
    std::this_thread::sleep_for(reportGap);
    RecordWait(1, 0);
    return 0;
}
