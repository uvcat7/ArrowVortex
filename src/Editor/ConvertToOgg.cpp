#include <Editor/ConvertToOgg.h>
#include <Editor/Music.h>

#include <Core/Utils.h>
#include <Core/StringUtils.h>

#include <System/System.h>

#include <stdint.h>
#include <string.h>

namespace Vortex {

namespace {

#pragma pack(1)
struct WaveHeader {
    uint8_t chunkId[4];
    uint32_t chunkSize;
    uint8_t format[4];
    uint8_t subchunk1Id[4];
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint8_t subchunk2Id[4];
    uint32_t subchunk2Size;
};
#pragma pack()

void WriteWaveHeader(WaveHeader* out, int numFrames, int samplerate) {
    memcpy(out->subchunk1Id, "fmt ", 4);
    out->subchunk1Size = 16;
    out->audioFormat = 1;
    out->numChannels = 2;
    out->sampleRate = samplerate;
    out->byteRate = samplerate * 4;
    out->blockAlign = 4;
    out->bitsPerSample = 16;

    memcpy(out->subchunk2Id, "data", 4);
    out->subchunk2Size = numFrames * 4;

    memcpy(out->chunkId, "RIFF", 4);
    out->chunkSize = 36 + out->subchunk2Size;
    memcpy(out->format, "WAVE", 4);
}

struct OggConversionPipe : public System::CommandPipe {
    int read() {
        if (terminateFlag.stop_requested()) {
            return 0;
        }
        if (firstChunk) {
            firstChunk = false;
            return sizeof(WaveHeader);
        }
        int numFrames = min(framesLeft, 4096);
        for (int i = 0, p = 0; i < numFrames; ++i) {
            samples[p++] = *srcL++;
            samples[p++] = *srcR++;
        }
        framesLeft -= numFrames;
        *progress = (uint8_t)(100 - (uint64_t)(100 * framesLeft / totalFrames));
        return numFrames * 4;
    }
    uint8_t* progress;
    bool firstChunk;
    int totalFrames, framesLeft;
    const short *srcL, *srcR;
    short samples[4096 * 2];
    std::stop_token terminateFlag;
};

};  // namespace

OggConversionThread::OggConversionThread() { progress = 0; }

void OggConversionThread::exec() {
    const Sound& music = gMusic->getSamples();

    // Create a pipe that feeds audio data to oggenc.
    auto pipe = new OggConversionPipe;
    pipe->totalFrames = pipe->framesLeft = music.getNumFrames();
    pipe->srcL = music.samplesL();
    pipe->srcR = music.samplesR();
    pipe->firstChunk = true;
    pipe->progress = &progress;
    pipe->terminateFlag = getStopToken();
    WriteWaveHeader((WaveHeader*)(pipe->samples), music.getNumFrames(),
                    music.getFrequency());

    // Encode the PCM file with the oggenc2 command line utility.
    std::string cmd = Str::fmt("oggenc2.exe -q6 -o \"%1\" -").arg(outPath);

    // Call oggenc2 with the command line parameters.
    if (!gSystem->runSystemCommand(cmd, pipe, pipe->samples)) {
        error = "could not run oggenc2.exe";
    }
    delete pipe;
}

};  // namespace Vortex