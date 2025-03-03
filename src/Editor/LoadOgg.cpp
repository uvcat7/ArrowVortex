#include <Core/StringUtils.h>

#include <Editor/Sound.h>

#include <libvorbis/include/ogg/ogg.h>
#include <libvorbis/include/vorbis/vorbisfile.h>
#include <errno.h>

#include <string.h>
#include <ctype.h>

#include <System/File.h>

namespace Vortex {
namespace {

struct OggLoader : public SoundSource
{
	~OggLoader();

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
	FileReader* file;
};

OggLoader::~OggLoader()
{
	if(vf)
	{
		ov_clear(vf);
		delete vf;
	}
	delete file;
}

int OggLoader::readFrames(int frames, short* buffer)
{
	int bytesPerFrame = numChannels * 2;
	int bytesRead = ov_read(vf, (char*)buffer, frames * bytesPerFrame, 0, 2, 1, &bitstream);
	if(bytesRead < 0) bytesRead = 0;
	return bytesRead / bytesPerFrame;
}

static size_t OvRead(void* ptr, size_t size, size_t nmemb, void* file)
{
	return ((FileReader*)file)->read(ptr, size, nmemb);
}

static int OvSeek(void* file, ogg_int64_t offset, int whence)
{
	return ((FileReader*)file)->seek((long)offset, whence);
}

static long OvTell(void* file)
{
	return ((FileReader*)file)->tell();
}

static void ReadComment(const char* str, int len, const char* tag, String& out)
{
	const char* end = str + len;
	int taglen = strlen(tag);
	if(taglen < len && memcmp(str, tag, taglen) == 0)
	{
		str += taglen;
		while(str != end && (*str == ' ' || *str == '\t')) ++str;
		if(str != end && *str == '=') ++str;
		while(str != end && (*str == ' ' || *str == '\t')) ++str;
		Str::assign(out, str, end - str);
	}
}

}; // anonymous namespace.

SoundSource* LoadOgg(FileReader* file, String& title, String& artist)
{
	// Try to open the ogg-vorbis file.
	OggVorbis_File* vf = new OggVorbis_File;
	int res = ov_open_callbacks(file, vf, nullptr, 0, ov_callbacks{OvRead, OvSeek, nullptr, OvTell});
	if(res < 0)
	{
		delete vf;
		return nullptr;
	}

	// Read the metadata.
	vorbis_comment* comments = ov_comment(vf, -1);
	if(comments)
	{
		for(int i = 0; i < comments->comments; ++i)
		{
			const char* str = comments->user_comments[i];
			int len = comments->comment_lengths[i];
			ReadComment(str, len, "ARTIST", artist);
			ReadComment(str, len, "TITLE", title);
		}
	}

	// The file is valid, return the ogg-vorbis loader.
	OggLoader* loader = new OggLoader;
	vorbis_info* info = ov_info(vf, -1);

	loader->vf = vf;
	loader->bitstream = 0;
	loader->frequency = info->rate;
	loader->numChannels = info->channels;
	loader->numFrames = (int)ov_pcm_total(vf, -1);
	loader->numFramesLeft = loader->numFrames;
	loader->file = file;

	return loader;
}

}; // namespace Vortex
