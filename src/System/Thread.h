#pragma once

#include <Core/Core.h>

namespace Vortex {

/// A thread that performs a task, running in the background.
class BackgroundThread
{
public:
	virtual ~BackgroundThread();

	BackgroundThread();

	/// Creates a thread, which calls "exec" once, and then terminates. The function returns when
	/// the thread is created; use "waitUntilDone" to wait until the thread has terminated.
	void start();

	/// Sets the terminate flag and waits until the thread is terminated. The terminate flag is
	/// only a request; The "exec" function is responsible for testing the flag and returning.
	void terminate();

	/// Waits until the thread has terminated, after which the function returns.
	void waitUntilDone();

	/// Returns true if the thread has terminated, false if the thread is still running.
	bool isDone() const;

	/// The worker function called by the thread created in "start".
	virtual void exec() = 0;

protected:
	uint8_t terminationFlag_;
private:
	void* data_;  // TODO: replace with a more descriptive variable name.
};

/// A set of multiple threads that perform the same task, which split into items.
class ParallelThreads
{
public:
	virtual ~ParallelThreads();

	ParallelThreads();

	/// Returns the number of concurrent threads supported by the hardware.
	static int concurrency();

	/// Creates several threads, which concurrently start calling "exec". Once "exec" has been
	/// called for every value starting from zero up to numItems - 1, the function returns.
	void run(int numItems, int numThreads = concurrency());

	/// The worker function called by the threads created in "run".
	virtual void exec(int item, int thread) = 0;
};

/// A wrapper around critical section.
class CriticalSection
{
public:
	CriticalSection();
	~CriticalSection();

	void lock();
	void unlock();

private:
	void* criticalSectionHandle;
};

}; // namespace Vortex
