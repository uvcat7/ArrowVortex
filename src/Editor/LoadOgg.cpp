#include <Core/StringUtils.h>

#include <Editor/Sound.h>

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <errno.h>

#include <cstring>
#include <fstream>

#include <System/File.h>

namespace Vortex {
namespace {

struct OggLoader : public SoundSource {
    ~OggLoader() override;

    int getFrequency() override { return frequency; }
    int getNumFrames() override { return numFrames; }
    int getNumChannels() override { return numChannels; }
    int getBytesPerSample() override { return 2; }

    int readFrames(int frames, short* buffer) override;

    OggVorbis_File* vf;
    int numFrames;
    int numFramesLeft;
    int numChannels;
    int frequency;
    int bitstream;
    std::ifstream file;
};

OggLoader::~OggLoader() {
    if (vf != nullptr) {
        ov_clear(vf);
        delete vf;
    }
}

int OggLoader::readFrames(int frames, short* buffer) {
    int bytesPerFrame = numChannels * 2;
    int bytesRead = ov_read(vf, reinterpret_cast<char*>(buffer),
                            frames * bytesPerFrame, 0, 2, 1, &bitstream);
    if (bytesRead < 0) bytesRead = 0;
    return bytesRead / bytesPerFrame;
}

static size_t OvRead(void* ptr, size_t size, size_t nmemb, void* loader) {
    std::ifstream& file = static_cast<OggLoader*>(loader)->file;
    file.read(static_cast<char*>(ptr), size * nmemb);
    return file.gcount();
}

static int OvSeek(void* loader, ogg_int64_t offset, int whence) {
    std::ifstream& file = static_cast<OggLoader*>(loader)->file;
    file.seekg(offset, static_cast<std::ios::seekdir>(whence));
    return file.fail();
}

static long OvTell(void* loader) {
    std::ifstream& file = static_cast<OggLoader*>(loader)->file;
    return file.tellg();
}

static void ReadComment(const char* str, int len, const char* tag,
                        std::string& out) {
    const char* end = str + len;
    int taglen = static_cast<int>(strlen(tag));
    if (taglen < len && memcmp(str, tag, taglen) == 0) {
        str += taglen;
        while (str != end && (*str == ' ' || *str == '\t')) ++str;
        if (str != end && *str == '=') ++str;
        while (str != end && (*str == ' ' || *str == '\t')) ++str;
        out = std::string(str, static_cast<size_t>(end - str));
    }
}

};  // anonymous namespace.

SoundSource* LoadOgg(std::ifstream&& file, std::string& title,
                     std::string& artist) {
    OggLoader* loader = new OggLoader;
    loader->file = std::move(file);

    // Try to open the ogg-vorbis file.
    OggVorbis_File* vf = new OggVorbis_File;
    int res = ov_open_callbacks(loader, vf, nullptr, 0,
                                ov_callbacks{OvRead, OvSeek, nullptr, OvTell});
    if (res < 0) {
        delete loader;
        delete vf;
        return nullptr;
    }

    // Read the metadata.
    vorbis_comment* comments = ov_comment(vf, -1);
    if (comments) {
        for (int i = 0; i < comments->comments; ++i) {
            const char* str = comments->user_comments[i];
            int len = comments->comment_lengths[i];
            ReadComment(str, len, "ARTIST", artist);
            ReadComment(str, len, "TITLE", title);
        }
    }

    // The file is valid, return the ogg-vorbis loader.
    vorbis_info* info = ov_info(vf, -1);

    loader->vf = vf;
    loader->bitstream = 0;
    loader->frequency = info->rate;
    loader->numChannels = info->channels;
    loader->numFrames = ov_pcm_total(vf, -1);
    loader->numFramesLeft = loader->numFrames;

    return loader;
}

};  // namespace Vortex
