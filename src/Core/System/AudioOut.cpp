#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/System/AudioOut.h>
#include <Core/System/Debug.h>
#include <Core/System/Log.h>

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "mmsystem.h"

namespace AV {

using namespace std;

// =====================================================================================================================
// Static data

static constexpr int WaveOutChannels = 2;
static constexpr int WaveOutBlocks = 8;
static constexpr int WaveOutBlockFrames = 8192;
static constexpr int WaveOutBlockSize = sizeof(short) * WaveOutChannels * WaveOutBlockFrames;

struct ThreadEvent
{
	ThreadEvent() { handle = CreateEvent(nullptr, FALSE, FALSE, nullptr); }
	~ThreadEvent() { CloseHandle(handle); }
	operator HANDLE() { return handle; }
	HANDLE handle;
};

enum class ThreadEvents
{
	Kill = WAIT_OBJECT_0 + 0,
	Resume = WAIT_OBJECT_0 + 1,
	Pause = WAIT_OBJECT_0 + 2,
	WriteBlock = WAIT_OBJECT_0 + 3,
};

namespace AudioOut
{
	struct State
	{
		int frequency = 0;
		int freeBlockIndex = 0;
	
		BYTE* blockMemory = nullptr;
		WAVEHDR headers[WaveOutBlocks] = {};
		HWAVEOUT waveout = 0;
		HANDLE thread = 0;
	
		ThreadEvent killThread;
		ThreadEvent pauseThread;
		ThreadEvent resumeThread;
		ThreadEvent threadPaused;
		ThreadEvent writeBlock;
	
		volatile LONG freeBlocks = 0;
	
		bool isOpened = false;
		bool isPaused = true;
	
