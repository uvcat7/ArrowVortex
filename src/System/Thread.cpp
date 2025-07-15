#include <System/Thread.h>

#include <Core/Utils.h>
#include <Core/AlignedMemory.h>

#include <vector>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

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

// ================================================================================================
// ParallelThreads.

struct ParallelThreadsShared
{
	ParallelThreads* owner;
	LONG size, *counter;
};

struct ParallelThreadsData
{
	ParallelThreadsShared* shared;
	int index;
};

static DWORD WINAPI ParallelThreadsFunc(LPVOID lparam)
{
	auto data = (ParallelThreadsData*)lparam;
	while(true)
	{
		long pos = InterlockedDecrement(data->shared->counter);
		if(pos < 0) break;
		int item = (int)(data->shared->size - 1 - pos);
		data->shared->owner->exec(item, data->index);
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
	static int s_num_of_procs = 0;
	if (s_num_of_procs == 0)
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		s_num_of_procs = std::clamp<int>(sysinfo.dwNumberOfProcessors, 1, 16);
	}
	return s_num_of_procs;
}

void ParallelThreads::run(int numItems, int numThreads)
{
	if(numItems <= 0 || numThreads <= 0) return;

	ParallelThreadsShared shared;
	shared.counter = AlignedMalloc<LONG>(1);
	shared.size = *shared.counter = numItems;
	shared.owner = this;

	std::vector<ParallelThreadsData> threads(numThreads);
	std::vector<HANDLE> handles(numThreads);

	for(int i = 0; i < numThreads; ++i)
	{
		threads[i].shared = &shared;
		threads[i].index = i;
		handles[i] = CreateThread(0, 0, ParallelThreadsFunc, &threads[i], 0, 0);
	}

	WaitForMultipleObjects(numThreads, handles.data(), TRUE, INFINITE);
	for(HANDLE handle : handles)
	{
		CloseHandle(handle);
	}

	if (shared.counter) {
		_aligned_free(shared.counter);
		shared.counter = nullptr;
	};
}

// ================================================================================================
// CriticalSection.

CriticalSection::CriticalSection()
	: criticalSectionHandle(malloc(sizeof(CRITICAL_SECTION)))
{
	if (!criticalSectionHandle)
		throw std::bad_alloc();

	InitializeCriticalSection((LPCRITICAL_SECTION)criticalSectionHandle);
}

CriticalSection::~CriticalSection()
{
	if (criticalSectionHandle) {
		DeleteCriticalSection((LPCRITICAL_SECTION)criticalSectionHandle);
		free(criticalSectionHandle);
		criticalSectionHandle = nullptr;
	}
}

void CriticalSection::lock()
{
	if (criticalSectionHandle)
		EnterCriticalSection((LPCRITICAL_SECTION)criticalSectionHandle);
}

void CriticalSection::unlock()
{
	if (criticalSectionHandle)
		LeaveCriticalSection((LPCRITICAL_SECTION)criticalSectionHandle);
}

}; // namespace Vortex
