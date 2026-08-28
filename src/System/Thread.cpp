#include <System/Thread.h>

#include <Core/Utils.h>
#include <Core/AlignedMemory.h>

#include <vector>

namespace Vortex {

BackgroundThread::BackgroundThread() = default;

BackgroundThread::~BackgroundThread() { terminate(); }

void BackgroundThread::start() {
    if (thread.joinable()) {
        return;
    }

    stopRequested.store(false, std::memory_order_release);
    done.store(false, std::memory_order_release);
    thread = std::thread([&]() {
        exec();
        done.store(true, std::memory_order_release);
    });
}

void BackgroundThread::terminate() {
    stopRequested.store(true, std::memory_order_release);
    waitUntilDone();
}

void BackgroundThread::waitUntilDone() {
    if (!thread.joinable()) {
        return;
    }
    thread.join();
}

ThreadStopToken BackgroundThread::getStopToken() {
    return ThreadStopToken(&stopRequested);
}

bool BackgroundThread::isDone() const {
    return done.load(std::memory_order_acquire);
}

};  // namespace Vortex
