#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "Common.hpp"
#include "../include/Cancel.hpp"
#include "prx/libkernel/Time/include/TimedWait.hpp"

namespace {

// FreeBSD 11 struct mq_attr: four longs and four reserved ones.
struct GuestQueueAttributes {
    std::int64_t flags;
    std::int64_t maximumMessages;
    std::int64_t messageSize;
    std::int64_t currentMessages;
    std::int64_t reserved[4];
};
static_assert(sizeof(GuestQueueAttributes) == 64);

struct GuestSignalEvent {
    int notify;
    int signal;
};

constexpr int GuestNoEntry = 2;
constexpr int GuestBadDescriptor = 9;
constexpr int GuestExists = 17;
constexpr int GuestAgain = 35;
constexpr int GuestMessageSize = 40;
constexpr int GuestNameTooLong = 63;
constexpr int GuestBusy = 16;

constexpr int AccessMask = 0x0003;
constexpr int ReadOnly = 0x0000;
constexpr int WriteOnly = 0x0001;
constexpr int NonBlocking = 0x0004;
constexpr int Create = 0x0200;
constexpr int Exclusive = 0x0800;

constexpr std::size_t NameMaximum = 255;
constexpr std::int64_t DefaultMessages = 10;
constexpr std::int64_t DefaultMessageSize = 1024;
constexpr std::int64_t MaximumMessages = 100;
constexpr std::int64_t MaximumMessageSize = 16384;
constexpr unsigned PriorityLimit = 64;
constexpr int NotifyNone = 0;
constexpr int RealtimeClock = 0;

struct Message {
    unsigned priority;
    std::vector<char> data;
};

struct Descriptor;

struct Queue {
    std::string name;
    bool linked = true;
    int openCount = 0;
    std::int64_t maximumMessages;
    std::int64_t messageSize;
    std::mutex lock;
    TimedWait::Condition notEmpty;
    TimedWait::Condition notFull;
    std::list<Message> messages;
    int receiversWaiting = 0;
    const Descriptor* notifier = nullptr;
};

struct Descriptor {
    std::shared_ptr<Queue> queue;
    int flags;
};

std::mutex registryLock;
std::list<std::shared_ptr<Queue>> queues;
std::set<const Descriptor*> descriptors;

int Fail(int error) {
    errno = error;
    return -1;
}

void* FailOpen(int error) {
    errno = error;
    return reinterpret_cast<void*>(-1);
}

// FreeBSD's mqueuefs accepts "/name" with no other slash, of at most 255 bytes.
int CheckName(const char* name) {
    if (name == nullptr) throw std::invalid_argument("mq: null name");
    const std::size_t length = std::strlen(name);
    if (length > NameMaximum) return GuestNameTooLong;
    if (name[0] != '/' || length < 2 || std::strchr(name + 1, '/') != nullptr) return PosixThread::GUEST_EINVAL;
    return 0;
}

Descriptor* Find(void* handle) {
    std::lock_guard lock(registryLock);
    auto* descriptor = static_cast<Descriptor*>(handle);
    return descriptors.count(descriptor) ? descriptor : nullptr;
}

std::optional<std::uint64_t> Deadline(const KernelTimespec* timeout, bool& invalid) {
    invalid = false;
    if (timeout == nullptr) return std::nullopt;
    KernelUseconds usec = 0;
    if (!PosixThread::RelativeMicroseconds(RealtimeClock, timeout, &usec)) {
        invalid = true;
        return std::nullopt;
    }
    return TimedWait::DeadlineNanos(usec);
}

int Send(void* handle, const char* data, std::size_t length, unsigned priority, const KernelTimespec* timeout, bool timed) {
    Descriptor* descriptor = Find(handle);
    if (descriptor == nullptr || (descriptor->flags & AccessMask) == ReadOnly) return Fail(GuestBadDescriptor);
    Queue& queue = *descriptor->queue;
    if (length > static_cast<std::size_t>(queue.messageSize)) return Fail(GuestMessageSize);
    if (priority >= PriorityLimit) return Fail(PosixThread::GUEST_EINVAL);
    if (data == nullptr && length != 0) throw std::invalid_argument("mq_send: null message");
    ThreadCancel::Check();
    std::unique_lock lock(queue.lock);
    const auto room = [&] { return static_cast<std::int64_t>(queue.messages.size()) < queue.maximumMessages; };
    if (!room()) {
        if (descriptor->flags & NonBlocking) return Fail(GuestAgain);
        bool invalid = false;
        const auto deadline = timed ? Deadline(timeout, invalid) : std::nullopt;
        if (invalid) return Fail(PosixThread::GUEST_EINVAL);
        if (!ThreadCancel::WaitUntil(queue.notFull, lock, deadline, room)) return Fail(PosixThread::GUEST_ETIMEDOUT);
    }
    auto position = queue.messages.begin();
    while (position != queue.messages.end() && position->priority >= priority) ++position;
    const bool wasEmpty = queue.messages.empty();
    queue.messages.insert(position, Message{priority, std::vector<char>(data, data + length)});
    if (wasEmpty && queue.receiversWaiting == 0 && queue.notifier != nullptr) queue.notifier = nullptr;
    queue.notEmpty.NotifyOne();
    return 0;
}

std::int64_t Receive(void* handle, char* buffer, std::size_t length, unsigned* priority, const KernelTimespec* timeout, bool timed) {
    Descriptor* descriptor = Find(handle);
    if (descriptor == nullptr || (descriptor->flags & AccessMask) == WriteOnly) return Fail(GuestBadDescriptor);
    Queue& queue = *descriptor->queue;
    if (length < static_cast<std::size_t>(queue.messageSize)) return Fail(GuestMessageSize);
    if (buffer == nullptr) throw std::invalid_argument("mq_receive: null buffer");
    ThreadCancel::Check();
    std::unique_lock lock(queue.lock);
    const auto ready = [&] { return !queue.messages.empty(); };
    if (!ready()) {
        if (descriptor->flags & NonBlocking) return Fail(GuestAgain);
        bool invalid = false;
        const auto deadline = timed ? Deadline(timeout, invalid) : std::nullopt;
        if (invalid) return Fail(PosixThread::GUEST_EINVAL);
        ++queue.receiversWaiting;
        const bool received = ThreadCancel::WaitUntil(queue.notEmpty, lock, deadline, ready);
        --queue.receiversWaiting;
        if (!received) return Fail(PosixThread::GUEST_ETIMEDOUT);
    }
    Message message = std::move(queue.messages.front());
    queue.messages.pop_front();
    std::memcpy(buffer, message.data.data(), message.data.size());
    if (priority != nullptr) *priority = message.priority;
    queue.notFull.NotifyOne();
    return static_cast<std::int64_t>(message.data.size());
}

}

