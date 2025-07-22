#include <System/Thread.h>

#include <Core/Utils.h>
#include <Core/AlignedMemory.h>

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef min
#undef max

namespace Vortex {

// ================================================================================================
// BackgroundThread.

#define BTDATA ((BackgroundThreadData*)data_)

struct BackgroundThreadData
{
	BackgroundThread* owner;
	HANDLE handle;
	uint8_t done;
};

struct BackgroundThreadParam
{
	BackgroundThread* thread;
	BackgroundThreadData* data;
};

static DWORD WINAPI BackgroundThreadFunc(LPVOID lparam)
{
	auto data = (BackgroundThreadData*)lparam;
	data->owner->exec();
 	data->done = 1;
	return 0;
}

BackgroundThread::BackgroundThread()
{
	auto data = new BackgroundThreadData;
	terminationFlag_ = 0;
	data->owner = this;
	data->done = 0;
	data->handle = 0;
	data_ = data;
}

BackgroundThread::~BackgroundThread()
{
	terminate();
	delete BTDATA;
}

void BackgroundThread::start()
{
	if(BTDATA->handle == 0 && BTDATA->done == 0)
	{
		BTDATA->handle = CreateThread(0, 0, BackgroundThreadFunc, BTDATA, 0, 0);
	}
}

void BackgroundThread::terminate()
{
	if(BTDATA->handle)
	{
		terminationFlag_ = 1;
		waitUntilDone();
	}
}

void BackgroundThread::waitUntilDone()
{
	if(BTDATA->handle)
	{
		WaitForSingleObject(BTDATA->handle, INFINITE);
		CloseHandle(BTDATA->handle);
		BTDATA->handle = 0;
	}
}

bool BackgroundThread::isDone() const
{
	return BTDATA->done != 0;
}

}; // namespace Vortex
