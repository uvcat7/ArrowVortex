#include <Editor/Waveform.h>

#include <math.h>
#include <stdint.h>
#include <algorithm>
#include <vector>

#include <Core/Utils.h>
#include <Core/Draw.h>
#include <Core/QuadBatch.h>
#include <Core/StringUtils.h>
#include <Core/Xmr.h>

#include <System/System.h>
#include <System/Debug.h>

#include <Editor/Music.h>
#include <Editor/View.h>
#include <Editor/Editor.h>
#include <Managers/TempoMan.h>
#include <Editor/Menubar.h>
#include <Editor/TextOverlay.h>
#include <Editor/Butterworth.h>

namespace Vortex {

static const int TEX_W = 256;
static const int TEX_H = 128;
static const int UNUSED_BLOCK = -1;

struct WaveBlock
{
	int id;
	Texture tex[4];
};

// ================================================================================================
// WaveFilter.

struct WaveFilter {

Waveform::FilterType type;
double strength;

std::vector<short> samplesL;
std::vector<short> samplesR;

static void lowPassFilter(const short* src, short* dst,
	int numFrames, int samplerate, double strength)
{
	double cutoff = 0.01 + 0.1 * (1.0 - strength);
	LowPassFilter(dst, src, numFrames, cutoff);
}

static void highPassFilter(const short* src, short* dst,
	int numFrames, int samplerate, double strength)
{
	double cutoff = 0.10 + 0.80 * strength;
	HighPassFilter(dst, src, numFrames, cutoff);
}

WaveFilter(Waveform::FilterType type, double strength)
	: type(type), strength(strength)
{
	update();
}

void update()
{
	auto filter = highPassFilter;
	if(type == Waveform::FT_LOW_PASS)
	{
		filter = lowPassFilter;
	}

	samplesL.clear();
	samplesR.clear();

	auto& music = gMusic->getSamples();
	if(music.isCompleted())
	{
		int numFrames = music.getNumFrames();
		int samplerate = music.getFrequency();

		samplesL.resize(numFrames, 0);
		samplesR.resize(numFrames, 0);

		filter(music.samplesL(), samplesL.data(), numFrames, samplerate, strength);
		filter(music.samplesR(), samplesR.data(), numFrames, samplerate, strength);
	}
}

}; // WaveFilter.

// ================================================================================================
// WaveformImpl :: member data.

struct WaveformImpl : public Waveform {

std::vector<WaveBlock*> waveformBlocks_;

WaveFilter* waveformFilter_;
std::vector<uchar> waveformTextureBuffer_;

int waveformBlockWidth_, waveformSpacing_;

ColorScheme waveformColorScheme_;
WaveShape waveformShape_;
Luminance waveformLuminance_;
int waveformAntiAliasingMode_;
bool waveformOverlayFilter_;

// ================================================================================================
// ViewImpl :: constructor / destructor.

~WaveformImpl()
{
	for(auto block : waveformBlocks_) delete block;
	delete waveformFilter_;
}

WaveformImpl()
{
	setPreset(PRESET_VORTEX);

	waveformFilter_ = nullptr;
	waveformOverlayFilter_ = true;

	updateBlockW();
	waveformTextureBuffer_.resize(TEX_W * TEX_H);

	clearBlocks();
}

// ================================================================================================
// ViewImpl :: load / save settings.

static void SaveColor(XmrNode* n, const char* name, colorf col)
{
	String r = Str::val(col.r, 0, 2), g = Str::val(col.g, 0, 2);
	String b = Str::val(col.b, 0, 2), a = Str::val(col.a, 0, 2);
	const char* str[] = {r.str(), g.str(), b.str(), a.str()};
	n->addAttrib(name, str, 4);
}

static const char* ToString(WaveShape style)
{
	if(style == WS_SIGNED) return "signed";
	return "rectified";
}

static const char* ToString(Luminance lum)
{
	if(lum == LL_AMPLITUDE) return "amplitude";
	return "uniform";
}

static WaveShape ToWaveShape(StringRef str)
{
	if(str == "signed") return WS_SIGNED;
	return WS_RECTIFIED;
}

static Luminance ToLuminance(StringRef str)
{
	if(str == "amplitude") return LL_AMPLITUDE;
	return LL_UNIFORM;
}

void loadSettings(XmrNode& settings)
{
	XmrNode* waveform = settings.child("waveform");
	if(waveform)
	{
		waveform->get("bgColor", (float*)&waveformColorScheme_.bg, 4);
		waveform->get("waveColor", (float*)&waveformColorScheme_.wave, 4);
		waveform->get("filterColor", (float*)&waveformColorScheme_.filter, 4);

		const char* ll = waveform->get("luminance");
		if(ll) setLuminance(ToLuminance(ll));

		const char* ws = waveform->get("waveStyle");
		if(ws) setWaveShape(ToWaveShape(ws));	

		waveform->get("antiAliasing", &waveformAntiAliasingMode_);
		setAntiAliasing(clamp(waveformAntiAliasingMode_, 0, 3));
	}
}

void saveSettings(XmrNode& settings)
{
	XmrNode* waveform = settings.child("waveform");
	if(!waveform) waveform = settings.addChild("waveform");

	SaveColor(waveform, "bgColor", waveformColorScheme_.bg);
	SaveColor(waveform, "waveColor", waveformColorScheme_.wave);
	SaveColor(waveform, "filterColor", waveformColorScheme_.filter);

	waveform->addAttrib("luminance", ToString(waveformLuminance_));
	waveform->addAttrib("waveStyle", ToString(waveformShape_));
	waveform->addAttrib("antiAliasing", (long)waveformAntiAliasingMode_);
}

// ================================================================================================
// ViewImpl :: member functions.

void clearBlocks()
{
	for(auto block : waveformBlocks_)
	{
		block->id = UNUSED_BLOCK;
	}
}

void setOverlayFilter(bool enabled)
{
	waveformOverlayFilter_ = enabled;

	clearBlocks();
}

bool getOverlayFilter()
{
	return waveformOverlayFilter_;
}

void enableFilter(FilterType type, double strength)
{
	delete waveformFilter_;
	waveformFilter_ = new WaveFilter(type, strength);

	clearBlocks();
}

void disableFilter()
{
	delete waveformFilter_;
	waveformFilter_ = nullptr;

	clearBlocks();
}

int getWidth()
{
	return waveformBlockWidth_ * 2 + waveformSpacing_ * 2 + 8;
}

void onChanges(int changes)
{
	if(changes & VCM_MUSIC_IS_LOADED)
	{
		if(waveformFilter_) waveformFilter_->update();
	}
}

void tick()
{
}

void setPreset(Preset preset)
{
	switch(preset)
	{
	case PRESET_VORTEX:
		waveformColorScheme_.bg = {0.0f, 0.0f, 0.0f, 1};
		waveformColorScheme_.wave = {0.4f, 0.6, 0.4f, 1};
		waveformColorScheme_.filter = {0.8f, 0.8f, 0.5f, 1};
		waveformShape_ = WS_RECTIFIED;
		waveformLuminance_ = LL_UNIFORM;
		waveformAntiAliasingMode_ = 1;		
		break;
	case PRESET_DDREAM:
		waveformColorScheme_.bg = {0.45f, 0.6f, 0.11f, 0.8f};
		waveformColorScheme_.wave = {0.9f, 0.9f, 0.33f, 1};
		waveformColorScheme_.filter = {0.8f, 0.3f, 0.3f, 1};
		waveformShape_ = WS_SIGNED;
		waveformLuminance_ = LL_UNIFORM;
		waveformAntiAliasingMode_ = 0;
		break;
	};
	clearBlocks();
}

void setColors(ColorScheme colors)
{
	waveformColorScheme_ = colors;
}

ColorScheme getColors()
{
	return waveformColorScheme_;
}

void setLuminance(Luminance lum)
{
	waveformLuminance_ = lum;
	clearBlocks();
}

Luminance getLuminance()
{
	return waveformLuminance_;
}

void setWaveShape(WaveShape style)
{
	waveformShape_ = style;
	clearBlocks();
}

WaveShape getWaveShape()
{
	return waveformShape_;
}

void setAntiAliasing(int level)
{
	waveformAntiAliasingMode_ = level;
	clearBlocks();
}

int getAntiAliasing()
{
	return waveformAntiAliasingMode_;
}

// ================================================================================================
// Waveform :: luminance functions.

struct WaveEdge { int l, r; uchar lum; };

void edgeLumUniform(WaveEdge* edge, int h)
{
	for(int y = 0; y < h; ++y, ++edge)
	{
		edge->lum = 255;
	}
}

void edgeLumAmplitude(WaveEdge* edge, int w, int h)
{
	int scalar = (255 << 16) * 2 / w;
	for(int y = 0; y < h; ++y, ++edge)
	{
		int mag = max(abs(edge->l), abs(edge->r));
		edge->lum = (mag * scalar) >> 16;
	}
}

// ================================================================================================
// Waveform :: edge shape functions.

void edgeShapeRectified(uchar* dst, const WaveEdge* edge, int w, int h)
{
	int cx = w / 2;
	for(int y = 0; y < h; ++y, dst += w, ++edge)
	{
		int mag = max(abs(edge->l), abs(edge->r));
		int l = cx - mag;
		int r = cx + mag;
		for(int x = l; x < r; ++x) dst[x] = edge->lum;
	}
}

void edgeShapeSigned(uchar* dst, const WaveEdge* edge, int w, int h)
{
	int cx = w / 2;
	for(int y = 0; y < h; ++y, dst += w, ++edge)
	{
		int l = cx + min(edge->l, 0);
		int r = cx + max(edge->r, 0);
		for(int x = l; x < r; ++x) dst[x] = edge->lum;
	}
}

// ================================================================================================
// Waveform :: anti-aliasing functions.

void antiAlias2x(uchar* dst, int w, int h)
{
	int newW = w / 2, newH = h / 2;
	for(int y = 0; y < newH; ++y)
	{
		uchar* line = waveformTextureBuffer_.data() + (y * 2) * w;
		for(int x = 0; x < newW; ++x, ++dst)
		{
			uchar* a = line + (x * 2), *b = a + w;
			int sum = 0;
			sum += a[0] + a[1];
			sum += b[0] + b[1];
			*dst = sum / 4;
		}
	}
}

void antiAlias3x(uchar* dst, int w, int h)
{
	int newW = w / 3, newH = h / 3;
	for(int y = 0; y < newH; ++y)
	{
		uchar* line = waveformTextureBuffer_.data() + (y * 3) * w;
		for(int x = 0; x < newW; ++x, ++dst)
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

void antiAlias4x(uchar* dst, int w, int h)
{
	int newW = w / 4, newH = h / 4;
	for(int y = 0; y < newH; ++y)
	{
		uchar* line = waveformTextureBuffer_.data() + (y * 4) * w;
		for(int x = 0; x < newW; ++x, ++dst)
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

// ================================================================================================
// Waveform :: block rendering functions.

void sampleEdges(WaveEdge* edges, int w, int h, int channel, int blockId, bool filtered)
{
	auto& music = gMusic->getSamples();

	double samplesPerSec = (double)music.getFrequency();
	double samplesPerPixel = samplesPerSec / fabs(gView->getPixPerSec());
	double samplesPerBlock = (double)TEX_H * samplesPerPixel;

	int64_t srcFrames = filtered ? waveformFilter_->samplesL.size() : music.getNumFrames();
	int64_t samplePos = max((int64_t)0, (int64_t)(samplesPerBlock * (double)blockId));
	double sampleCount = min((double) srcFrames - samplePos, samplesPerBlock);

	if (samplePos >= srcFrames || sampleCount <= 0)
	{
		// A crash could occur if we try to access out-of-bounds memory.
		// Fill the edges with zeroes to avoid this issue.
		for (int y = 0; y < h; ++y)
		{
			edges[y] = {0, 0, 0};
		}
		return;
	}

	double sampleSkip = max(0.001, (samplesPerPixel / 200.0));
	int wh = w / 2 - 1;

	const short* in = nullptr;
	if(filtered)
	{
		in = ((channel == 0) ? waveformFilter_->samplesL.data() : waveformFilter_->samplesR.data());
	}
	else
	{
		in = ((channel == 0) ? music.samplesL() : music.samplesR());
	}
	in += samplePos;

	double advance = samplesPerPixel * TEX_H / h;
	double ofs = 0;
	for(int y = 0; y < h; ++y)
	{
		// Determine the last sample of the line.
		double end = min(sampleCount, (double)(y + 1) * advance);

		// Find the minimum/maximum amplitude within the line.
		int minAmp = SHRT_MAX;
		int maxAmp = SHRT_MIN;
		while(ofs < end)
		{
			maxAmp = max(maxAmp, (int)*(in + (int) round(ofs)));
			minAmp = min(minAmp, (int)*(in + (int) round(ofs)));
			ofs += sampleSkip;
		}

		// Clamp the minimum/maximum amplitude.
		int l = (minAmp * wh) >> 15;
		int r = (maxAmp * wh) >> 15;
		if(r >= l)
		{
			edges[y] = {clamp(l, -wh, wh), clamp(r, -wh, wh), 0};
		}
		else
		{
			edges[y] = {0, 0, 0};
		}
	}
}

void renderWaveform(Texture* textures, WaveEdge* edgeBuf, int w, int h, int blockId, bool filtered)
{
	uchar* texBuf = waveformTextureBuffer_.begin();
	for(int channel = 0; channel < 2; ++channel)
	{
		memset(texBuf, 0, w * h);

		// Process edges
		sampleEdges(edgeBuf, w, h, channel, blockId, filtered);

		// Apply luminance
		if (waveformLuminance_ == LL_UNIFORM) {
			edgeLumUniform(edgeBuf, h);
		}
		else if (waveformLuminance_ == LL_AMPLITUDE) {
			edgeLumAmplitude(edgeBuf, w, h);
		}

		// Apply wave shape
		if (waveformShape_ == WS_RECTIFIED) {
			edgeShapeRectified(texBuf, edgeBuf, w, h);
		}
		else if (waveformShape_ == WS_SIGNED) {
			edgeShapeSigned(texBuf, edgeBuf, w, h);
		}

		// Apply anti-aliasing
		switch (waveformAntiAliasingMode_) {
		case 1: antiAlias2x(texBuf, w, h); break;
		case 2: antiAlias3x(texBuf, w, h); break;
		case 3: antiAlias4x(texBuf, w, h); break;
		}

		// Create or update texture
		if (!textures[channel].handle()) {
			textures[channel] = Texture(TEX_W, TEX_H, Texture::ALPHA);
		}
		textures[channel].modify(0, 0, waveformBlockWidth_, TEX_H, texBuf);
	}
}

void renderBlock(WaveBlock* block)
{
	int w = waveformBlockWidth_ * (waveformAntiAliasingMode_ + 1);
	int h = TEX_H * (waveformAntiAliasingMode_ + 1);
	waveformTextureBuffer_.resize(w * h);

	std::vector<WaveEdge> edges;
	edges.resize(h);

	if(waveformFilter_)
	{
		if(waveformOverlayFilter_)
		{
			renderWaveform(block->tex + 0, edges.data(), w, h, block->id, false);
			renderWaveform(block->tex + 2, edges.data(), w, h, block->id, true);
		}
		else
		{
			renderWaveform(block->tex, edges.data(), w, h, block->id, true);
		}
	}
	else
	{
		renderWaveform(block->tex, edges.data(), w, h, block->id, false);
	}
}

WaveBlock* getBlock(int id)
{
	// Check if we already have the requested block.
	for(auto block : waveformBlocks_)
	{
		if(block->id == id)
		{
			return block;
		}
	}

	// If not, check if we have any free blocks available.
	for(auto block : waveformBlocks_)
	{
		if(block->id == UNUSED_BLOCK)
		{
			block->id = id;
			renderBlock(block);
			return block;
		}
	}

	// If not, create a new block.
	WaveBlock* block = new WaveBlock;
	waveformBlocks_.push_back(block);

	block->id = id;
	renderBlock(block);
	return block;
}

void updateBlockW()
{
	int width = waveformBlockWidth_;

	waveformBlockWidth_ = min(TEX_W, gView->applyZoom(256));
	waveformSpacing_ = gView->applyZoom(24);

	if(waveformBlockWidth_ != width) clearBlocks();
}

void drawBackground()
{
	updateBlockW();

	int w = waveformBlockWidth_ * 2 + waveformSpacing_ * 2 + 8;
	int h = gView->getHeight();
	int cx = gView->getReceptorCoords().xc;

	Draw::fill({cx - w / 2, 0, w, h}, ToColor32(waveformColorScheme_.bg));
}

void drawPeaks()
{
	updateBlockW();

	bool reversed = gView->hasReverseScroll();
	int visibilityStartY, visibilityEndY;
	if(reversed)
	{
		visibilityEndY = gView->timeToY(0);
		visibilityStartY = visibilityEndY - gView->getHeight();
	}
	else
	{
		visibilityStartY = -gView->timeToY(0);
		visibilityEndY = visibilityStartY + gView->getHeight();
	}

	// Free up blocks that are no longer visible.
	for(auto block : waveformBlocks_)
	{
		if(block->id != UNUSED_BLOCK)
		{
			int y = block->id * TEX_H;
			if(y >= visibilityEndY || y + TEX_H <= visibilityStartY)
			{
				block->id = UNUSED_BLOCK;
			}
		}
	}

	// Show blocks for the regions of the song that are visible.
	int border = waveformSpacing_, pw = waveformBlockWidth_ / 2;
	int cx = gView->getReceptorCoords().xc;
	int xl = cx - pw - border;
	int xr = cx + pw + border;

	areaf uvs = {0, 0, waveformBlockWidth_ / (float)TEX_W, 1};
	if(reversed) swapValues(uvs.t, uvs.b);

	int id = max(0, visibilityStartY / TEX_H);
	for(; id * TEX_H < visibilityEndY; ++id)
	{
		auto block = getBlock(id);

		int y = id * TEX_H - visibilityStartY;
		if(reversed) y = gView->getHeight() - y - TEX_H;

		TextureHandle texL = block->tex[0].handle();
		TextureHandle texR = block->tex[1].handle();

		color32 waveCol = ToColor32(waveformColorScheme_.wave);
		Draw::fill({xl - pw, y, pw * 2, TEX_H}, waveCol, texL, uvs, Texture::ALPHA);
		Draw::fill({xr - pw, y, pw * 2, TEX_H}, waveCol, texR, uvs, Texture::ALPHA);

		if(waveformFilter_ && waveformOverlayFilter_)
		{
			TextureHandle texL = block->tex[2].handle();
			TextureHandle texR = block->tex[3].handle();

			color32 filterCol = ToColor32(waveformColorScheme_.filter);
			Draw::fill({xl - pw, y, pw * 2, TEX_H}, filterCol, texL, uvs, Texture::ALPHA);
			Draw::fill({xr - pw, y, pw * 2, TEX_H}, filterCol, texR, uvs, Texture::ALPHA);
		}
	}
}

}; // WaveformImpl.

// ================================================================================================
// Waveform :: API.

Waveform* gWaveform = nullptr;

void Waveform::create(XmrNode& settings)
{
	gWaveform = new WaveformImpl;
	((WaveformImpl*)gWaveform)->loadSettings(settings);
}

void Waveform::destroy()
{
	delete (WaveformImpl*)gWaveform;
	gWaveform = nullptr;
}

}; // namespace Vortex