		Mixer* mixer = nullptr;
	};
	static State* state = nullptr;
}
using AudioOut::state;

// =====================================================================================================================
// Wave out thread.

static void CALLBACK WaveOutCallback(HWAVEOUT, UINT msg, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
	if (msg == WOM_DONE)
	{
		InterlockedIncrement(&state->freeBlocks);
		SetEvent(state->writeBlock);
	}
}

static DWORD WINAPI WaveOutThread(LPVOID)
{
	const HANDLE inputs[] =
	{
		state->killThread,
		state->resumeThread,
		state->pauseThread,
		state->writeBlock
	};
	while (true)
	{
		// Wait for a thread event.
		auto id = (ThreadEvents)WaitForMultipleObjects(4, inputs, FALSE, INFINITE);
		if (id == ThreadEvents::Kill)
		{
			return 0;
		}
		else if (id == ThreadEvents::Pause)
		{
			SetEvent(state->threadPaused);
			id = (ThreadEvents)WaitForMultipleObjects(2, inputs, FALSE, INFINITE);
			if (id == ThreadEvents::Kill) return 0;
		}
		else if (id == ThreadEvents::WriteBlock)
		{
			while (state->freeBlocks > 0)
			{
				LONG result = InterlockedDecrement(&state->freeBlocks);
				if (result < 0) break;

				// Get the next free buffer block.
				BYTE* samples = state->blockMemory + state->freeBlockIndex * WaveOutBlockSize;
				WAVEHDR* header = state->headers + state->freeBlockIndex;
				state->freeBlockIndex = (state->freeBlockIndex + 1) % WaveOutBlocks;

				// Send the filled block to wave out.
				state->mixer->mix((short*)samples, WaveOutBlockFrames);
				waveOutWrite(state->waveout, header, sizeof(WAVEHDR));
			}
		}
	}
	return 0;
}

// =====================================================================================================================
// Initialization

void AudioOut::initialize()
{
	state = new State();

	state->blockMemory = (BYTE*)_aligned_malloc(WaveOutBlockSize * WaveOutBlocks, 16);
	for (int i = 0; i < WaveOutBlocks; ++i)
	{
		WAVEHDR* header = state->headers + i;
		memset(header, 0, sizeof(WAVEHDR));
	}
}

void AudioOut::deinitialize()
{
	AudioOut::close();

	_aligned_free(state->blockMemory);

	Util::reset(state);
}

// =====================================================================================================================
// Open and close.

bool AudioOut::open(Mixer* mixer, int sampleRate)
{
	if (state->isOpened) close();

	// Try to open the waveout device.
	WAVEFORMATEX wfex = {
		.wFormatTag = WAVE_FORMAT_PCM,
		.nChannels = WaveOutChannels,
		.nSamplesPerSec = (DWORD)sampleRate,
		.nAvgBytesPerSec = sizeof(short) * WaveOutChannels * sampleRate,
		.nBlockAlign = sizeof(short) * WaveOutChannels,
		.wBitsPerSample = sizeof(short) * 8,
		.cbSize = 0,
	};

	MMRESULT res = waveOutOpen(&state->waveout, WAVE_MAPPER, &wfex,
		(DWORD_PTR)(&WaveOutCallback), 0, CALLBACK_FUNCTION);

	if (res != MMSYSERR_NOERROR)
	{
		Log::error(format("Failed to open wave out: {}", res));
		close();
		return false;
	}

	// Create the output buffer blocks.
	for (int i = 0; i < WaveOutBlocks; ++i)
	{
		WAVEHDR* header = state->headers + i;

		memset(header, 0, sizeof(WAVEHDR));
		header->lpData = (LPSTR)(state->blockMemory + WaveOutBlockSize * i);
		header->dwBufferLength = WaveOutBlockSize;
		header->dwUser = i;

		MMRESULT res = waveOutPrepareHeader(state->waveout, header, sizeof(WAVEHDR));
		if (res != MMSYSERR_NOERROR)
		{
			Log::error(format("Could not prepare waveout header: {}", res));
			close();
			return false;
		}
	}

	state->frequency = sampleRate;
	state->mixer = mixer;
	state->isOpened = true;

	// Start the wave out update thread.
	state->thread = CreateThread(nullptr, 0,
		(LPTHREAD_START_ROUTINE)WaveOutThread, nullptr, 0, nullptr);

	return true;
}

void AudioOut::close()
{
	if (state->thread)
	{
		SetEvent(state->killThread);
		LONG err = WaitForSingleObject(state->thread, INFINITE);
		if (err) Log::error(format("failed to close audio thread: {}", err));
		CloseHandle(state->thread);
		state->thread = 0;
	}
	if (state->waveout)
	{
		LONG err = waveOutReset(state->waveout);
		if (err) Log::error(format("failed to reset wave out: {}", err));

		for (int i = 0; i < WaveOutBlocks; ++i)
		{
			WAVEHDR* header = state->headers + i;
			if (header->dwBufferLength > 0)
			{
				LONG err = waveOutUnprepareHeader(state->waveout, state->headers + i, sizeof(WAVEHDR));
				if (err) Log::error(format("failed to unprepare wave out header: {}", err));
				memset(header, 0, sizeof(WAVEHDR));
			}
		}

		err = waveOutClose(state->waveout);
		if (err) Log::error(format("failed to close wave out: {}", err));
		state->waveout = 0;
	}

	state->freeBlockIndex = 0;
	state->freeBlocks = 0;
	state->isOpened = false;
	state->isPaused = true;
}

// =====================================================================================================================
// Pause and resume.

void AudioOut::pause()
{
	if (state->isOpened && !state->isPaused)
	{
		SetEvent(state->pauseThread);
		WaitForSingleObject(state->threadPaused, INFINITE);
		waveOutReset(state->waveout);
		state->isPaused = true;
	}
}

void AudioOut::resume()
{
	if (state->isOpened && state->isPaused)
	{
		state->freeBlockIndex = 0;
		state->freeBlocks = WaveOutBlocks;
		SetEvent(state->resumeThread);
		waveOutRestart(state->waveout);
		SetEvent(state->writeBlock);
		state->isPaused = false;
	}
}

} // namespace AV