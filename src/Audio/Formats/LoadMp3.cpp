#include <Precomp.h>

#include <libmad/include/mad.h>

#include <Audio/Formats.h>

using namespace std;

namespace AV {
namespace LoadMp3 {

// =====================================================================================================================
// ID3 tag parsing.

typedef ulong id3_length_t;

enum class Id3TagType
{
	None = 0,
	V1,
	V2,
	V2Footer
};

static Id3TagType ID3GetTagtype(const uchar *data, id3_length_t length)
{
	if (length >= 3 &&
		data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
		return Id3TagType::V1;

	if (length >= 10 &&
		((data[0] == 'I' && data[1] == 'D' && data[2] == '3') ||
		(data[0] == '3' && data[1] == 'D' && data[2] == 'I')) &&
		data[3] < 0xff && data[4] < 0xff &&
		data[6] < 0x80 && data[7] < 0x80 && data[8] < 0x80 && data[9] < 0x80)
		return data[0] == 'I' ? Id3TagType::V2 : Id3TagType::V2Footer;

	return Id3TagType::None;
}

static ulong ID3ParseUint(const uchar **ptr, int bytes)
{
	ulong value = 0;
	switch(bytes)
	{
		case 4: value = (value << 8) | *(*ptr)++; [[fallthrough]];
		case 3: value = (value << 8) | *(*ptr)++; [[fallthrough]];
		case 2: value = (value << 8) | *(*ptr)++; [[fallthrough]];
		case 1: value = (value << 8) | *(*ptr)++;
	}
	return value;
}

static ulong ID3ParseSyncsafe(const uchar **ptr, int bytes)
{
	ulong value = 0;
	switch(bytes)
	{
	case 5:
		value = (value << 4) | (*(*ptr)++ & 0x0f); [[fallthrough]];
	case 4:
		value = (value << 7) | (*(*ptr)++ & 0x7f);
		value = (value << 7) | (*(*ptr)++ & 0x7f);
		value = (value << 7) | (*(*ptr)++ & 0x7f);
		value = (value << 7) | (*(*ptr)++ & 0x7f);
	}
	return value;
}

static void ID3ParseHeader(const uchar **ptr, uint *version, int *flags, id3_length_t *size)
{
	*ptr += 3;
	*version = ID3ParseUint(ptr, 2);
	*flags = ID3ParseUint(ptr, 1);
	*size = ID3ParseSyncsafe(ptr, 4);
}

static long ID3TagQuery(const uchar *data, id3_length_t length)
{
	uint version;
	int flags;
	id3_length_t size;
	switch(ID3GetTagtype(data, length))
	{
	case Id3TagType::V1:
		return 128;

	case Id3TagType::V2:
		ID3ParseHeader(&data, &version, &flags, &size);
		if (flags & 0x10 /*ID3_TAG_FLAG_FOOTERPRESENT*/) size += 10;
		return 10 + size;

	case Id3TagType::V2Footer:
		ID3ParseHeader(&data, &version, &flags, &size);
		return -(int)size - 10;

	case Id3TagType::None:
		break;
	}
	return 0;
}

// =====================================================================================================================
// XING header parsing.

struct XingHeader
{
	enum class Type { Xing, Info };

	long flags;				// valid fields (see below).
	unsigned long frames;	// total number of frames.
	unsigned long bytes;	// total number of bytes.
	unsigned char toc[100];	// 100-point seek table.
	long scale;				// ??
	Type type;
};

namespace XingFlag
{
	constexpr long Frames = 1 << 0;
	constexpr long Bytes  = 1 << 1;
	constexpr long TOC    = 1 << 2;
	constexpr long Scale  = 1 << 3;
}

static int XingParse(XingHeader* myXingHeader, struct mad_bitptr ptr, unsigned int bitlen)
{
	const uint XING_MAGIC = (('X' << 24) | ('i' << 16) | ('n' << 8) | 'g');
	const uint INFO_MAGIC = (('I' << 24) | ('n' << 16) | ('f' << 8) | 'o');
	unsigned data;
	if (bitlen < 64)
		goto fail;
	data = mad_bit_read(&ptr, 32);

	if (data == XING_MAGIC)
		myXingHeader->type = XingHeader::Type::Xing;
	else if (data == INFO_MAGIC)
		myXingHeader->type = XingHeader::Type::Info;
	else
		goto fail;

	myXingHeader->flags = mad_bit_read(&ptr, 32);
	bitlen -= 64;

	if (myXingHeader->flags & XingFlag::Frames)
	{
		if (bitlen < 32)
			goto fail;

		myXingHeader->frames = mad_bit_read(&ptr, 32);
		bitlen -= 32;
	}

	if (myXingHeader->flags & XingFlag::Bytes)
	{
		if (bitlen < 32)
			goto fail;

		myXingHeader->bytes = mad_bit_read(&ptr, 32);
		bitlen -= 32;
	}

	if (myXingHeader->flags & XingFlag::TOC)
	{
		if (bitlen < 800)
			goto fail;

		for (int i = 0; i < 100; ++i)
			myXingHeader->toc[i] = (unsigned char)mad_bit_read(&ptr, 8);

		bitlen -= 800;
	}

	if (myXingHeader->flags & XingFlag::Scale)
	{
		if (bitlen < 32)
			goto fail;

		myXingHeader->scale = mad_bit_read(&ptr, 32);
		bitlen -= 32;
	}

	return 0;

fail:
	myXingHeader->flags = 0;
	return -1;
}

static inline short ScaleSample(mad_fixed_t sample)
{
	sample += (1L << (MAD_F_FRACBITS - 16));
	if (sample >= MAD_F_ONE)	sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE) sample = -MAD_F_ONE;
	return (short)(sample >> (MAD_F_FRACBITS + 1 - 16));
}

// =====================================================================================================================
// MP3 sound source.

class Mp3SoundSource : public SoundSource
{
public:
	Mp3SoundSource(shared_ptr<FileReader> file)
		: myFile(file)
	{
		mad_stream_init(&myStream);
		mad_frame_init(&myFrame);
		mad_synth_init(&mySynth);

		memset(myFileBuffer, 0, sizeof(myFileBuffer));
		memset(mySynthBuffer, 0, sizeof(mySynthBuffer));
		synthNumSamples = 0;
		mySynthBufferPos = 0;

		myIsFirstFrame = true;
		myHasXingHeader = false;

		myNumChannels = 0;

		memset(&myXingHeader, 0, sizeof(XingHeader));
	}

