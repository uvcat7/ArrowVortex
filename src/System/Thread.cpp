#include <System/Thread.h>

#include <Core/Utils.h>
#include <Core/AlignedMemory.h>

#include <vector>

namespace Vortex {

BackgroundThread::BackgroundThread() { done = false; }

BackgroundThread::~BackgroundThread() = default;

void BackgroundThread::start() {
    if (isDone()) {
        return;
    }

    thread = std::jthread([&]() {
        exec();
        done = true;
    });
}

void BackgroundThread::terminate() {
    thread.request_stop();
    waitUntilDone();
}

void BackgroundThread::waitUntilDone() {
    if (!thread.joinable()) {
        return;
    }
    thread.join();
}

std::stop_token BackgroundThread::getStopToken() {
    return thread.get_stop_token();
}

bool BackgroundThread::isDone() const { return done; }

};  // namespace Vortex