extern "C" {

void* APS5_VABI mq_open_nid_postfix(const char* name, int flags, ...) {
    if (const int error = CheckName(name)) return FailOpen(error);
    const GuestQueueAttributes* attributes = nullptr;
    if (flags & Create) {
#ifdef _WIN32
        __builtin_sysv_va_list arguments;
        __builtin_sysv_va_start(arguments, flags);
        (void)__builtin_va_arg(arguments, int);
        attributes = __builtin_va_arg(arguments, const GuestQueueAttributes*);
        __builtin_sysv_va_end(arguments);
#else
        va_list arguments;
        va_start(arguments, flags);
        (void)va_arg(arguments, int);
        attributes = va_arg(arguments, const GuestQueueAttributes*);
        va_end(arguments);
#endif
    }
    if ((flags & AccessMask) == AccessMask) return FailOpen(PosixThread::GUEST_EINVAL);

    std::lock_guard lock(registryLock);
    std::shared_ptr<Queue> queue;
    for (auto& existing : queues) {
        if (existing->linked && existing->name == name) {
            queue = existing;
            break;
        }
    }
    if (queue != nullptr) {
        if ((flags & (Create | Exclusive)) == (Create | Exclusive)) return FailOpen(GuestExists);
    } else {
        if (!(flags & Create)) return FailOpen(GuestNoEntry);
        std::int64_t maximum = DefaultMessages;
        std::int64_t size = DefaultMessageSize;
        if (attributes != nullptr) {
            if (attributes->maximumMessages <= 0 || attributes->maximumMessages > MaximumMessages ||
                attributes->messageSize <= 0 || attributes->messageSize > MaximumMessageSize)
                return FailOpen(PosixThread::GUEST_EINVAL);
            maximum = attributes->maximumMessages;
            size = attributes->messageSize;
        }
        queue = std::make_shared<Queue>();
        queue->name = name;
        queue->maximumMessages = maximum;
        queue->messageSize = size;
        queues.push_back(queue);
    }
    ++queue->openCount;
    auto* descriptor = new Descriptor{queue, flags & (AccessMask | NonBlocking)};
    descriptors.insert(descriptor);
    return descriptor;
}

int APS5_VABI mq_close_nid_postfix(void* handle) {
    std::lock_guard lock(registryLock);
    auto* descriptor = static_cast<Descriptor*>(handle);
    if (!descriptors.erase(descriptor)) return Fail(GuestBadDescriptor);
    Queue& queue = *descriptor->queue;
    {
        std::lock_guard queueLock(queue.lock);
        if (queue.notifier == descriptor) queue.notifier = nullptr;
    }
    if (--queue.openCount == 0 && !queue.linked) queues.remove(descriptor->queue);
    delete descriptor;
    return 0;
}

int APS5_VABI mq_unlink_nid_postfix(const char* name) {
    if (const int error = CheckName(name)) return Fail(error);
    std::lock_guard lock(registryLock);
    for (auto entry = queues.begin(); entry != queues.end(); ++entry) {
        if (!(*entry)->linked || (*entry)->name != name) continue;
        (*entry)->linked = false;
        if ((*entry)->openCount == 0) queues.erase(entry);
        return 0;
    }
    return Fail(GuestNoEntry);
}

int APS5_VABI mq_send_nid_postfix(void* handle, const char* data, std::size_t length, unsigned priority) {
    return Send(handle, data, length, priority, nullptr, false);
}

int APS5_VABI mq_timedsend_nid_postfix(void* handle, const char* data, std::size_t length, unsigned priority, const KernelTimespec* timeout) {
    return Send(handle, data, length, priority, timeout, true);
}

std::int64_t APS5_VABI mq_receive_nid_postfix(void* handle, char* buffer, std::size_t length, unsigned* priority) {
    return Receive(handle, buffer, length, priority, nullptr, false);
}

std::int64_t APS5_VABI mq_timedreceive_nid_postfix(void* handle, char* buffer, std::size_t length, unsigned* priority, const KernelTimespec* timeout) {
    return Receive(handle, buffer, length, priority, timeout, true);
}

int APS5_VABI mq_getattr_nid_postfix(void* handle, GuestQueueAttributes* attributes) {
    Descriptor* descriptor = Find(handle);
    if (descriptor == nullptr) return Fail(GuestBadDescriptor);
    if (attributes == nullptr) throw std::invalid_argument("mq_getattr: null attributes");
    Queue& queue = *descriptor->queue;
    std::lock_guard lock(queue.lock);
    *attributes = {};
    attributes->flags = descriptor->flags & NonBlocking;
    attributes->maximumMessages = queue.maximumMessages;
    attributes->messageSize = queue.messageSize;
    attributes->currentMessages = static_cast<std::int64_t>(queue.messages.size());
    return 0;
}

int APS5_VABI mq_setattr_nid_postfix(void* handle, const GuestQueueAttributes* attributes, GuestQueueAttributes* previous) {
    Descriptor* descriptor = Find(handle);
    if (descriptor == nullptr) return Fail(GuestBadDescriptor);
    if (attributes == nullptr) throw std::invalid_argument("mq_setattr: null attributes");
    if (previous != nullptr) mq_getattr_nid_postfix(handle, previous);
    std::lock_guard lock(descriptor->queue->lock);
    descriptor->flags = (descriptor->flags & ~NonBlocking) | (static_cast<int>(attributes->flags) & NonBlocking);
    return 0;
}

int APS5_VABI mq_notify_nid_postfix(void* handle, const GuestSignalEvent* event) {
    Descriptor* descriptor = Find(handle);
    if (descriptor == nullptr) return Fail(GuestBadDescriptor);
    Queue& queue = *descriptor->queue;
    std::lock_guard lock(queue.lock);
    if (event == nullptr) {
        if (queue.notifier == descriptor) queue.notifier = nullptr;
        return 0;
    }
    if (queue.notifier != nullptr) return Fail(GuestBusy);
    if (event->notify != NotifyNone) throw std::runtime_error("mq_notify: signal and thread notification are not implemented");
    queue.notifier = descriptor;
    return 0;
}

}
