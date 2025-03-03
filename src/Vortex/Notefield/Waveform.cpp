#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Xmr.h>

#include <Core/System/System.h>
#include <Core/System/Debug.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Texture.h>

#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/Managers/MusicMan.h>
#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/View/View.h>
#include <Vortex/View/TextOverlay.h>
#include <Vortex/NoteField/Notefield.h>
#include <Vortex/Notefield/Waveform.h>

#include <Audio/Butterworth.h>

namespace AV {

using namespace std;
using namespace Util;

static constexpr int TexW = 256;
static constexpr int TexH = 128;
static constexpr int UnusedBlock = -1;

struct WaveBlock
{
	int id = 0;
	Texture tex[4];
};

// =====================================================================================================================
// WaveFilter.

struct WaveFilter {

Waveform::FilterType type;
double strength;

vector<short> samplesL;
vector<short> samplesR;

static void ApplyLowPassFilter(
	const short* src, short* dst, size_t numFrames, uint samplerate, double strength)
{
	double cutoff = 0.01 + 0.1 * (1.0 - strength);
	Butterworth::LowPassFilter(dst, src, numFrames, cutoff);
}

static void ApplyHighPassFilter(
	const short* src, short* dst, size_t numFrames, uint samplerate, double strength)
{
	double cutoff = 0.10 + 0.80 * strength;
	Butterworth::HighPassFilter(dst, src, numFrames, cutoff);
}

WaveFilter(Waveform::FilterType type, double strength)
	: type(type), strength(strength)
{
	update();
}

void update()
{
	auto filter = ApplyHighPassFilter;
	if (type == Waveform::FilterType::LowPass)
	{
		filter = ApplyLowPassFilter;
	}

	samplesL.clear();
	samplesR.clear();

	auto& music = MusicMan::getSamples();
	if (music.isCompleted())
	{
		size_t numFrames = music.getNumFrames();
		int samplerate = music.getFrequency();

		samplesL.resize(numFrames);
		samplesR.resize(numFrames);

		filter(music.samplesL(), samplesL.data(), numFrames, samplerate, strength);
		filter(music.samplesR(), samplesR.data(), numFrames, samplerate, strength);
	}
}

}; // WaveFilter.

// =====================================================================================================================
// Enum conversion.

template<>
const char* Enum::toString<Waveform::Luminance>(Waveform::Luminance value)
{
	switch (value)
	{
	case Waveform::Luminance::Uniform:
		return "uniform";
	case Waveform::Luminance::Amplitude:
		return "amplitude";
	}
	return "unknown";
}

template<>
optional<Waveform::Luminance> Enum::fromString<Waveform::Luminance>(stringref str)
{
	if (str == "uniform")
		return Waveform::Luminance::Uniform;
	if (str == "amplitude")
		return Waveform::Luminance::Amplitude;
	return std::nullopt;
}

template<>
const char* Enum::toString<Waveform::WaveShape>(Waveform::WaveShape value)
{
	switch (value)
	{
	case Waveform::WaveShape::Rectified:
		return "rectified";
	case Waveform::WaveShape::Signed:
		return "signed";
	}
	return "unknown";
}

template<>
optional<Waveform::WaveShape> Enum::fromString<Waveform::WaveShape>(stringref str)
{
	if (str == "rectified")
		return Waveform::WaveShape::Rectified;
	if (str == "signed")
		return Waveform::WaveShape::Signed;
	return std::nullopt;
}

// =====================================================================================================================
// Static data.


namespace Waveform
{
	struct State
	{
		vector<WaveBlock*> blocks;
	
		WaveFilter* filter = nullptr;
		vector<uchar> texBuf;
	
		int blockW = 0;
		int spacing = 0;
		bool overlayFilter = false;
	
