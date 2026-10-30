#pragma once

#include <Core/Core.h>
#include <thread>
#include <atomic>
#include <memory>

namespace Vortex {

/// A copyable view of a background thread's cooperative cancellation state.
class StopToken {
   public:
    StopToken() = default;
    bool stop_requested() const {
        return stopRequested && stopRequested->load();
    }

   private:
    friend class BackgroundThread;
    explicit StopToken(std::shared_ptr<std::atomic_bool> stopRequested)
        : stopRequested(stopRequested) {}

    std::shared_ptr<std::atomic_bool> stopRequested;
};

/// A thread that performs a task, running in the background.
class BackgroundThread {
   public:
    virtual ~BackgroundThread();
    BackgroundThread();
    /// Creates a thread, which calls "exec" once, and then terminates. The
    /// function returns when the thread is created; use "waitUntilDone" to wait
    /// until the thread has terminated.
    void start();

    /// Sets the terminate flag and waits until the thread is terminated. The
    /// terminate flag is only a request; The "exec" function is responsible for
    /// testing the flag and returning.
    void terminate();

    /// Waits until the thread has terminated, after which the function returns.
    void waitUntilDone();

    StopToken getStopToken() const;

    /// Returns true if the thread has terminated, false if the thread is still
    /// running.
    bool isDone() const;
    bool shouldStop() const;
    virtual void exec() = 0;

   private:
    std::thread thread;
    std::atomic_bool done{false};
    std::shared_ptr<std::atomic_bool> stopRequested;
};

};  // namespace Vortex
