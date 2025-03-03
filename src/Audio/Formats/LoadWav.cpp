#include <Precomp.h>

#include <Audio/Formats.h>

using namespace std;

namespace AV {
namespace LoadWav {

#pragma pack(1)
struct WaveHeader
{
	uint8_t chunkId[4];
	uint32_t chunkSize;
	uint8_t format[4];
	uint8_t subChunkId[4];
	uint32_t subChunkSize;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bps;
};
struct WaveData
{
	uint8_t chunkId[4];
	uint32_t chunkSize;
};
#pragma pack()

class WavSoundSource : public SoundSource
{
public:
	WavSoundSource(shared_ptr<FileReader> file)
		: myFile(file)
	{
	}

	bool decodeHeader()
	{
		WaveHeader header;
		if (myFile->read(&header, sizeof(WaveHeader), 1) == 0
			|| memcmp(header.chunkId, "RIFF", 4) != 0
			|| memcmp(header.format, "WAVE", 4) != 0
			|| memcmp(header.subChunkId, "fmt ", 4) != 0
			|| header.audioFormat != 1
			|| header.sampleRate == 0
			|| header.numChannels == 0
			|| (header.bps != 8 && header.bps != 16 && header.bps != 24))
			return false;

		myFrequency = header.sampleRate;
		myNumChannels = header.numChannels;
		myBytesPerSample = header.bps / 8;

		// Skip over additional parameters at the end of the format chunk.
		myFile->skip(header.subChunkSize - 16);

		// Read the start of the data chunk.
		WaveData data;
		if (myFile->read(&data, sizeof(WaveData), 1) == 0
			|| memcmp(data.chunkId, "data", 4) != 0)
			return false;

		myNumFrames = data.chunkSize / (myBytesPerSample * myNumChannels);
		myNumFramesLeft = myNumFrames;

		return true;
	}

	size_t getNumFrames() override { return myNumFrames; }
	int getFrequency() override { return myFrequency; }
	int getNumChannels() override { return myNumChannels; }
	int getBytesPerSample() override { return myBytesPerSample; }

	size_t readFrames(size_t numFrames, short* buffer) override
	{
		auto numFramesToRead = min(myNumFramesLeft, numFrames);
		myNumFramesLeft -= numFramesToRead;
		return myFile->read(buffer, myBytesPerSample * myNumChannels, numFramesToRead);
	}

private:
	shared_ptr<FileReader> myFile;
	size_t myNumFrames = 0;
	size_t myNumFramesLeft = 0;
	int myNumChannels = 0;
	int myBytesPerSample = 0;
	int myFrequency = 0;
};

} // namespace LoadWav

AudioFormats::LoadResult AudioFormats::loadWav(const FilePath& path)
{
	// Open the file.
	auto file = make_shared<FileReader>();
	if (!file->open(path))
		return { .error = "Could not open file." };

	// Decode the header to check if the file is valid.
	auto audio = make_shared<LoadWav::WavSoundSource>(file);
	if (!audio->decodeHeader())
		return { .error = "Could not decode WAV." };

	return { .audio = audio };
}

} // namespace AV