	~Mp3SoundSource()
	{
		mad_synth_finish(&mySynth);
		mad_frame_finish(&myFrame);
		mad_stream_finish(&myStream);
	}

	size_t getNumFrames() override { return 0; }
	int getFrequency() override { return myFrequency; }
	int getNumChannels() override { return myNumChannels; }
	int getBytesPerSample() override { return 2; }

	int64_t fillInputBuffer()
	{
		// Mad needs more data from the input file.
		size_t inbytes = 0;
		if (myStream.next_frame != NULL)
		{
			// Pull out remaining data from the last buffer.
			inbytes = myStream.bufend - myStream.next_frame;
			memmove(myFileBuffer, myStream.next_frame, inbytes);
		}

		bool eofBeforeReading = myFile->eof();

		size_t rc = myFile->read(myFileBuffer + inbytes, 1, sizeof(myFileBuffer) - inbytes - MAD_BUFFER_GUARD);
		if (rc < 0) return -1;

		if (myFile->eof() && !eofBeforeReading)
		{
			/* We just reached EOF.  Append MAD_BUFFER_GUARD bytes of NULs to the
			* buffer, to ensure that the last frame is flushed. */
			memset(myFileBuffer + inbytes + rc, 0, MAD_BUFFER_GUARD);
			rc += MAD_BUFFER_GUARD;
		}
		if (rc == 0) return 0;

		inbytes += rc;
		mad_stream_buffer(&myStream, myFileBuffer, (unsigned long)inbytes);
		return rc;
	}

	bool decodeFirstFrame()
	{
		int64_t res = decodeNextFrame();
		if (res <= 0) return false;

		myFrequency = myFrame.header.samplerate;
		myNumChannels = MAD_NCHANNELS(&myFrame.header);

		synthDecodedFrame();

		return true;
	}

