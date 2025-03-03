#pragma once

#include <Core/System/File.h>

namespace AV {

// In the following context, a sample refers to a single value.
// A frame refers to a set samples, one for each audio channel.

struct SoundSource
{
	virtual ~SoundSource() {}

	virtual size_t getNumFrames() = 0;    // Number of frames in the entire signal.
	virtual int getFrequency() = 0;      // Number of frames per second.
	virtual int getNumChannels() = 0;    // Number of samples per frame.
	virtual int getBytesPerSample() = 0; // Size of a sample value in bytes.

	// Writes audio frames into the buffer until either the buffer is filled
	// or the source end is reached. Returns the number of frames written.
	virtual size_t readFrames(size_t numFrames, short* buffer) = 0;
};

class Sound
{
public:
	struct Trim { int framePos, numFrames; };
	struct Stretch { int framePos, numFrames, stretchMultiplier; };

	enum class LoadResult
	{
		Success,
		Failure,
		UnknownFormat,
		NotEnoughMemory,
	};

	Sound();
	~Sound();

	void clear();

	// Destroys the current sample data and starts loading audio from a file.
	// When threaded, "isAllocated" and "isCompleted" can be used to check loading progress.
	LoadResult load(const FilePath& path, bool threaded);

	// Returns the number of frames per second.
	int getFrequency() const { return myFrequency; }

	// Returns the number of frames in the entire signal.
	size_t getNumFrames() const { return myNumFrames; }

	// Returns the samples buffer for the left channel.
	const short* samplesL() const { return mySamplesL; }

	// Returns the sample buffer for the right channel.
	const short* samplesR() const { return mySamplesR; }

	// Returns true if the sample buffers allocation is completed, false otherwise.
	bool isAllocated() const { return myIsAllocated; }

	// Returns true if the sound has finished loading, false otherwise.
	bool isCompleted() const { return myIsCompleted; }

	// Returns the loading progress percentage, or zero if the progress is unknown.
	int getLoadingProgress() const;

	// Returns the startTime elapsed since loading started.
	double getLoadingTime() const;

	// Returns the most recent error that occured during loading.
	const char* lastError() const { return myError; }

	// Returns the filename of the audio file with the highest priority in the given directory.
	static std::string find(const DirectoryPath& dir);

private:
	class Thread;
	Thread* myThread;
	short* mySamplesL;
	short* mySamplesR;
	int myFrequency;
	size_t myNumFrames;
	bool myIsAllocated;
	bool myIsCompleted;
	const char* myError;
};

} // namespace AV