		EventSubscriptions subscriptions;
	
	};
	static State* state = nullptr;
}
using Waveform::state;

static void UpdateBlockW()
{
	int width = state->blockW;

	state->blockW = (int)View::getZoomScale() * 256;
	state->spacing = (int)View::getZoomScale() * 24;

	if (state->blockW != width)
	{
		Waveform::clearBlocks();
	}
}



// =====================================================================================================================
// Initialization.

static void HandleMusicSamplesChanged()
{
	Waveform::clearBlocks();

	if (state->filter)
		state->filter->update();
}

void Waveform::initialize(const XmrDoc& settings)
{
	state = new State();

	state->filter = nullptr;
	state->overlayFilter = true;

	UpdateBlockW();
	state->texBuf.resize(TexW * TexH);

	clearBlocks();

	state->subscriptions.add<MusicMan::MusicSamplesChanged>(HandleMusicSamplesChanged);

}

void Waveform::deinitialize()
{
	for (auto block : state->blocks) delete block;
	delete state->filter;
	Util::reset(state);
}

// =====================================================================================================================
// Member functions.

void Waveform::clearBlocks()
{
	for (auto block : state->blocks)
	{
		block->id = UnusedBlock;
	}
}

void Waveform::overlayFilter(bool enabled)
{
	state->overlayFilter = enabled;

	clearBlocks();
}

void Waveform::enableFilter(FilterType type, double strength)
{
	delete state->filter;
	state->filter = new WaveFilter(type, strength);

	clearBlocks();
}

void Waveform::disableFilter()
{
	Util::reset(state->filter);
	clearBlocks();
}

int Waveform::getWidth()
{
	return state->blockW * 2 + state->spacing * 2 + 8;
}

void Waveform::setPreset(Preset preset)
{
	auto& settings = AV::Settings::waveform();
	switch(preset)
	{
	case Preset::Vortex:
		settings.backgroundColor.set({0.0f, 0.0f, 0.0f, 1});
		settings.waveColor.set({0.4f, 0.6, 0.4f, 1});
		settings.filterColor.set({0.8f, 0.8f, 0.5f, 1});
		settings.waveShape.set(WaveShape::Rectified);
		settings.luminance.set(Luminance::Uniform);
		settings.antiAliasing.set(1);
		break;

	case Preset::DDReam:
		settings.backgroundColor.set({0.45f, 0.6f, 0.11f, 0.8f});
		settings.waveColor.set({0.9f, 0.9f, 0.33f, 1});
		settings.filterColor.set({0.8f, 0.3f, 0.3f, 1});
		settings.waveShape.set(WaveShape::Signed);
		settings.luminance.set(Luminance::Uniform);
		settings.antiAliasing.set(0);
		break;
	};
	clearBlocks();
}

// =====================================================================================================================
// Luminance functions.

struct WaveEdge { int l, r; uchar lum; };

static void edgeLumUniform(WaveEdge* edge, int h)
{
	for (int y = 0; y < h; ++y, ++edge)
	{
		edge->lum = 255;
	}
}

static void edgeLumAmplitude(WaveEdge* edge, int w, int h)
{
	int scalar = (255 << 16) * 2 / w;
	for (int y = 0; y < h; ++y, ++edge)
	{
		int mag = max(abs(edge->l), abs(edge->r));
		edge->lum = (mag * scalar) >> 16;
	}
}

// =====================================================================================================================
// Edge shape functions.

static void edgeShapeRectified(uchar* dst, const WaveEdge* edge, int w, int h)
{
	int cx = w / 2;
	for (int y = 0; y < h; ++y, dst += w, ++edge)
	{
		int mag = max(abs(edge->l), abs(edge->r));
		int l = cx - mag;
		int r = cx + mag;
		for (int x = l; x < r; ++x) dst[x] = edge->lum;
	}
}

static void edgeShapeSigned(uchar* dst, const WaveEdge* edge, int w, int h)
{
	int cx = w / 2;
	for (int y = 0; y < h; ++y, dst += w, ++edge)
	{
		int l = cx + min(edge->l, 0);
		int r = cx + max(edge->r, 0);
		for (int x = l; x < r; ++x) dst[x] = edge->lum;
	}
}

// =====================================================================================================================
// Anti-aliasing functions.

static void antiAlias2x(uchar* dst, int w, int h)
{
	int newW = w / 2, newH = h / 2;
	for (int y = 0; y < newH; ++y)
	{
		uchar* line = state->texBuf.data() + (y * 2) * w;
		for (int x = 0; x < newW; ++x, ++dst)
		{
			uchar* a = line + (x * 2), *b = a + w;
			int sum = 0;
			sum += a[0] + a[1];
			sum += b[0] + b[1];
			*dst = sum / 4;
		}
	}
}

static void antiAlias3x(uchar* dst, int w, int h)
{
	int newW = w / 3, newH = h / 3;
	for (int y = 0; y < newH; ++y)
	{
		uchar* line = state->texBuf.data() + (y * 3) * w;
		for (int x = 0; x < newW; ++x, ++dst)
		{
			uchar* a = line + (x * 3);
			uchar* b = a + w, *c = b + w;
			int sum = 0;
			sum += a[0] + a[1] + a[2];
			sum += b[0] + b[1] + b[2];
			sum += c[0] + c[1] + c[2];
			*dst = sum / 9;
		}
	}
}

static void antiAlias4x(uchar* dst, int w, int h)
{
	int newW = w / 4, newH = h / 4;
	for (int y = 0; y < newH; ++y)
	{
		uchar* line = state->texBuf.data() + (y * 4) * w;
		for (int x = 0; x < newW; ++x, ++dst)
		{
			uchar* a = line + (x * 4), *b = a + w;
			uchar* c = b + w, *d = c + w;
			int sum = 0;
			sum += a[0] + a[1] + a[2] + a[3];
			sum += b[0] + b[1] + b[2] + b[3];
			sum += c[0] + c[1] + c[2] + c[3];
			sum += d[0] + d[1] + d[2] + d[3];
			*dst = sum / 16;
		}
	}
}

// =====================================================================================================================
// Block rendering functions.

static void SampleEdges(WaveEdge* edges, int w, int h, int channel, int blockId, bool filtered)
{
	auto& music = MusicMan::getSamples();

	double samplesPerSec = (double)music.getFrequency();
	double samplesPerPixel = samplesPerSec / fabs(View::getPixPerSec());
	double samplesPerBlock = (double)TexH * samplesPerPixel;

	int64_t srcFrames = filtered ? state->filter->samplesL.size() : music.getNumFrames();
	int64_t samplePos = max((int64_t)0, (int64_t)(samplesPerBlock * (double)blockId));
	int sampleCount = (int)min(srcFrames - samplePos, (int64_t)samplesPerBlock);
	if ((int64_t)music.getNumFrames() < samplePos) sampleCount = 0;

	int sampleSkip = max(1, int(samplesPerPixel / 200.0));
	int wh = w / 2 - 1;

	const short* in = nullptr;
	if (filtered)
	{
		in = ((channel == 0) ? state->filter->samplesL.data() : state->filter->samplesR.data());
	}
	else
	{
		in = ((channel == 0) ? music.samplesL() : music.samplesR());
	}
	in += samplePos;

	double advance = samplesPerPixel * TexH / h;
	for (int y = 0, ofs = 0; y < h; ++y)
	{
		// Determine the last sample of the line.
		int end = min(sampleCount, int((double)(y + 1) * advance));

		// Find the minimum/maximum amplitude within the line.
		int minAmp = SHRT_MAX;
		int maxAmp = SHRT_MIN;
		while (ofs < end)
		{
			maxAmp = max(maxAmp, (int)*in);
			minAmp = min(minAmp, (int)*in);
			ofs += sampleSkip;
			in += sampleSkip;
		}

		// Clamp the minimum/maximum amplitude.
		int l = (minAmp * wh) >> 15;
		int r = (maxAmp * wh) >> 15;
		if (r > l)
		{
			edges[y] = {clamp(l, -wh, wh), clamp(r, -wh, wh), 0};
		}
		else
		{
			edges[y] = {0, 0, 0};
		}
	}
}

static void RenderWaveform(Texture* textures, WaveEdge* edgeBuf, int w, int h, int blockId, bool filtered)
{
	auto& settings = AV::Settings::waveform();
	uchar* texBuf = state->texBuf.data();
	for (int channel = 0; channel < 2; ++channel)
	{
		memset(texBuf, 0, w * h);

		SampleEdges(edgeBuf, w, h, channel, blockId, filtered);
		switch (settings.luminance)
		{
			case Waveform::Luminance::Uniform:   edgeLumUniform(edgeBuf, h); break;
			case Waveform::Luminance::Amplitude: edgeLumAmplitude(edgeBuf, w, h); break;
		}
		switch (settings.waveShape)
		{
			case Waveform::WaveShape::Rectified: edgeShapeRectified(texBuf, edgeBuf, w, h); break;
			case Waveform::WaveShape::Signed:    edgeShapeSigned(texBuf, edgeBuf, w, h); break;
		};
		switch (settings.antiAliasing)
		{
			case 1: antiAlias2x(texBuf, w, h); break;
			case 2: antiAlias3x(texBuf, w, h); break;
			case 3: antiAlias4x(texBuf, w, h); break;
		};
		if (!textures[channel].valid())
		{
			textures[channel] = Texture(TexW, TexH, TextureFormat::Alpha);
		}
		textures[channel].modify(texBuf, 0, 0, state->blockW, TexH, state->blockW, PixelFormat::Alpha);
	}
}

void renderBlock(WaveBlock* block)
{
	auto& settings = AV::Settings::waveform();

	int w = state->blockW * (settings.antiAliasing + 1);
	int h = TexH * (settings.antiAliasing + 1);
	state->texBuf.resize(w * h);

	vector<WaveEdge> edges;
	edges.resize(h);

	if (state->filter)
	{
		if (state->overlayFilter)
		{
			RenderWaveform(block->tex + 0, edges.data(), w, h, block->id, false);
			RenderWaveform(block->tex + 2, edges.data(), w, h, block->id, true);
		}
		else
		{
			RenderWaveform(block->tex, edges.data(), w, h, block->id, true);
		}
	}
	else
	{
		RenderWaveform(block->tex, edges.data(), w, h, block->id, false);
	}
}

WaveBlock* getBlock(int id)
{
	// Check if we already have the requested block.
	for (auto block : state->blocks)
	{
		if (block->id == id)
		{
			return block;
		}
	}

	// If not, check if we have any free blocks available.
	for (auto block : state->blocks)
	{
		if (block->id == UnusedBlock)
		{
			block->id = id;
			renderBlock(block);
			return block;
		}
	}

	// If not, create a new block.
	WaveBlock* block = new WaveBlock;
	state->blocks.push_back(block);

	block->id = id;
	renderBlock(block);
	return block;
}

void Waveform::drawBackground()
{
	auto& settings = AV::Settings::waveform();

	UpdateBlockW();

	int w = state->blockW * 2 + state->spacing * 2 + 8;
	int h = View::getHeight();
	int cx = (int)View::getReceptorCoords().xc;

	auto color = settings.backgroundColor.value().toU8();
	Draw::fill(Rect(cx - w/2, 0, cx + w/2, h), color);
}

void Waveform::drawPeaks()
{
	UpdateBlockW();

	bool reversed = AV::Settings::view().reverseScroll;
	int visibilityStartY, visibilityEndY;
	if (reversed)
	{
		visibilityEndY = (int)View::timeToY(0);
		visibilityStartY = visibilityEndY - View::getHeight();
	}
	else
	{
		visibilityStartY = -(int)View::timeToY(0);
		visibilityEndY = visibilityStartY + View::getHeight();
	}

	// Free up blocks that are no longer visible.
	for (auto block : state->blocks)
	{
		if (block->id != UnusedBlock)
		{
			int y = block->id * TexH;
			if (y >= visibilityEndY || y + TexH <= visibilityStartY)
			{
				block->id = UnusedBlock;
			}
		}
	}

	// Show blocks for the regions of the song that are visible.
	int border = state->spacing, pw = state->blockW / 2;
	int cx = (int)View::getReceptorCoords().xc;
	int xl = cx - pw - border;
	int xr = cx + pw + border;

	Rectf uvs(0, 0, state->blockW / (float)TexW, 1);
	if (reversed) swap(uvs.t, uvs.b);

	auto& settings = AV::Settings::waveform();
	auto waveCol = settings.waveColor.value().toU8();
	auto filterCol = settings.filterColor.value().toU8();

	int id = max(0, visibilityStartY / TexH);
	for (; id * TexH < visibilityEndY; ++id)
	{
		auto block = getBlock(id);

		int y = id * TexH - visibilityStartY;
		if (reversed) y = View::getHeight() - y - TexH;

		const auto& texL = block->tex[0];
		const auto& texR = block->tex[1];

		Draw::fill(Rect(xl - pw, y, xl + pw, y + TexH), waveCol, texL, uvs);
		Draw::fill(Rect(xr - pw, y, xr + pw, y + TexH), waveCol, texR, uvs);

		if (state->filter && state->overlayFilter)
		{
			const auto& texL = block->tex[2];
			const auto& texR = block->tex[3];

			Draw::fill(Rect(xl - pw, y, xl + pw, y + TexH), filterCol, texL, uvs);
			Draw::fill(Rect(xr - pw, y, xr + pw, y + TexH), filterCol, texR, uvs);
		}
	}
}

} // namespace AV
