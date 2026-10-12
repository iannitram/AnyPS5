#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

struct Attributes {
    std::int64_t flags;
    std::int64_t maximumMessages;
    std::int64_t messageSize;
    std::int64_t currentMessages;
    std::int64_t reserved[4];
};

struct SignalEvent {
    int notify;
    int signal;
    std::uint64_t value;
    std::uint64_t function;
    std::uint64_t attributes;
};

}

extern "C" {
void* APS5_VABI mq_open_nid_postfix(const char*, int, ...);
int APS5_VABI mq_close_nid_postfix(void*);
int APS5_VABI mq_unlink_nid_postfix(const char*);
int APS5_VABI mq_send_nid_postfix(void*, const char*, std::size_t, unsigned);
int APS5_VABI mq_timedsend_nid_postfix(void*, const char*, std::size_t, unsigned, const KernelTimespec*);
std::int64_t APS5_VABI mq_receive_nid_postfix(void*, char*, std::size_t, unsigned*);
std::int64_t APS5_VABI mq_timedreceive_nid_postfix(void*, char*, std::size_t, unsigned*, const KernelTimespec*);
int APS5_VABI mq_getattr_nid_postfix(void*, Attributes*);
int APS5_VABI mq_setattr_nid_postfix(void*, const Attributes*, Attributes*);
int APS5_VABI mq_notify_nid_postfix(void*, const SignalEvent*);
int APS5_VABI clock_gettime_nid_postfix(int, KernelTimespec*);
int* APS5_VABI __error_nid_postfix();
}

namespace {

constexpr int ReadOnly = 0;
constexpr int WriteOnly = 1;
constexpr int ReadWrite = 2;
constexpr int NonBlocking = 0x0004;
constexpr int Create = 0x0200;
constexpr int Exclusive = 0x0800;
void* const Failed = reinterpret_cast<void*>(-1);

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "failed: %s (errno %d)\n", message, *__error_nid_postfix());
        std::abort();
    }
}

bool Fails(long long result, int error) { return result == -1 && *__error_nid_postfix() == error; }

KernelTimespec In(long milliseconds) {
    KernelTimespec now{};
    clock_gettime_nid_postfix(0, &now);
    now.tv_nsec += milliseconds * 1000000;
    now.tv_sec += now.tv_nsec / 1000000000;
    now.tv_nsec %= 1000000000;
    return now;
}

void CheckOpen() {
    Require(mq_open_nid_postfix("/anyps5-missing", ReadWrite) == Failed && *__error_nid_postfix() == 2, "missing queue");
    Require(mq_open_nid_postfix("relative", ReadWrite | Create, 0600, nullptr) == Failed && *__error_nid_postfix() == 22, "name without slash");
    Require(mq_open_nid_postfix("/a/b", ReadWrite | Create, 0600, nullptr) == Failed && *__error_nid_postfix() == 22, "name with inner slash");
    Require(mq_open_nid_postfix("/", ReadWrite | Create, 0600, nullptr) == Failed && *__error_nid_postfix() == 22, "empty name");
    const std::string longest = "/" + std::string(254, 'q');
    void* queue = mq_open_nid_postfix(longest.c_str(), ReadWrite | Create, 0600, nullptr);
    Require(queue != Failed && mq_close_nid_postfix(queue) == 0 && mq_unlink_nid_postfix(longest.c_str()) == 0, "255-byte name");
    Require(mq_open_nid_postfix((longest + "q").c_str(), ReadWrite | Create, 0600, nullptr) == Failed && *__error_nid_postfix() == 63, "name too long");
    Attributes bad{0, 0, 16, 0, {}};
    Require(mq_open_nid_postfix("/anyps5-bad", ReadWrite | Create, 0600, &bad) == Failed && *__error_nid_postfix() == 22, "zero messages");
    bad = {0, 101, 16, 0, {}};
    Require(mq_open_nid_postfix("/anyps5-bad", ReadWrite | Create, 0600, &bad) == Failed && *__error_nid_postfix() == 22, "too many messages");
    bad = {0, 4, 16385, 0, {}};
    Require(mq_open_nid_postfix("/anyps5-bad", ReadWrite | Create, 0600, &bad) == Failed && *__error_nid_postfix() == 22, "message too large");

    void* fresh = mq_open_nid_postfix("/anyps5-defaults", ReadWrite | Create | Exclusive, 0600, nullptr);
    Attributes attributes{};
    Require(fresh != Failed && mq_getattr_nid_postfix(fresh, &attributes) == 0, "defaults");
    Require(attributes.maximumMessages == 10 && attributes.messageSize == 1024 && attributes.currentMessages == 0 && attributes.flags == 0, "default attributes");
    Require(mq_open_nid_postfix("/anyps5-defaults", ReadWrite | Create | Exclusive, 0600, nullptr) == Failed && *__error_nid_postfix() == 17, "exclusive");
    void* again = mq_open_nid_postfix("/anyps5-defaults", ReadOnly | NonBlocking);
    Require(again != Failed && again != fresh && mq_getattr_nid_postfix(again, &attributes) == 0 && attributes.flags == NonBlocking, "second descriptor");
    Require(mq_unlink_nid_postfix("/anyps5-defaults") == 0 && Fails(mq_unlink_nid_postfix("/anyps5-defaults"), 2), "unlink");
    Require(mq_open_nid_postfix("/anyps5-defaults", ReadWrite) == Failed && *__error_nid_postfix() == 2, "unlinked name is gone");
    Require(mq_send_nid_postfix(fresh, "kept", 4, 0) == 0, "unlinked queue stays usable");
    char buffer[1024];
    Require(mq_receive_nid_postfix(again, buffer, sizeof(buffer), nullptr) == 4, "shared by descriptors");
    Require(mq_close_nid_postfix(fresh) == 0 && mq_close_nid_postfix(again) == 0, "close");
    Require(Fails(mq_close_nid_postfix(fresh), 9), "double close");
}

