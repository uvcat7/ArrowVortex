#include <System/Thread.h>

#include <Core/Utils.h>
#include <Core/AlignedMemory.h>

#include <vector>

namespace Vortex {

BackgroundThread::BackgroundThread()
{
	terminationFlag_ = 0;
	done = false;
}

BackgroundThread::~BackgroundThread()
{
}

void BackgroundThread::start()
{
	if (isDone()) {
		return;
	}

	thread = std::jthread([&]() {
		exec();
		done = true;
		});
}

void BackgroundThread::terminate()
{
	terminationFlag_ = 1;
	thread.request_stop();
	waitUntilDone();
}

void BackgroundThread::waitUntilDone()
{
	if (!thread.joinable()) {
		return;
	}
	thread.join();
}

bool BackgroundThread::isDone() const
{
	return done;
}

}; // namespace Vortex
