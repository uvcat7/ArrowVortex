#include <Editor/Sound.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fstream>

#include <Core/Utils.h>

#include <System/File.h>

namespace Vortex {
namespace {

#pragma pack(1)
struct WaveHeader {
    uint8_t chunkId[4];  // "RIFF"
    uint32_t chunkSize;
    uint8_t format[4];  // "WAVE"
};
struct WaveFormat {
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};
struct WaveData {
    uint8_t chunkId[4];
    uint32_t chunkSize;
};
#pragma pack()

struct WavLoader : public SoundSource {
    int getFrequency() override { return frequency; }
    int getNumFrames() override { return numFrames; }
    int getNumChannels() override { return numChannels; }
    int getBytesPerSample() override { return bytesPerSample; }
    int readFrames(int frames, short* buffer) override;

    std::ifstream file;
    int numFrames;
    int numFramesLeft;
    int numChannels;
    int bytesPerSample;
    int frequency;
};

int WavLoader::readFrames(int frames, short* buffer) {
    int numFramesToRead = std::min(numFramesLeft, frames);
    numFramesLeft -= numFramesToRead;

    std::streamsize bytesPerFrame =
        static_cast<std::streamsize>(bytesPerSample) * numChannels;
    file.read(reinterpret_cast<char*>(buffer), bytesPerFrame * numFramesToRead);
    return static_cast<int>(file.gcount() / bytesPerFrame);
}

};  // anonymous namespace

SoundSource* LoadWav(std::ifstream&& file, std::string& title,
                     std::string& artist) {
    // Read the wave header.
    WaveHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (file.fail() || memcmp(header.chunkId, "RIFF", 4) != 0 ||
        memcmp(header.format, "WAVE", 4) != 0) {
        return nullptr;
    }

    // Find format tag.
    WaveData data;
    while (true) {
        file.read(reinterpret_cast<char*>(&data), sizeof(WaveData));
        if (file.fail()) return nullptr;
        if (memcmp(data.chunkId, "fmt ", 4) == 0) break;
        file.ignore(data.chunkSize);
    }

    // Read format.
    WaveFormat format;
    file.read(reinterpret_cast<char*>(&format), sizeof(format));
    if (file.fail() || format.audioFormat != 1 || format.sampleRate == 0 ||
        format.numChannels == 0 ||
        (format.bitsPerSample != 8 && format.bitsPerSample != 16 &&
         format.bitsPerSample != 24)) {
        return nullptr;
    }

    // Skip over additional parameters at the end of the format chunk.
    if (data.chunkSize > 16) {
        size_t extraBytes = static_cast<size_t>(data.chunkSize) - 16;
        file.ignore(extraBytes);
    }

    // Read the start of the data chunk.
    while (true) {
        file.read(reinterpret_cast<char*>(&data), sizeof(WaveData));
        if (file.fail()) return nullptr;
        if (memcmp(data.chunkId, "data", 4) == 0) break;
        file.ignore(data.chunkSize);
    }

    // Create a wav loader that will read the contents of the data chunk.
    WavLoader* loader = new WavLoader;

    loader->frequency = format.sampleRate;
    loader->numChannels = format.numChannels;
    loader->bytesPerSample = format.bitsPerSample / 8;
    loader->numFrames =
        data.chunkSize / (loader->bytesPerSample * loader->numChannels);
    loader->numFramesLeft = loader->numFrames;
    loader->file = std::move(file);

    return loader;
}

};  // namespace Vortex