void CheckMessages() {
    Attributes requested{0, 3, 8, 0, {}};
    void* queue = mq_open_nid_postfix("/anyps5-messages", ReadWrite | Create, 0600, &requested);
    Require(queue != Failed, "create small queue");
    Require(mq_send_nid_postfix(queue, "low", 3, 1) == 0, "send low");
    Require(mq_send_nid_postfix(queue, "high", 4, 9) == 0, "send high");
    Require(mq_send_nid_postfix(queue, "low2", 4, 1) == 0, "send second low");
    Require(Fails(mq_send_nid_postfix(queue, "123456789", 9, 0), 40), "message size");
    Require(Fails(mq_send_nid_postfix(queue, "x", 1, 64), 22), "priority limit");
    Attributes attributes{};
    Require(mq_getattr_nid_postfix(queue, &attributes) == 0 && attributes.currentMessages == 3 && attributes.maximumMessages == 3 && attributes.messageSize == 8, "current messages");

    KernelTimespec soon = In(30);
    Require(Fails(mq_timedsend_nid_postfix(queue, "full", 4, 0, &soon), 60), "timed send times out on a full queue");
    KernelTimespec invalid{0, 2000000000};
    Require(Fails(mq_timedsend_nid_postfix(queue, "full", 4, 0, &invalid), 22), "invalid timeout");
    Attributes nonblocking{NonBlocking, 0, 0, 0, {}};
    Attributes previous{};
    Require(mq_setattr_nid_postfix(queue, &nonblocking, &previous) == 0 && previous.flags == 0, "set nonblocking");
    Require(Fails(mq_send_nid_postfix(queue, "full", 4, 0), 35), "nonblocking full queue");

    char buffer[8];
    unsigned priority = 0;
    Require(Fails(mq_receive_nid_postfix(queue, buffer, 7, &priority), 40), "receive buffer smaller than the message size");
    Require(mq_receive_nid_postfix(queue, buffer, sizeof(buffer), &priority) == 4 && priority == 9 && std::memcmp(buffer, "high", 4) == 0, "highest priority first");
    Require(mq_receive_nid_postfix(queue, buffer, sizeof(buffer), &priority) == 3 && priority == 1 && std::memcmp(buffer, "low", 3) == 0, "FIFO within a priority");
    Require(mq_receive_nid_postfix(queue, buffer, sizeof(buffer), &priority) == 4 && std::memcmp(buffer, "low2", 4) == 0, "last message");
    Require(Fails(mq_receive_nid_postfix(queue, buffer, sizeof(buffer), &priority), 35), "nonblocking empty queue");
    Require(mq_setattr_nid_postfix(queue, &previous, nullptr) == 0, "restore blocking");
    soon = In(30);
    Require(Fails(mq_timedreceive_nid_postfix(queue, buffer, sizeof(buffer), &priority, &soon), 60), "timed receive times out");
    Require(mq_send_nid_postfix(queue, "", 0, 0) == 0 && mq_timedreceive_nid_postfix(queue, buffer, sizeof(buffer), nullptr, &invalid) == 0, "empty message, timeout unchecked when not blocking");

    std::atomic<bool> received{false};
    std::thread receiver([&] {
        char message[8];
        unsigned value = 0;
        received = mq_receive_nid_postfix(queue, message, sizeof(message), &value) == 5 && value == 3 && std::memcmp(message, "wakes", 5) == 0;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    Require(mq_send_nid_postfix(queue, "wakes", 5, 3) == 0, "send to a waiting receiver");
    receiver.join();
    Require(received, "blocked receiver woke");

    for (int i = 0; i < 3; i++) Require(mq_send_nid_postfix(queue, "f", 1, 0) == 0, "fill");
    std::atomic<bool> sent{false};
    std::thread sender([&] { sent = mq_send_nid_postfix(queue, "late", 4, 0) == 0; });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    Require(!sent, "sender blocks on a full queue");
    Require(mq_receive_nid_postfix(queue, buffer, sizeof(buffer), nullptr) == 1, "make room");
    sender.join();
    Require(sent, "blocked sender woke");

    void* reader = mq_open_nid_postfix("/anyps5-messages", ReadOnly);
    void* writer = mq_open_nid_postfix("/anyps5-messages", WriteOnly);
    Require(Fails(mq_send_nid_postfix(reader, "x", 1, 0), 9) && Fails(mq_receive_nid_postfix(writer, buffer, sizeof(buffer), nullptr), 9), "access modes");

    Attributes state{};
    while (mq_getattr_nid_postfix(queue, &state) == 0 && state.currentMessages > 0) mq_receive_nid_postfix(queue, buffer, sizeof(buffer), nullptr);
    SignalEvent none{0, 0, 0, 0, 0};
    Require(mq_notify_nid_postfix(queue, &none) == 0 && Fails(mq_notify_nid_postfix(reader, &none), 16), "one notification registration");
    Require(mq_notify_nid_postfix(queue, nullptr) == 0 && mq_notify_nid_postfix(reader, &none) == 0, "unregister and register again");
    SignalEvent signal{1, 30, 0, 0, 0};
    Require(mq_notify_nid_postfix(reader, nullptr) == 0, "unregister");
    bool threw = false;
    try {
        mq_notify_nid_postfix(queue, &signal);
    } catch (const std::exception&) {
        threw = true;
    }
    Require(threw, "signal notification throws");
    SignalEvent unknown{7, 0, 0, 0, 0};
    Require(Fails(mq_notify_nid_postfix(queue, &unknown), 22), "unknown notification is EINVAL");
    Require(mq_send_nid_postfix(queue, "n", 1, 0) == 0, "queue a message");
    Require(mq_notify_nid_postfix(queue, &none) == 0 && mq_notify_nid_postfix(reader, &none) == 0, "registering on a non-empty queue with no receiver is consumed at once, as kern_kmq_notify does");
    Require(mq_notify_nid_postfix(reader, nullptr) == 0 && mq_receive_nid_postfix(queue, buffer, sizeof(buffer), nullptr) == 1, "drain");
    Require(mq_notify_nid_postfix(queue, &none) == 0 && mq_send_nid_postfix(queue, "m", 1, 0) == 0 && mq_notify_nid_postfix(reader, &none) == 0, "a send with no receiver consumes the registration");
    Require(mq_notify_nid_postfix(reader, nullptr) == 0 && mq_receive_nid_postfix(queue, buffer, sizeof(buffer), nullptr) == 1, "drain again");
    Require(Fails(mq_send_nid_postfix(queue, buffer, sizeof(buffer) + 1, 64), 22), "priority is checked before size, as mqueue_send does");

    Require(mq_close_nid_postfix(reader) == 0 && mq_close_nid_postfix(writer) == 0 && mq_close_nid_postfix(queue) == 0, "close all");
    Require(mq_unlink_nid_postfix("/anyps5-messages") == 0, "unlink after close");
    Require(Fails(mq_send_nid_postfix(queue, "x", 1, 0), 9), "closed descriptor");

    void* doomed = mq_open_nid_postfix("/anyps5-doomed", ReadWrite | Create, 0600, nullptr);
    Require(doomed != Failed, "open doomed queue");
    std::atomic<bool> timedOut{false};
    std::thread waiter([&] {
        char message[1024];
        KernelTimespec later = In(300);
        timedOut = Fails(mq_timedreceive_nid_postfix(doomed, message, sizeof(message), nullptr, &later), 60);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Require(mq_unlink_nid_postfix("/anyps5-doomed") == 0 && mq_close_nid_postfix(doomed) == 0, "unlink and close under a blocked receiver");
    void* other = mq_open_nid_postfix("/anyps5-other", ReadWrite | Create, 0600, nullptr);
    Require(other != Failed, "the queue registry stays usable");
    waiter.join();
    Require(timedOut, "the blocked receiver keeps waiting on its queue until its timeout");
    Require(mq_close_nid_postfix(other) == 0 && mq_unlink_nid_postfix("/anyps5-other") == 0, "cleanup");
}

}

int main() {
    CheckOpen();
    CheckMessages();
    std::puts("guest message queue tests passed");
    return 0;
}
