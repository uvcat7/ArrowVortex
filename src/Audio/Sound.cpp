#include <Precomp.h>

#include <Core/Utils/Unicode.h>
#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/Log.h>
#include <Core/System/File.h>
#include <Core/System/Thread.h>
#include <Core/System/System.h>

#include <Audio/Sound.h>
#include <Audio/Formats.h>

#include <Vortex/View/Hud.h>

namespace AV {

using namespace std;
using namespace Util;

typedef AudioFormats::LoadResult (*AudioLoadFunction)(const FilePath& path);

static int Int24(const uint8_t* v)
{
	if (v[2] & 0x80)
	{
		return ((0xff << 24) | (v[2] << 16) | (v[1] << 8) | v[0]);
	}
	else
	{
		return ((v[2] << 16) | (v[1] << 8) | v[0]);
	}
}

static void ConvertSamples(
	size_t numFrames, int16_t* dstL, int16_t* dstR, const void* source, int numChannels, int bytesPerSample)
{
	if (bytesPerSample == 2)
	{
		const int16_t* src = (const int16_t*)source;
		if (numChannels == 1)
		{
			memcpy(dstL, src, numFrames * 2);
			memcpy(dstR, src, numFrames * 2);
		}
		else
		{
			for (size_t i = 0; i < numFrames; ++i, ++dstL, ++dstR, src += numChannels)
			{
				*dstL = src[0];
				*dstR = src[1];
			}
		}
	}
	else
	{
		const uint8_t* l = (const uint8_t*)source;
		const uint8_t* r = l + ((numChannels > 1) ? bytesPerSample : 0);
		const int ofs = bytesPerSample * numChannels;
		if (bytesPerSample == 1)
		{
			for (size_t i = 0; i < numFrames; ++i, ++dstL, ++dstR, l += ofs, r += ofs)
			{
				*dstL = (int16_t)(((int)*l - 128) * 256);
				*dstR = (int16_t)(((int)*r - 128) * 256);
			}
		}
		else if (bytesPerSample == 3)
		{
			for (size_t i = 0; i < numFrames; ++i, ++dstL, ++dstR, l += ofs, r += ofs)
			{
				*dstL = (int16_t)(Int24(l) / 256);
				*dstR = (int16_t)(Int24(r) / 256);
			}
		}
	}
}

static AudioLoadFunction GetLoader(stringref format)
{
	if (format == "ogg")
		return AudioFormats::loadOgg;

	if (format == "mp3")
		return AudioFormats::loadMp3;

	if (format == "wav")
		return AudioFormats::loadWav;

	return nullptr;
}

// =====================================================================================================================
// Loader

static constexpr int BufferSize = 1024;

class Sound::Thread : public BackgroundThread
{
public:
	~Thread();
	Thread(Sound* sound, shared_ptr<SoundSource> source);

