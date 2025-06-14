#include <Editor/Minimap.h>

#include <math.h>

#include <Core/Utils.h>

#include <System/Debug.h>
#include <System/System.h>

#include <Core/Draw.h>
#include <Core/Renderer.h>

#include <Editor/Common.h>
#include <Editor/Editor.h>
#include <Editor/View.h>
#include <Editor/TextOverlay.h>
#include <Editor/Menubar.h>
#include <Editor/Selection.h>

#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>
#include <Managers/ChartMan.h>
#include <Managers/StyleMan.h>
#include <Managers/NoteMan.h>

namespace Vortex {

namespace {

static const int MAP_HEIGHT = 4096;
static const int MAP_WIDTH = 16;
static const int TEXTURE_SIZE = 256; //sqrt(MAP_HEIGHT_MAX * MAP_WIDTH)
static const int NUM_PIECES = MAP_HEIGHT / TEXTURE_SIZE;

static const color32 arrowcol[9] =
{
	RGBAtoColor32(255,   0,   0, 255), // Red.
	RGBAtoColor32( 12,  74, 206, 255), // Blue.
	RGBAtoColor32(145,  12, 206, 255), // Purple.
	RGBAtoColor32(255, 255,   0, 255), // Yellow.
	RGBAtoColor32(206,  12, 113, 255), // Pink.
	RGBAtoColor32(247, 148,  29, 255), // Orange.
	RGBAtoColor32(105, 231, 245, 255), // Teal.
	RGBAtoColor32(  0, 198,   0, 255), // Green.
	RGBAtoColor32(132, 132, 132, 255), // Gray.
};
static const color32 freezecol =
{
	RGBAtoColor32(64, 128, 0, 255) // Green
};
static const color32 rollcol =
{
	RGBAtoColor32(96, 96, 128, 255), // Blue gray
};
static const color32 minecol =
{
	RGBAtoColor32(192, 192, 192, 255), // Light gray
};

struct SetPixelData
{
	uint* pixels;
	int noteW;
	double pixPerOfs, startOfs;
};

static void SetPixels(const SetPixelData& spd, int x, double tor, color32 color)
{
	int y = clamp((int)((tor - spd.startOfs) * spd.pixPerOfs), 0, MAP_HEIGHT - 1);
	uint* dst = spd.pixels + y * MAP_WIDTH + x;
	for(int i = 0; i < spd.noteW; ++i, ++dst) *dst = color;
}

static void SetPixels(const SetPixelData& spd, int x, double tor, double end, color32 color)
{
	int yt = clamp((int)((tor - spd.startOfs) * spd.pixPerOfs), 0, MAP_HEIGHT - 1);
	int yb = clamp((int)((end - spd.startOfs) * spd.pixPerOfs), 0, MAP_HEIGHT - 1);
	for(int y = yt; y <= yb; ++y)
	{
		uint* dst = spd.pixels + y * MAP_WIDTH + x;
		for(int i = 0; i < spd.noteW; ++i, ++dst) *dst = color;
	}
}

}; // anonymous namespace

// ================================================================================================
// MinimapImpl :: member data.

struct MinimapImpl : public Minimap {

recti rect_;
Texture myImage;
Mode myMode;
int myNotesH;
double myChartBeginOfs;
double myChartEndOfs;
bool myIsDragging;
float myUvs[NUM_PIECES * 8];

// ================================================================================================
// MinimapImpl :: constructor and destructor.

~MinimapImpl()
{
}

MinimapImpl()
{
	myMode = NOTES;
	myImage = Texture(TEXTURE_SIZE, TEXTURE_SIZE);
	myNotesH = 0;
	myIsDragging = false;

	VortexAssert(TEXTURE_SIZE * TEXTURE_SIZE == MAP_HEIGHT * MAP_WIDTH);
	// Split the texture area into vertical strips from left to right.
	float u = 0.f, du = (float)MAP_WIDTH / (float)TEXTURE_SIZE;
	for(int i = 0; i < NUM_PIECES; ++i, u += du)
	{
		float v[8] = {u, 0, u + du, 0, u, 1, u + du, 1};
		memcpy(myUvs + i * 8, v, sizeof(float) * 8);
	}
}

// ================================================================================================
// MinimapImpl :: rendering functions.

void renderNotes(SetPixelData& spd, const int* colx)
{
	if(gView->isTimeBased())
	{
		for(auto& note : *gNotes)
		{
			int rowtype = ToRowType(note.row);
			if(note.endrow > note.row)
			{
				color32 color = (note.isRoll) ? rollcol : freezecol;
				SetPixels(spd, colx[note.col], note.time, note.endtime, color);
			}
			color32 color;
			if(note.isSelected)
			{
				color = RGBAtoColor32(255, 255, 255, 255);
			}
			else
			{
				color = (note.isMine) ? minecol : arrowcol[rowtype];
			}
			SetPixels(spd, colx[note.col], note.time, color);
		}
	}
	else
	{
		for(auto& note : *gNotes)
		{
			int rowtype = ToRowType(note.row);
			if(note.endrow > note.row)
			{
				color32 color = (note.isRoll) ? rollcol : freezecol;
				SetPixels(spd, colx[note.col], note.row, note.endrow, color);
			}
			color32 color;
			if(note.isSelected)
			{
				color = RGBAtoColor32(255, 255, 255, 255);
			}
			else
			{
				color = (note.isMine) ? minecol : arrowcol[rowtype];
			}
			SetPixels(spd, colx[note.col], note.row, color);
		}
	}
}

static int BlendI32(int a, int b, int f)
{
	return a + (((b - a) * f) >> 8);
}

static void SetDensityRow(uint* pixels, int y, double density)
{
	static const uchar colors[] =
	{
		 64, 192, 192,
		128, 255,  64,
		255,  64,  64,
		128, 128, 255,
	};
	density = round(density * 10.0) * (512.0 * 0.1 / 16.0);
	int coloring = clamp((int)density, 0, 511);
	int ci = coloring >> 8; ci *= 3;
	int bf = coloring & 255;
	int r = BlendI32(colors[ci + 0], colors[ci + 3], bf);
	int g = BlendI32(colors[ci + 1], colors[ci + 4], bf);
	int b = BlendI32(colors[ci + 2], colors[ci + 5], bf);
	uint col = Color32(r, g, b);

	int w = clamp((int)(density * 0.05), 4, MAP_WIDTH) / 2;
	uint* dst = pixels + y * MAP_WIDTH + MAP_WIDTH / 2 - w;
	for(int i = -w; i < w; ++i, ++dst) *dst = col;
}

void renderDensity(SetPixelData& spd, const int* colx)
{
	auto first = gNotes->begin(), end = gNotes->end(), it = first, last = end - 1;

	if(gView->isTimeBased())
	{
		double sec = myChartBeginOfs;
		double secPerPix = (double)(myChartEndOfs - myChartBeginOfs) / (double)myNotesH;
		for(int y = 0; y < myNotesH; ++y)
		{
			double density = 0.0;
			for(; it != end && it->time < sec; ++it);
			for(; it != end && it->time < sec + secPerPix; ++it)
			{
				if(it->isMine || it->isWarped) continue;
				double pre = (it > first) ? (it - 1)->time : (it->time - 1.0);
				double post = (it < last) ? (it + 1)->time : (it->time + 1.0);
				if(post > pre) density = max(density, 2.0 / (post - pre));
			}
			if(density > 0.0) SetDensityRow(spd.pixels, y, density);
			sec += secPerPix;
		}
	}
	else
	{
		double row = myChartBeginOfs;
		double rowPerPix = (double)(myChartEndOfs - myChartBeginOfs) / (double)myNotesH;
		for(int y = 0; y < myNotesH; ++y)
		{
			double density = 0.0;
			for(; it != end && it->row < row; ++it);
			for(; it != end && it->row < row + rowPerPix; ++it)
			{
				if(it->isMine || it->isWarped) continue;
				double pre = (it > first) ? (it - 1)->time : (it->time - 1.0);
				double post = (it < last) ? (it + 1)->time : (it->time + 1.0);
				if(post > pre) density = max(density, 2.0 / (post - pre));
			}
			if(density > 0.0) SetDensityRow(spd.pixels, y, density);
			row += rowPerPix;
		}
	}
}

// ================================================================================================
// MinimapImpl :: member functions.

recti myGetMapRect()
{
	vec2i size = gSystem->getWindowSize();
	rect_ = { size.x - 32, 8, 24, size.y - 16 };
	return {rect_.x + 2, rect_.y + 16, rect_.w - 4, rect_.h - 32};
}

bool updateMinimapHeight()
{
	recti rect = myGetMapRect();

	//If the vertical size has changed, update the minimap
	if (rect.h != myNotesH)
	{
		myNotesH = min(MAP_HEIGHT, rect.h);
		onChanges(VCM_VIEW_CHANGED);
		return true;
	}

	return false;
}

double myGetMapOffset(int y)
{
	double tor = 0.0;
	if(myNotesH > 0)
	{
		int topY, bottomY;
		recti chartRect = myGetMapRect();
		updateMinimapHeight();
		topY = chartRect.y;
		bottomY = chartRect.y + chartRect.h;

		double t = clamp((double)(y - topY) / (double)(bottomY - topY), 0.0, 1.0);
		if(gView->hasReverseScroll()) t = 1.0 - t;

		tor = myChartBeginOfs + (myChartEndOfs - myChartBeginOfs) * t;
	}
	return tor;
}

void onChanges(int changes)
{
	int bits = VCM_NOTES_CHANGED | VCM_TEMPO_CHANGED | VCM_VIEW_CHANGED | VCM_END_ROW_CHANGED;

	if(myMode == NOTES) bits |= VCM_SELECTION_CHANGED;

	if((changes & bits) == 0) return;

	std::vector<uint> buffer(MAP_HEIGHT * MAP_WIDTH, 0);

	// The height of the chart region is based on the time elapsed between the first and last row.
	double timeStart = gTempo->rowToTime(0);
	double timeEnd = gTempo->rowToTime(gSimfile->getEndRow());

	if(gView->isTimeBased())
	{
		myChartBeginOfs = timeStart;
		myChartEndOfs = timeEnd;
	}
	else
	{
		myChartBeginOfs = 0.0;
		myChartEndOfs = (double)gSimfile->getEndRow();
	}

	if(gChart->isOpen())
	{
		// Calculate the x-position of every note column.
		int cols = gStyle->getNumCols();
		int colw = (cols <= 8) ? 2 : 1;
		int coldx = (cols <= 4) ? 3 : (cols <= 8) ? 2 : 1;
		int colx[SIM_MAX_COLUMNS] = {};
		for(int c = 0; c < cols; ++c)
		{
			colx[c] = MAP_WIDTH / 2 + (c - cols / 2) * coldx;
		}

		auto rect = myGetMapRect();
		double pixPerOfs = (double) rect.h / (myChartEndOfs - myChartBeginOfs);
		SetPixelData spd = {buffer.data(), colw, pixPerOfs, myChartBeginOfs};

		if(myMode == DENSITY)
		{
			renderDensity(spd, colx);
		}
		else
		{
			renderNotes(spd, colx);
		}
	}

	// Update the texture strips.
	for(int i = 0; i < NUM_PIECES; ++i)
	{
		uint* buf = buffer.data();
		auto src = (const uchar*)(buf + i * MAP_WIDTH * TEXTURE_SIZE);
		myImage.modify(i * MAP_WIDTH, 0, MAP_WIDTH, TEXTURE_SIZE, src);
	}
}

void onMousePress(MousePress& evt)
{
	if(evt.button == Mouse::LMB && !gTextOverlay->isOpen() && evt.unhandled())
	{
		if(IsInside(rect_, evt.x, evt.y))
		{
			gView->setCursorOffset(myGetMapOffset(evt.y));
			myIsDragging = true;
			evt.setHandled();
		}
	}
}

void onMouseRelease(MouseRelease& evt)
{
	if(evt.button == Mouse::LMB && myIsDragging)
	{
		gView->setCursorOffset(myGetMapOffset(evt.y));
		myIsDragging = false;
	}
}

void tick()
{
	vec2i size = gSystem->getWindowSize();
	rect_ = { size.x - 32, 8, 24, size.y - 16 };

	if(myIsDragging)
	{
		int y = gSystem->getMousePos().y;
		gView->setCursorOffset(myGetMapOffset(y));
	}
}

void drawRegion(int x, int w, double ofsA, double ofsB, color32 color)
{
	recti chartRect = myGetMapRect();
	double pixPerOfs = (double)chartRect.h / (myChartEndOfs - myChartBeginOfs);
	int baseY = chartRect.y;
	if(gView->hasReverseScroll())
	{
		baseY += chartRect.h;
		pixPerOfs = -pixPerOfs;
	}
	int y1 = baseY + (int)((ofsA - myChartBeginOfs) * pixPerOfs + 0.5);
	int y2 = baseY + (int)((ofsB - myChartBeginOfs) * pixPerOfs + 0.5);
	if(y1 > y2) swapValues(y1, y2);

	Draw::fill({x, y1, w, max(y2 - y1, 1)}, color);
}

void draw()
{
	// If the text overlay is open, we hide the minimap to avoid clutter.
	if(gTextOverlay->isOpen()) return;

	updateMinimapHeight();

	// Draw the minimap box outline.
	recti rect(rect_);
	Draw::fill(rect, Colors::white);
	rect = Shrink(rect, 1);
	Draw::fill(rect, Colors::black);
	rect = Shrink(rect, 1);
	Renderer::pushScissorRect(rect);
	rect = myGetMapRect();

	// Draw the region selection if there is one.
	if(gSelection->getType() == Selection::REGION)
	{
		auto region = gSelection->getSelectedRegion();
		double top = gView->rowToOffset(region.beginRow);
		double btm = gView->rowToOffset(region.endRow);
		drawRegion(rect.x, rect.w, top, btm, RGBAtoColor32(153, 255, 153, 153));
	}

	// Draw the marker lines for the first and last row.
	Draw::fill(SideT(rect, 1), RGBAtoColor32(128, 128, 128, 255));
	Draw::fill(SideB(rect, 1), RGBAtoColor32(128, 128, 128, 255));

	// Draw the view range.
	if(myNotesH > 0)
	{
		double top = gView->yToOffset(0);
		double btm = gView->yToOffset(gView->getHeight());
		drawRegion(rect.x, rect.w, top, btm, RGBAtoColor32(255, 204, 102, 128));
	}

	// Draw the minimap image.
	int vp[NUM_PIECES * 8], y, dy;
	int xl = rect.x + rect.w / 2 - MAP_WIDTH / 2;
	int xr = xl + MAP_WIDTH;

	if(gView->hasReverseScroll())
	{
		y = rect.y + rect.h;
		dy = -TEXTURE_SIZE;
	}
	else
	{
		y = rect.y;
		dy = TEXTURE_SIZE;
	}

	for(int i = 0; i < NUM_PIECES; ++i, y += dy)
	{
		int y2 = y + dy;
		int v[8] = {xl, y, xr, y, xl, y2, xr, y2};
		memcpy(vp + i * 8, v, sizeof(int) * 8);
	}

	Renderer::resetColor();
	Renderer::bindTexture(myImage.handle());
	Renderer::bindShader(Renderer::SH_TEXTURE);
	Renderer::drawQuads(NUM_PIECES, vp, myUvs);

	Renderer::popScissorRect();
}

void setMode(Mode mode)
{
	myMode = mode;
	gMenubar->update(Menubar::VIEW_MINIMAP);
	onChanges(VCM_ALL_CHANGES);
}

Minimap::Mode getMode() const
{
	return myMode;
}

}; // MinimapImpl

// ================================================================================================
// Minimap API.

Minimap* gMinimap = nullptr;

void Minimap::create()
{
	gMinimap = new MinimapImpl;
}

void Minimap::destroy()
{
	delete (MinimapImpl*)gMinimap;
	gMinimap = nullptr;
}

}; // namespace Vortex
