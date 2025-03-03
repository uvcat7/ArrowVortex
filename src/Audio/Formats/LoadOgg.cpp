#include <Precomp.h>

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#include <Core/Utils/String.h>
#include <Core/System/File.h>

#include <Audio/Formats.h>

using namespace std;

namespace AV {	
namespace LoadOgg {

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

static void ReadComment(const char* str, int len, const char* tag, string& out)
{
	const char* end = str + len;
	auto taglen = strlen(tag);
	if (taglen < len && memcmp(str, tag, taglen) == 0)
	{
		str += taglen;
		while (str != end && (*str == ' ' || *str == '\t')) ++str;
		if (str != end && *str == '=') ++str;
		while (str != end && (*str == ' ' || *str == '\t')) ++str;
		out = string(str, end - str);
	}
}

struct OggSoundSource : public SoundSource
{
	OggSoundSource(shared_ptr<FileReader> file, shared_ptr<OggVorbis_File> ogg)
		: myFile(file)
		, myOgg(ogg)
	{
		vorbis_info* info = ov_info(ogg.get(), -1);
		myBitstream = 0;
		myFrequency = info->rate;
		myNumChannels = info->channels;
		myNumFrames = (int)ov_pcm_total(ogg.get(), -1);
		myNumFramesLeft = myNumFrames;
	}

	~OggSoundSource()
	{
		ov_clear(myOgg.get());
	}

	size_t getNumFrames() override { return myNumFrames; }
	int getFrequency() override { return myFrequency; }
	int getNumChannels() override { return myNumChannels; }
	int getBytesPerSample() override { return 2; }

	size_t readFrames(size_t frames, short* buffer) override
	{
		int bytesPerFrame = myNumChannels * 2;
		int numBytes = (int)min(frames, (size_t)INT_MAX) * bytesPerFrame;
		auto bytesRead = ov_read(myOgg.get(), (char*)buffer, numBytes, 0, 2, 1, &myBitstream);
		if (bytesRead < 0) bytesRead = 0;
		return bytesRead / bytesPerFrame;
	}

	shared_ptr<FileReader> myFile;
	shared_ptr<OggVorbis_File> myOgg;
	size_t myNumFrames;
	size_t myNumFramesLeft;
	int myNumChannels;
	int myFrequency;
	int myBitstream;
};

} // namespace LoadOgg

AudioFormats::LoadResult AudioFormats::loadOgg(const FilePath& path)
{
	// Open the file.
	auto file = make_shared<FileReader>();
	if (!file->open(path))
		return { .error = "Could not open file." };

	// Create an ogg-vorbis file and set up callbacks.
	auto ogg = make_shared<OggVorbis_File>();
	ov_callbacks callbacks = { LoadOgg::OvRead, LoadOgg::OvSeek, nullptr, LoadOgg::OvTell };
	int res = ov_open_callbacks(file.get(), ogg.get(), nullptr, 0, callbacks);
	if (res < 0)
		return { .error = "Could not decode ogg-vorbis." };

	return { .audio = make_shared<LoadOgg::OggSoundSource>(file, ogg) };
}

AudioFormats::Metadata AudioFormats::loadOggMetadata(const FilePath& path)
{
	AudioFormats::Metadata result;

	FileReader file;
	if (!file.open(path))
		return result;

	OggVorbis_File ogg;
	ov_callbacks callbacks = { LoadOgg::OvRead, LoadOgg::OvSeek, nullptr, LoadOgg::OvTell };
	int res = ov_open_callbacks(&file, &ogg, nullptr, 0, callbacks);
	if (res < 0)
		return result;

	auto comments = ov_comment(&ogg, -1);
	if (comments)
	{
		for (int i = 0; i < comments->comments; ++i)
		{
			const char* str = comments->user_comments[i];
			int len = comments->comment_lengths[i];
			LoadOgg::ReadComment(str, len, "ARTIST", result.artist);
			LoadOgg::ReadComment(str, len, "TITLE", result.title);
		}
	}

	ov_clear(&ogg);
	return result;
}

} // namespace AV