	int64_t decodeNextFrame()
	{
		int64_t bytes_read = 0;

		while (true)
		{
			int ret = mad_frame_decode(&myFrame, &myStream);

			if (ret == -1 && (myStream.error == MAD_ERROR_BUFLEN || myStream.error == MAD_ERROR_BUFPTR))
			{
				if (bytes_read > 25000) return -1; // We've read this much without actually getting a frame; error.
				int64_t ret = fillInputBuffer();
				if (ret <= 0) return ret;
				bytes_read += ret;
				continue;
			}

			if (ret == -1 && myStream.error == MAD_ERROR_LOSTSYNC)
			{
				const int tagsize = ID3TagQuery(myStream.this_frame, (id3_length_t)(myStream.bufend - myStream.this_frame));
				if (tagsize)
				{
					mad_stream_skip(&myStream, tagsize);
					bytes_read -= tagsize;
					continue;
				}
			}

			if (ret == -1 && myStream.error == MAD_ERROR_BADDATAPTR)
			{
				// Something's corrupt.  One cause of this is cutting an MP3 in the middle
				// without reencoding; the first two frames will reference data from previous
				// frames that have been removed.  The frame is valid--we can get a header from
				// it, we just can't synth useful data. BASS pretends the bad frames are silent.
				// Emulate that, for compatibility.
				ret = 0; // pretend success.
			}

			if (!ret)
			{
				if (myIsFirstFrame)
				{
					// We're at the beginning. Is this a Xing tag?
					myXingHeader.flags = 0;
					if (XingParse(&myXingHeader, myStream.anc_ptr, myStream.anc_bitlen) == 0)
					{
						if (myXingHeader.type != XingHeader::Type::Info)
						{
							myHasXingHeader = true;
							// The first frame contained a header. Continue searching.
							if (myXingHeader.type == XingHeader::Type::Xing) continue;
						}
					}
					myIsFirstFrame = false;
				}
				return 1;
			}

			if (myStream.error == MAD_ERROR_BADCRC)
			{
				mad_frame_mute(&myFrame);
				mad_synth_mute(&mySynth);
				continue;
			}

			if (!MAD_RECOVERABLE(myStream.error))
			{
				return -1;
			}
		}
	}

	void synthDecodedFrame()
	{
		synthNumSamples = mySynthBufferPos = 0;

		if (MAD_NCHANNELS(&myFrame.header) != myNumChannels) return;

		mad_synth_frame(&mySynth, &myFrame);
		for (int i = 0; i < mySynth.pcm.length; ++i)
		{
			for (int ch = 0; ch < myNumChannels; ++ch)
			{
				short sample = ScaleSample(mySynth.pcm.samples[ch][i]);
				mySynthBuffer[synthNumSamples] = sample;
				++synthNumSamples;
			}
		}
	}

	size_t readFrames(size_t frames, short* buffer) override
	{
		size_t framesWritten = 0;
		while (frames > 0)
		{
			// Copy the synthesized samples that are left in the synth buffer.
			if (synthNumSamples > 0)
			{
				size_t framesToCopy = synthNumSamples / myNumChannels;
				if (framesToCopy > frames) framesToCopy = frames;

				auto samplesToCopy = framesToCopy * myNumChannels;
				auto bytesToCopy = samplesToCopy * sizeof(short);

				memcpy(buffer, mySynthBuffer + mySynthBufferPos, bytesToCopy);

				buffer += samplesToCopy;
				frames -= framesToCopy;
				framesWritten += framesToCopy;
				mySynthBufferPos += samplesToCopy;
				synthNumSamples -= samplesToCopy;
				continue;
			}

			// Decode and synthesize more samples.
			int64_t ret = decodeNextFrame();
			if (ret <= 0) return 0;
			synthDecodedFrame();
		}
		return framesWritten;
	}

private:
	shared_ptr<FileReader> myFile;

	mad_stream myStream;
	mad_frame myFrame;
	mad_synth mySynth;

	int myNumChannels;
	int myFrequency;

	uchar myFileBuffer[16384];
	short mySynthBuffer[8192];
	size_t synthNumSamples;
	size_t mySynthBufferPos;

	bool myIsFirstFrame;
	bool myHasXingHeader;

	XingHeader myXingHeader;
};

} // namespace LoadMp3

AudioFormats::LoadResult AudioFormats::loadMp3(const FilePath& path)
{
	// Open the file.
	auto file = make_shared<FileReader>();
	if (!file->open(path))
		return { .error = "Could not open file." };

	// Decode and synth the first frame to check if the file is valid.
	auto audio = make_shared<LoadMp3::Mp3SoundSource>(file);
	if (!audio->decodeFirstFrame())
		return { .error = "Could not decode MP3." };

	return { .audio = audio };
}

} // namespace AV
