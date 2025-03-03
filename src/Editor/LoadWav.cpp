#include <Editor/Sound.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <Core/Utils.h>

#include <System/File.h>

namespace Vortex {
namespace {

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

struct WavLoader : public SoundSource
{
	~WavLoader();

	int getFrequency() override { return frequency; }
	int getNumFrames() override { return numFrames; }
	int getNumChannels() override { return numChannels; }
	int getBytesPerSample() override { return bytesPerSample; }
	int readFrames(int frames, short* buffer) override;

	FileReader* file;
	int numFrames;
	int numFramesLeft;
	int numChannels;
	int bytesPerSample;
	int frequency;
};

WavLoader::~WavLoader()
{
	delete file;
}

int WavLoader::readFrames(int frames, short* buffer)
{
	int numFramesToRead = min(numFramesLeft, frames);
	numFramesLeft -= numFramesToRead;
	return file->read(buffer, bytesPerSample * numChannels, numFramesToRead);
}

}; // anonymous namespace

SoundSource* LoadWav(FileReader* file, String& title, String& artist)
{
	// Read the wave header.
	WaveHeader header;
	if(file->read(&header, sizeof(WaveHeader), 1) == 0
		|| memcmp(header.chunkId, "RIFF", 4) != 0
		|| memcmp(header.format, "WAVE", 4) != 0
		|| memcmp(header.subChunkId, "fmt ", 4) != 0
		|| header.audioFormat != 1
		|| header.sampleRate == 0
		|| header.numChannels == 0
		|| (header.bps != 8 && header.bps != 16 && header.bps != 24))
	{
		return nullptr;
	}

	// Skip over additional parameters at the end of the format chunk.
	file->skip(header.subChunkSize - 16);

	// Read the start of the data chunk.
	WaveData data;
	if(file->read(&data, sizeof(WaveData), 1) == 0
		|| memcmp(data.chunkId, "data", 4) != 0)
	{
		return nullptr;
	}

	// Create a wav loader that will read the contents of the data chunk.
	WavLoader* loader = new WavLoader;

	loader->frequency = header.sampleRate;
	loader->numChannels = header.numChannels;
	loader->bytesPerSample = header.bps / 8;
	loader->numFrames = data.chunkSize / (loader->bytesPerSample * loader->numChannels);
	loader->numFramesLeft = loader->numFrames;
	loader->file = file;

	return loader;
}

}; // namespace Vortex
