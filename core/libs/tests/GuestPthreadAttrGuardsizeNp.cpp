#include "prx/libkernel/include/Api.hpp"
#include "SceTypes.hpp"
#include <cstddef>
#include <cstdlib>
#include <future>

static constexpr int SCE_OK = 0;
static constexpr std::size_t CREATION_GUARD_SIZE = 0x3000;

static void Require(bool value) { if (!value) std::abort(); }

static void* APS5_VABI Worker(void* arg) {
    static_cast<std::future<void>*>(arg)->get();
    return nullptr;
}

int main() {
    PthreadAttr defaults = nullptr;
    Require(scePthreadAttrInit(&defaults) == SCE_OK);
    std::size_t defaultGuard = 0;
    Require(scePthreadAttrGetguardsize(&defaults, &defaultGuard) == SCE_OK);
    Require(scePthreadAttrDestroy(&defaults) == SCE_OK);

    PthreadAttr attr = nullptr;
    Require(scePthreadAttrInit(&attr) == SCE_OK);
    Require(scePthreadAttrSetguardsize(&attr, CREATION_GUARD_SIZE) == SCE_OK);
    std::promise<void> release;
    auto released = release.get_future();
    Pthread thread = nullptr;
    Require(scePthreadCreate(&thread, &attr, Worker, &released, nullptr) == SCE_OK);
    Require(scePthreadAttrDestroy(&attr) == SCE_OK);

    PthreadAttr query = nullptr;
    Require(scePthreadAttrInit(&query) == SCE_OK);
    Require(pthread_attr_get_np_nid_postfix(thread, &query) == 0);
    std::size_t guard = 0;
    Require(pthread_attr_getguardsize_nid_postfix(&query, &guard) == 0 && guard == CREATION_GUARD_SIZE);
    Require(scePthreadAttrDestroy(&query) == SCE_OK);

    release.set_value();
    Require(scePthreadJoin(thread, nullptr) == SCE_OK);

    std::promise<void> releaseDefault;
    auto releasedDefault = releaseDefault.get_future();
    Pthread defaultThread = nullptr;
    Require(scePthreadCreate(&defaultThread, nullptr, Worker, &releasedDefault, nullptr) == SCE_OK);

    PthreadAttr defaultQuery = nullptr;
    Require(scePthreadAttrInit(&defaultQuery) == SCE_OK);
    Require(pthread_attr_get_np_nid_postfix(defaultThread, &defaultQuery) == 0);
    guard = 0;
    Require(pthread_attr_getguardsize_nid_postfix(&defaultQuery, &guard) == 0 && guard == defaultGuard);
    Require(scePthreadAttrDestroy(&defaultQuery) == SCE_OK);

    releaseDefault.set_value();
    Require(scePthreadJoin(defaultThread, nullptr) == SCE_OK);
}
