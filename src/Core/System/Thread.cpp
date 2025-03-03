#include <Precomp.h>

#include <Core/Utils/Util.h>

#include <Core/System/Thread.h>

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#define AlignedMalloc(type) ((type*)_aligned_malloc(sizeof(type), 16))
#define AlignedFree(ptr)    if (ptr) {_aligned_free(ptr); ptr = nullptr;}

namespace AV {

using namespace Util;

// =====================================================================================================================
// Thread :: Background thread.

#define BTDATA ((BackgroundThreadData*)myData)

struct BackgroundThreadData
{
	BackgroundThread* owner;
	HANDLE handle;
	uchar done;
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
	myTerminateFlag = 0;
	data->owner = this;
	data->done = 0;
	data->handle = 0;
	myData = data;
}

BackgroundThread::~BackgroundThread()
{
	terminate();
	delete BTDATA;
}

void BackgroundThread::run()
{
	if (BTDATA->handle == 0)
	{
		BTDATA->done = 0;
		BTDATA->handle = CreateThread(0, 0, BackgroundThreadFunc, BTDATA, 0, 0);
	}
}

void BackgroundThread::terminate()
{
	if (BTDATA->handle)
	{
		myTerminateFlag = 1;
		waitUntilDone();
	}
}

void BackgroundThread::waitUntilDone()
{
	if (BTDATA->handle)
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

// =====================================================================================================================
// Thread :: Parallel threads.

struct ParallelThreadsShared
{
	ParallelThreads* owner;
	LONG size, *counter;
};

struct ParallelThreadsData
{
	ParallelThreadsShared* shared;
	uint index;
};

static DWORD WINAPI ParallelThreadsFunc(LPVOID lparam)
{
	auto data = (ParallelThreadsData*)lparam;
	while (true)
	{
		long pos = InterlockedDecrement(data->shared->counter);
		if (pos < 0) break;
		int item = int(data->shared->size - 1 - pos);
		data->shared->owner->execute(item, data->index);
	}
	return 0;
}

ParallelThreads::ParallelThreads()
{
}

ParallelThreads::~ParallelThreads()
{
}

int ParallelThreads::concurrency()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return clamp<uint>(sysinfo.dwNumberOfProcessors, 1, 64);
}

void ParallelThreads::run(int numItems, int numThreads)
{
	if (numItems <= 0 || numThreads <= 0) return;

	ParallelThreadsShared shared;
	shared.counter = AlignedMalloc(LONG);
	shared.size = *shared.counter = numItems;
	shared.owner = this;

	ParallelThreadsData* threads;
	HANDLE* handles;
	threads = new ParallelThreadsData[numThreads];
	handles = new HANDLE[numThreads];

	for (int i = 0; i < numThreads; ++i)
	{
		threads[i].shared = &shared;
		threads[i].index = i;
		handles[i] = CreateThread(0, 0, ParallelThreadsFunc, threads + i, 0, 0);
	}

	WaitForMultipleObjects(numThreads, handles, TRUE, INFINITE);
	for (int i = 0; i < numThreads; ++i)
	{
		CloseHandle(handles[i]);
	}

	delete[] threads;
	delete[] handles;
	AlignedFree(shared.counter);
}

// =====================================================================================================================
// Thread :: Critical section.

CriticalSection::CriticalSection()
	: myHandle(malloc(sizeof(CRITICAL_SECTION)))
{
	InitializeCriticalSection((LPCRITICAL_SECTION)myHandle);
}

CriticalSection::~CriticalSection()
{
	free(myHandle);
}

void CriticalSection::lock()
{
	EnterCriticalSection((LPCRITICAL_SECTION)myHandle);
}

void CriticalSection::unlock()
{
	LeaveCriticalSection((LPCRITICAL_SECTION)myHandle);
}

} // namespace AV