	void exec();
	bool readBlock();
	void cleanup();
	uchar progress() { return myProgress; }
	double elapsedTime() { return System::getElapsedTime() - myStartTime; }

private:
	shared_ptr<SoundSource> mySource;
	Sound* mySound;
	short* myBuffer;
	size_t myCurrentFrame;
	size_t myReservedFrames;
	uchar myProgress;
	double myStartTime;
};

Sound::Thread::~Thread()
{
	terminate();
	cleanup();
}

Sound::Thread::Thread(Sound* sound, shared_ptr<SoundSource> source)
{
	myStartTime = System::getElapsedTime();

	mySound = sound;
	mySource = source;

	int bytesPerFrame = source->getNumChannels() * source->getBytesPerSample();
	myBuffer = (short*)malloc(BufferSize * bytesPerFrame);

	myCurrentFrame = 0;
	myReservedFrames = 0;
	myProgress = 0;
}

void Sound::Thread::exec()
{
	while (readBlock());
	cleanup();
}

bool Sound::Thread::readBlock()
{
	if (myTerminateFlag) return false;

	int srcChannels = mySource->getNumChannels();
	auto framesRead = mySource->readFrames(BufferSize, myBuffer);

	if (mySound->myIsAllocated)
	{
		framesRead = min(framesRead, mySound->myNumFrames - myCurrentFrame);
	}

	if (framesRead > 0)
	{
		if (!mySound->myIsAllocated)
		{
			if (myCurrentFrame + framesRead > myReservedFrames)
			{
				myReservedFrames = max(myReservedFrames * 2, myCurrentFrame + framesRead);

				short* newBufferL = (short*)realloc(mySound->mySamplesL, myReservedFrames * sizeof(short));
				short* newBufferR = (short*)realloc(mySound->mySamplesR, myReservedFrames * sizeof(short));

				if (newBufferL && newBufferR)
				{
					mySound->mySamplesL = newBufferL;
					mySound->mySamplesR = newBufferR;
				}
				else
				{
					Hud::error("Insufficient memory to load the entire audio file.");

					if (newBufferL) mySound->mySamplesL = newBufferL;
					if (newBufferR) mySound->mySamplesR = newBufferR;

					framesRead = 0;
				}
			}
			mySound->myNumFrames += framesRead;
		}

		auto dstL = mySound->mySamplesL + myCurrentFrame;
		auto dstR = mySound->mySamplesR + myCurrentFrame;

		ConvertSamples(framesRead, dstL, dstR, myBuffer,
			mySource->getNumChannels(), mySource->getBytesPerSample());

		myCurrentFrame += framesRead;

		if (mySound->myIsAllocated)
		{
			myProgress = (uchar)(100 * myCurrentFrame / mySound->myNumFrames);
		}
	}

	if (framesRead == 0)
	{
		mySound->myIsAllocated = true;
		mySound->myIsCompleted = true;
	}

	return (framesRead > 0);
}

void Sound::Thread::cleanup()
{
	free(myBuffer);
	myBuffer = nullptr;
}

// =====================================================================================================================
// Sound

Sound::Sound()
	: myThread(nullptr)
	, mySamplesL(nullptr)
	, mySamplesR(nullptr)
{
	clear();
}

Sound::~Sound()
{
	clear();
}

void Sound::clear()
{
	Util::reset(myThread);

	if (mySamplesL)
	{
		free(mySamplesL);
		mySamplesL = nullptr;
	}

	if (mySamplesR)
	{
		free(mySamplesR);
		mySamplesR = nullptr;
	}

	myNumFrames = 0;
	myFrequency = 44100;
	myIsAllocated = true;
	myIsCompleted = true;
}

Sound::LoadResult Sound::load(const FilePath& path, bool threaded)
{
	clear();

	shared_ptr<SoundSource> source;

	// Call the load function associated with the extension.
	auto ext = path.extension();
	auto loader = GetLoader(ext);
	if (loader)
	{
		auto result = loader(path);
		source = result.audio;
		if (!source && !result.error.empty())
		{
			Log::blockBegin("Could not load audio file.");
			Log::error(format("file: {}", path.str));
			Log::error(format("reason: {}", result.error));
			Log::blockEnd();
		}
	}
	else
	{
		Log::blockBegin("Could not load audio file.");
		Log::error(format("file: {}", path.str));
		Log::error("reason: unknown audio format");
		Log::blockEnd();
		return LoadResult::UnknownFormat;
	}

	// The source taken ownership of the file, but if we have no source, then we delete the file.
	if (!source)
		return LoadResult::Failure;

	// Start a thread that reads samples from the source.
	myFrequency = source->getFrequency();
	myNumFrames = source->getNumFrames();
	myIsAllocated = false;
	myIsCompleted = false;

	// If the number of frames is known beforehand, we can pre-allocate the buffer.
	if (myNumFrames > 0)
	{
		size_t numBytes = (size_t)myNumFrames * sizeof(short);

		if (threaded)
		{
			mySamplesL = (short*)calloc(numBytes, 1);
			mySamplesR = (short*)calloc(numBytes, 1);
		}
		else
		{
			mySamplesL = (short*)malloc(numBytes);
			mySamplesR = (short*)malloc(numBytes);
		}

		if (!mySamplesL || !mySamplesR)
		{
			Hud::error("Insufficient memory to load the entire audio file.");

			Log::blockBegin("Could not load audio file.");
			Log::error(format("file: {}", path.str));
			Log::error("reason: insufficient memory");
			Log::error(format("size: {} MB", numBytes / 1'000'000));
			Log::blockEnd();

			clear();
			return LoadResult::NotEnoughMemory;
		}

		myIsAllocated = true;
	}

	// Start a sample reading thread.
	if (threaded)
	{
		myThread = new Sound::Thread(this, source);
		myThread->run();
	}
	else
	{
		Sound::Thread thread(this, source);
		thread.exec();
	}

	return LoadResult::Success;
}

int Sound::getLoadingProgress() const
{
	return myThread ? myThread->progress() : 100;
}

double Sound::getLoadingTime() const
{
	return myThread ? myThread->elapsedTime() : 0.0;
}

string Sound::find(const DirectoryPath& dir)
{
	auto files = FileSystem::listFiles(dir, false, "ogg");
	if (files.size())
		return files[0];

	files = FileSystem::listFiles(dir, false, "mp3");
	if (files.size())
		return files[0];

	files = FileSystem::listFiles(dir, false, "wav");
	if (files.size())
		return files[0];

	return string();
}

} // namespace AV
