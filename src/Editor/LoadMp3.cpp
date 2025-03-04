#include <Editor/Sound.h>

#include <mad.h>

#include <stdio.h>
#include <string.h>

#include <System/File.h>

namespace Vortex {
namespace {

// ================================================================================================
// ID3 tag parsing.

typedef ulong id3_length_t;

enum ID3Tagtype
{
	TAGTYPE_NONE = 0,
	TAGTYPE_ID3V1,
	TAGTYPE_ID3V2,
	TAGTYPE_ID3V2_FOOTER
};

static ID3Tagtype ID3GetTagtype(const uchar *data, id3_length_t length)
{
	if(length >= 3 &&
		data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
		return TAGTYPE_ID3V1;

	if(length >= 10 &&
		((data[0] == 'I' && data[1] == 'D' && data[2] == '3') ||
		(data[0] == '3' && data[1] == 'D' && data[2] == 'I')) &&
		data[3] < 0xff && data[4] < 0xff &&
		data[6] < 0x80 && data[7] < 0x80 && data[8] < 0x80 && data[9] < 0x80)
		return data[0] == 'I' ? TAGTYPE_ID3V2 : TAGTYPE_ID3V2_FOOTER;

	return TAGTYPE_NONE;
}

static ulong ID3ParseUint(const uchar **ptr, uint bytes)
{
	ulong value = 0;
	switch(bytes)
	{
		case 4: value = (value << 8) | *(*ptr)++;
		case 3: value = (value << 8) | *(*ptr)++;
		case 2: value = (value << 8) | *(*ptr)++;
		case 1: value = (value << 8) | *(*ptr)++;
	}
	return value;
}

static ulong ID3ParseSyncsafe(const uchar **ptr, uint bytes)
{
	ulong value = 0;
	switch(bytes)
	{
	case 5:
		value = (value << 4) | (*(*ptr)++ & 0x0f);
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
	case TAGTYPE_ID3V1:
		return 128;

	case TAGTYPE_ID3V2:
		ID3ParseHeader(&data, &version, &flags, &size);
		if(flags & 0x10 /*ID3_TAG_FLAG_FOOTERPRESENT*/) size += 10;
		return 10 + size;

	case TAGTYPE_ID3V2_FOOTER:
		ID3ParseHeader(&data, &version, &flags, &size);
		return -(int)size - 10;

	case TAGTYPE_NONE:
		break;
	}
	return 0;
}

// ================================================================================================
// XING header parsing.

struct XingHeader
{
	long flags;				// valid fields (see below).
	unsigned long frames;	// total number of frames.
	unsigned long bytes;	// total number of bytes.
	unsigned char toc[100];	// 100-point seek table.
	long scale;				// ??
	enum { XING, INFO } type;
};

enum XingFlags
{
	XING_FRAMES = 0x00000001L,
	XING_BYTES = 0x00000002L,
	XING_TOC = 0x00000004L,
	XING_SCALE = 0x00000008L
};

static int XingParse(XingHeader* xing, struct mad_bitptr ptr, unsigned int bitlen)
{
	const uint XING_MAGIC = (('X' << 24) | ('i' << 16) | ('n' << 8) | 'g');
	const uint INFO_MAGIC = (('I' << 24) | ('n' << 16) | ('f' << 8) | 'o');
	unsigned data;
	if(bitlen < 64)
		goto fail;
	data = mad_bit_read(&ptr, 32);

	if(data == XING_MAGIC)
		xing->type = XingHeader::XING;
	else if(data == INFO_MAGIC)
		xing->type = XingHeader::INFO;
	else
		goto fail;

	xing->flags = mad_bit_read(&ptr, 32);
	bitlen -= 64;

	if(xing->flags & XING_FRAMES)
	{
		if(bitlen < 32)
			goto fail;

		xing->frames = mad_bit_read(&ptr, 32);
		bitlen -= 32;
	}

	if(xing->flags & XING_BYTES)
	{
		if(bitlen < 32)
			goto fail;

		xing->bytes = mad_bit_read(&ptr, 32);
		bitlen -= 32;
	}

	if(xing->flags & XING_TOC)
	{
		if(bitlen < 800)
			goto fail;

		for(int i = 0; i < 100; ++i)
			xing->toc[i] = (unsigned char)mad_bit_read(&ptr, 8);

		bitlen -= 800;
	}

	if(xing->flags & XING_SCALE)
	{
		if(bitlen < 32)
			goto fail;

		xing->scale = mad_bit_read(&ptr, 32);
		bitlen -= 32;
	}

	return 0;

fail:
	xing->flags = 0;
	return -1;
}


// ================================================================================================
// MP3 loading.

struct MP3Loader : public SoundSource
{
	MP3Loader();
	~MP3Loader();

	int getFrequency() override { return frequency; }
	int getNumFrames() override { return 0; }
	int getNumChannels() override { return numChannels; }
	int getBytesPerSample() override { return 2; }
	int readFrames(int frames, short* buffer) override;

	int fillInputBuffer();
	bool decodeFirstFrame();
	int decodeNextFrame();
	void synthDecodedFrame();

	mad_stream madStream;
	mad_frame madFrame;
	mad_synth madSynth;

	int numChannels;
	int frequency;

	uchar fileBuf[16384];
	short synthBuf[8192];
	int synthNumSamples;
	int synthBufPos;

	bool isFirstFrame;
	bool hasXingHeader;

	XingHeader xing;

	FileReader* file;
};

MP3Loader::MP3Loader()
{
	mad_stream_init(&madStream);
	mad_frame_init(&madFrame);
	mad_synth_init(&madSynth);

	memset(fileBuf, 0, sizeof(fileBuf));
	memset(synthBuf, 0, sizeof(synthBuf));
	synthNumSamples = 0;
	synthBufPos = 0;

	isFirstFrame = true;
	hasXingHeader = false;

	numChannels = 0;

	memset(&xing, 0, sizeof(XingHeader));
}

MP3Loader::~MP3Loader()
{
	mad_synth_finish(&madSynth);
	mad_frame_finish(&madFrame);
 	mad_stream_finish(&madStream);

	delete file;
}

int MP3Loader::fillInputBuffer()
{
	// Mad needs more data from the input file.
	int inbytes = 0;
	if(madStream.next_frame != NULL)
	{
		// Pull out remaining data from the last buffer.
		inbytes = madStream.bufend - madStream.next_frame;
		memmove(fileBuf, madStream.next_frame, inbytes);
	}

	bool eofBeforeReading = file->eof();

	int rc = file->read(fileBuf + inbytes, 1, sizeof(fileBuf) - inbytes - MAD_BUFFER_GUARD);
	if(rc < 0) return -1;

	if(file->eof() && !eofBeforeReading)
	{
		/* We just reached EOF.  Append MAD_BUFFER_GUARD bytes of NULs to the
		* buffer, to ensure that the last frame is flushed. */
		memset(fileBuf + inbytes + rc, 0, MAD_BUFFER_GUARD);
		rc += MAD_BUFFER_GUARD;
	}
	if(rc == 0) return 0;

	inbytes += rc;
	mad_stream_buffer(&madStream, fileBuf, inbytes);
	return rc;
}

bool MP3Loader::decodeFirstFrame()
{
	int res = decodeNextFrame();
	if(res <= 0) return false;

	frequency = madFrame.header.samplerate;
	numChannels = MAD_NCHANNELS(&madFrame.header);

	synthDecodedFrame();

	return true;
}

int MP3Loader::decodeNextFrame()
{
	int bytes_read = 0;

	while(true)
	{
		int ret = mad_frame_decode(&madFrame, &madStream);

		if(ret == -1 && (madStream.error == MAD_ERROR_BUFLEN || madStream.error == MAD_ERROR_BUFPTR))
		{
			if(bytes_read > 25000) return -1; // We've read this much without actually getting a frame; error.
			int ret = fillInputBuffer();
			if(ret <= 0) return ret;
			bytes_read += ret;
			continue;
		}

		if(ret == -1 && madStream.error == MAD_ERROR_LOSTSYNC)
		{
			const int tagsize = ID3TagQuery(madStream.this_frame, madStream.bufend - madStream.this_frame);
			if(tagsize)
			{
				mad_stream_skip(&madStream, tagsize);
				bytes_read -= tagsize;
				continue;
			}
		}

		if(ret == -1 && madStream.error == MAD_ERROR_BADDATAPTR)
		{
			// Something's corrupt.  One cause of this is cutting an MP3 in the middle
			// without reencoding; the first two frames will reference data from previous
			// frames that have been removed.  The frame is valid--we can get a header from
			// it, we just can't synth useful data. BASS pretends the bad frames are silent.
			// Emulate that, for compatibility.
			ret = 0; // pretend success.
		}

		if(!ret)
		{
			if(isFirstFrame)
			{
				// We're at the beginning. Is this a Xing tag?
				xing.flags = 0;
				if(XingParse(&xing, madStream.anc_ptr, madStream.anc_bitlen) == 0)
				{
					if(xing.type != XingHeader::INFO)
					{
						hasXingHeader = true;
						// The first frame contained a header. Continue searching.
						if(xing.type == XingHeader::XING) continue;
					}
				}
				isFirstFrame = false;
			}			
			return 1;
		}

		if(madStream.error == MAD_ERROR_BADCRC)
		{
			mad_frame_mute(&madFrame);
			mad_synth_mute(&madSynth);
			continue;
		}

		if(!MAD_RECOVERABLE(madStream.error))
		{
			return -1;
		}
	}
}

static inline short ScaleSample(mad_fixed_t sample)
{
	sample += (1L << (MAD_F_FRACBITS - 16));
	if(sample >= MAD_F_ONE)	sample = MAD_F_ONE - 1;
	else if(sample < -MAD_F_ONE) sample = -MAD_F_ONE;
	return (short)(sample >> (MAD_F_FRACBITS + 1 - 16));
}

void MP3Loader::synthDecodedFrame()
{
	synthNumSamples = synthBufPos = 0;

	if(MAD_NCHANNELS(&madFrame.header) != numChannels) return;

	mad_synth_frame(&madSynth, &madFrame);
	for(int i = 0; i < madSynth.pcm.length; ++i)
	{
		for(int ch = 0; ch < numChannels; ++ch)
		{
			short sample = ScaleSample(madSynth.pcm.samples[ch][i]);
			synthBuf[synthNumSamples] = sample;
			++synthNumSamples;
		}
	}
}

int MP3Loader::readFrames(int frames, short* buffer)
{
	int framesWritten = 0;
	while(frames > 0)
	{
		// Copy the synthesized samples that are left in the synth buffer.
		if(synthNumSamples > 0)
		{
			int framesToCopy = synthNumSamples / numChannels;
			if(framesToCopy > frames) framesToCopy = frames;

			int samplesToCopy = framesToCopy * numChannels;
			int bytesToCopy = samplesToCopy * sizeof(short);

			memcpy(buffer, synthBuf + synthBufPos, bytesToCopy);

			buffer += samplesToCopy;
			frames -= framesToCopy;
			framesWritten += framesToCopy;
			synthBufPos += samplesToCopy;
			synthNumSamples -= samplesToCopy;
			continue;
		}

		// Decode and synthesize more samples.
		int ret = decodeNextFrame();
		if(ret <= 0) return 0;
		synthDecodedFrame();
	}
	return framesWritten;
}

}; // anonymous namespace.

SoundSource* LoadMP3(FileReader* file, String& title, String& artist)
{
	MP3Loader* loader = new MP3Loader;
	loader->file = file;

	// Decode and synth the first frame to check if the file is valid.
	if(!loader->decodeFirstFrame())
	{
		loader->file = nullptr;
		delete loader;
		return nullptr;
	}

	// The file is valid, return the MP3 loader.
	return loader;
}

}; // namespace Vortex
