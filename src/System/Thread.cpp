#include <System/Thread.h>

namespace Vortex {

BackgroundThread::BackgroundThread()
    : stopRequested(std::make_shared<std::atomic_bool>(false)) {}

BackgroundThread::~BackgroundThread() { terminate(); }

void BackgroundThread::start() {
    if (thread.joinable()) {
        if (!done.load()) return;
        thread.join();
    }

    stopRequested->store(false);
    done.store(false);
    thread = std::thread([this]() {
        exec();
        done.store(true);
    });
}

void BackgroundThread::terminate() {
    stopRequested->store(true);
    if (thread.joinable()) thread.join();
}

void BackgroundThread::waitUntilDone() {
    if (thread.joinable()) thread.join();
}

bool BackgroundThread::isDone() const { return done.load(); }

bool BackgroundThread::shouldStop() const { return stopRequested->load(); }

StopToken BackgroundThread::getStopToken() const {
    return StopToken(stopRequested);
}

}  // namespace Vortex
