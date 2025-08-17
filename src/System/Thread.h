#pragma once

#include <Core/Core.h>
#include <thread>
#include <atomic>

namespace Vortex {

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

    std::stop_token getStopToken();

    /// Returns true if the thread has terminated, false if the thread is still
    /// running.
    bool isDone() const;

    /// The worker function called by the thread created in "start".
    virtual void exec() = 0;

   private:
    std::jthread thread;
    std::atomic_bool done;
};

};  // namespace Vortex
