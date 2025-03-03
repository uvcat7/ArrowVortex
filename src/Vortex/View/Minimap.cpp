#include <Precomp.h>

#include <Vortex/View/Minimap.h>

#include <Core/Utils/Util.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Renderer.h>
#include <Core/Graphics/Texture.h>

#include <Core/System/Debug.h>
#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Interface/UiMan.h>
#include <Core/Interface/UiElement.h>

#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>
#include <Vortex/Settings.h>
#include <Vortex/Application.h>

#include <Vortex/Edit/TempoTweaker.h>
#include <Vortex/Managers/GameMan.h>

#include <Vortex/View/View.h>
#include <Vortex/View/TextOverlay.h>

#include <Vortex/Edit/Selection.h>
#include <Vortex/NoteField/Notefield.h>

namespace AV {

using namespace std;
using namespace Util;

static constexpr int MapHeight = 1024;
static constexpr int MapWidth = 16;

static constexpr int TextureWidth = 128;
static constexpr int TextureHeight = 128;

static constexpr int NumPieces = MapHeight / TextureHeight;

static const Color NoteColors[9] =
{
	Color(255,   0,   0, 255), // Red.
	Color( 12,  74, 206, 255), // Blue.
	Color(145,  12, 206, 255), // Purple.
	Color(255, 255,   0, 255), // Yellow.
	Color(206,  12, 113, 255), // Pink.
	Color(247, 148,  29, 255), // Orange.
	Color(105, 231, 245, 255), // Teal.
	Color(  0, 198,   0, 255), // Green.
	Color(132, 132, 132, 255), // Gray.
};
static const Color FreezeColor =
{
	Color(64, 128, 0, 255) // Green
};
static const Color RollColor =
{
	Color(96, 96, 128, 255), // Blue gray
};
static const Color MineColor =
{
	Color(192, 192, 192, 255), // Light gray
};

struct SetPixelData
{
	uint* pixels;
	int noteW;
	double pixPerOfs, startOfs;
};

static void SetPixels(const SetPixelData& spd, int x, double ofs, Color color)
{
	int y = clamp(int((ofs - spd.startOfs) * spd.pixPerOfs), 0, MapHeight - 1);
	uint* dst = spd.pixels + y * MapWidth + x;
	for (int i = 0; i < spd.noteW; ++i, ++dst)
		*dst = color.value;
}

static void SetPixels(const SetPixelData& spd, int x, double ofs, double end, Color color)
{
	int yt = clamp(int((ofs - spd.startOfs) * spd.pixPerOfs), 0, MapHeight - 1);
	int yb = clamp(int((end - spd.startOfs) * spd.pixPerOfs), 0, MapHeight - 1);
	for (int y = yt; y <= yb; ++y)
	{
		uint* dst = spd.pixels + y * MapWidth + x;
		for (int i = 0; i < spd.noteW; ++i, ++dst)
			*dst = color.value;
	}
}
 
// =====================================================================================================================
// Minimap :: state data.

struct MinimapUi : public UiElement
{
	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	void tick(bool enabled) override;
	void draw(bool enabled) override;
};

namespace Minimap
{
	struct State
	{
		Rect rect = {};
		Texture image = Texture(TextureWidth, TextureHeight);
	
		int notesH = 0;
		double chartBeginOfs = 0.0;
		double chartEndOfs = 0.0;
	
		bool isDragging = false;
		float uvs[NumPieces * 8] = {};
	
		EventSubscriptions subscriptions;
	};
	static State* state = nullptr;
}
using Minimap::state;

// =====================================================================================================================
// Minimap :: rendering functions.

static Rect GetMapRect()
{
	return state->rect.shrink(2, 16);
}

static Rect GetChartRect()
{
	Rect rect = GetMapRect();
	if (state->notesH > rect.h())
	{
		double cursorOfsY = View::getCursorOffset() - state->chartBeginOfs;
		double chartOfsH = state->chartEndOfs - state->chartBeginOfs;
		double t = cursorOfsY / chartOfsH;

		if (Settings::view().reverseScroll) t = 1.0 - t;

		rect.t -= int((double)(state->notesH - rect.h()) * t + 0.5);
	}
	else
	{
		rect.t += rect.cy() - state->notesH / 2;
	}
	rect.b = rect.t + state->notesH;
	return rect;
}

static double GetMapOffset(int y)
{
	double ofs = 0.0;
	if (state->notesH > 0)
	{
		int topY, bottomY;
		Rect mapRect = GetMapRect();
		if (state->notesH > mapRect.h())
		{
			topY = mapRect.t;
			bottomY = mapRect.b;
		}
		else
		{
			Rect chartRect = GetChartRect();
			topY = chartRect.t;
			bottomY = chartRect.b;
		}

		double t = clamp(((double)y - (double)topY) / ((double)bottomY - (double)topY), 0.0, 1.0);
		if (Settings::view().reverseScroll) t = 1.0 - t;

		ofs = state->chartBeginOfs + (state->chartEndOfs - state->chartBeginOfs) * t;
	}
	return ofs;
}

static void DrawRegion(int xl, int xr, double ofsA, double ofsB, Color color)
{
	Rect chartRect = GetChartRect();
	double pixPerOfs = (double)chartRect.h() / (state->chartEndOfs - state->chartBeginOfs);
	int baseY = chartRect.t;
	if (Settings::view().reverseScroll)
	{
		baseY = chartRect.b;
		pixPerOfs = -pixPerOfs;
	}
	int y1 = baseY + int((ofsA - state->chartBeginOfs) * pixPerOfs + 0.5);
	int y2 = baseY + int((ofsB - state->chartBeginOfs) * pixPerOfs + 0.5);
	sort(y1, y2);

	Draw::fill(Rect(xl, y1, xr, max(y1 + 1, y2)), color);
}

void RenderNotes(SetPixelData& spd, const int* colx)
{
	auto chart = Editor::currentChart();
	auto tempo = Editor::currentTempo();

	int numCols = chart->gameMode->numCols;

	if (Settings::view().isTimeBased)
	{
		for (int col = 0; col < numCols; ++col)
		{
			int x = colx[col];
			auto tracker = tempo->timing.timeTracker();
			for (auto& note : chart->notes[col])
			{
				if (note.type == (uint)NoteType::AutomaticKeysound) continue;

				int quantization = (int)ToQuantization(note.row);
				double time = tracker.advance(note.row);
				if (note.endRow > note.row)
				{
					Color color = (note.type == (uint)NoteType::Roll) ? RollColor : FreezeColor;
					SetPixels(spd, x, time, tracker.lookAhead(note.endRow), color);
				}
				Color color;
				if (Selection::hasSelectedNote(col, note))
				{
					color = Color::White;
				}
				else
				{
					color = (note.type == (uint)NoteType::Mine) ? MineColor : NoteColors[quantization];
				}
				SetPixels(spd, x, time, color);
			}
		}
	}
	else
	{
		for (int col = 0; col < numCols; ++col)
		{
			int x = colx[col];
			auto tracker = tempo->timing.timeTracker();
			for (auto& note : chart->notes[col])
			{
				if (note.type == (uint)NoteType::AutomaticKeysound)	continue;

				int quantization = (int)ToQuantization(note.row);
				if (note.endRow > note.row)
				{
					Color color = (note.type == (uint)NoteType::Roll) ? RollColor : FreezeColor;
					SetPixels(spd, x, note.row, note.endRow, color);
				}
				Color color;
				if (Selection::hasSelectedNote(col, note))
				{
					color = Color::White;
				}
				else
				{
					color = (note.type == (uint)NoteType::Mine) ? MineColor : NoteColors[quantization];
				}
				SetPixels(spd, x, note.row, color);
			}
		}
	}
}

static int BlendI32(int a, int b, int f)
{
	return a + (((b - a) * f) >> 8);
}

static void SetDensityRow(uint* pixels, int y, double density)
{
	static const uchar Colors[] =
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
	int r = BlendI32(Colors[ci + 0], Colors[ci + 3], bf);
	int g = BlendI32(Colors[ci + 1], Colors[ci + 4], bf);
	int b = BlendI32(Colors[ci + 2], Colors[ci + 5], bf);
	uint col = Color(r, g, b).value;

	int w = clamp(int(density * 0.05), 4, MapWidth) / 2;
	uint* dst = pixels + y * MapWidth + MapWidth / 2 - w;
	for (int i = -w; i < w; ++i, ++dst)
		*dst = col;
}

/*
void renderDensity(SetPixelData& spd, const int* colx)
{
	auto first = NotesMan::begin(), end = NotesMan::end(), it = first, last = end - 1;

	if (View::isTimeBased())
	{
		auto tracker = Editor::currentTempo()->timing.timeTracker();
		double sec = state->chartBeginOfs;
		double secPerPix = (double)(state->chartEndOfs - state->chartBeginOfs) / (double)state->notesH;
		for (int y = 0; y < state->notesH; ++y)
		{
			double density = 0.0;
			for (; it != end && tracker.advance(it->row) < sec; ++it);
			for (; it != end && tracker.advance(it->row) < sec + secPerPix; ++it)
			{
				if (it->type == NoteType::Mine || it->isWarped) continue;
				double pre = (it > first) ? (it - 1)->startTime : (it->startTime - 1.0);
				double post = (it < last) ? (it + 1)->startTime : (it->startTime + 1.0);
				if (post > pre) density = max(density, 2.0 / (post - pre));
			}
			if (density > 0.0) SetDensityRow(spd.pixels, y, density);
			sec += secPerPix;
		}
	}
	else
	{
		double row = state->chartBeginOfs;
		double rowPerPix = (double)(state->chartEndOfs - state->chartBeginOfs) / (double)state->notesH;
		for (int y = 0; y < state->notesH; ++y)
		{
			double density = 0.0;
			for (; it != end && it->row < row; ++it);
			for (; it != end && it->row < row + rowPerPix; ++it)
			{
				if (it->isMine || it->isWarped) continue;
				double pre = (it > first) ? (it - 1)->startTime : (it->startTime - 1.0);
				double post = (it < last) ? (it + 1)->startTime : (it->startTime + 1.0);
				if (post > pre) density = max(density, 2.0 / (post - pre));
			}
			if (density > 0.0) SetDensityRow(spd.pixels, y, density);
			row += rowPerPix;
		}
	}
}
*/

// =====================================================================================================================
// Minimap :: event handling.

void MinimapUi::onMousePress(MousePress& input)
{
	if (input.button == MouseButton::LMB && !TextOverlay::isOpen() && input.unhandled())
	{
		if (state->rect.contains(input.pos))
		{
			View::setCursorOffset(GetMapOffset(input.pos.y));
			state->isDragging = true;
			input.setHandled();
		}
	}
}

void MinimapUi::onMouseRelease(MouseRelease& input)
{
	if (input.button == MouseButton::LMB && state->isDragging)
	{
		View::setCursorOffset(GetMapOffset(input.pos.y));
		state->isDragging = false;
	}
}

static void RenderMinimap()
{
	auto sim = Editor::currentSimfile();
	auto chart = Editor::currentChart();
	auto tempo = Editor::currentTempo();

	vector<uint> buffer(MapHeight * MapWidth, 0);

	if (chart)
	{
		// The height of the chart region is based on the startTime elapsed between the first and last row.
		// The full map height is only used if the chart length exceeds 4 minutes (aka 240 seconds).
		auto& timing = tempo->timing;
		double timeStart = timing.rowToTime(0);
		double timeEnd = timing.rowToTime(View::getEndRow());
		state->notesH = clamp(int(MapHeight * (timeEnd - timeStart) / 240.0), 0, MapHeight);

		if (Settings::view().isTimeBased)
		{
			state->chartBeginOfs = timeStart;
			state->chartEndOfs = timeEnd;
		}
		else
		{
			state->chartBeginOfs = 0.0;
			state->chartEndOfs = (double)View::getEndRow();
		}

		// Calculate the x-position of every note column.
		int cols = chart->gameMode->numCols;
		int colw = (cols <= 8) ? 2 : 1;
		int coldx = (cols <= 4) ? 3 : (cols <= 8) ? 2 : 1;
		int colx[SimfileConstants::MaxColumns] = {};
		for (int c = 0; c < cols; ++c)
		{
			colx[c] = MapWidth / 2 + (c - cols / 2) * coldx;
		}

		double pixPerOfs = (double)state->notesH / (state->chartEndOfs - state->chartBeginOfs);
		SetPixelData spd = { buffer.data(), colw, pixPerOfs, state->chartBeginOfs };

		if (AV::Settings::view().minimapMode == Minimap::Mode::Density)
		{
			// TODO: reimplement
			//renderDensity(spd, colx);
		}
		else
		{
			RenderNotes(spd, colx);
		}
	}
	else
	{
		state->chartBeginOfs = 0.0;
		state->chartEndOfs = 120.0;
	}

	// Update the texture strips.
	for (int i = 0; i < NumPieces; ++i)
	{
		uint* buf = buffer.data();
		auto src = (const uchar*)(buf + i * MapWidth * TextureHeight);
		state->image.modify(src, i * MapWidth, 0, MapWidth, TextureHeight, MapWidth, PixelFormat::RGBA);
	}
}

void MinimapUi::tick(bool enabled)
{
	state->rect = View::rect().sliceR(40).shrink(8);
	if (state->isDragging)
	{
		int y = Window::getMousePos().y;
		View::setCursorOffset(GetMapOffset(y));
	}
}

static void HandleSettingChanged(const Setting::Changed& arg)
{
	if (arg == Settings::view().minimapMode)
		RenderMinimap();
}

// =====================================================================================================================
// Minimap :: constructor and destructor.

void Minimap::initialize(const XmrDoc& settings)
{
	state = new State();
	UiMan::add(make_unique<MinimapUi>());

	VortexAssert(TextureWidth * TextureHeight == MapHeight * MapWidth);

	// Split the texture area into vertical strips from left to right.
	float u = 0.f, du = (float)MapWidth / (float)TextureWidth;
	for (int i = 0; i < NumPieces; ++i, u += du)
	{
		float v[8] = {u, 0, u + du, 0, u, 1, u + du, 1};
		memcpy(state->uvs + i * 8, v, sizeof(float) * 8);
	}

	state->subscriptions.add<Setting::Changed>(HandleSettingChanged);
	state->subscriptions.add<Editor::ActiveChartChanged>(RenderMinimap);
	state->subscriptions.add<Selection::SelectionChanged>(RenderMinimap);
	state->subscriptions.add<View::EndRowChanged>(RenderMinimap);
	state->subscriptions.add<Chart::NotesChangedEvent>(RenderMinimap);
	state->subscriptions.add<Tempo::TimingChanged>(RenderMinimap);
}

void Minimap::deinitialize()
{
	Util::reset(state);
}

// =====================================================================================================================
// Minimap :: member functions.

void MinimapUi::draw(bool enabled)
{
	if (!Editor::currentSimfile()) return;

	// If the text overlay is open, we hide the minimap to avoid clutter.
	if (TextOverlay::isOpen()) return;

	// Draw the minimap box outline.
	Rect rect(state->rect);
	Draw::fill(rect, Color::White);
	rect = rect.shrink(1);
	Draw::fill(rect, Color::Black);
	rect = rect.shrink(1);
	Renderer::pushScissorRect(rect);
	rect = GetMapRect();

	// Draw the region selection if there is one.
	if (Selection::hasRegionSelection())
	{
		auto region = Selection::getRegion();
		double top = View::rowToOffset(region.beginRow);
		double btm = View::rowToOffset(region.endRow);
		DrawRegion(rect.l, rect.r, top, btm, Color(153, 255, 153));
	}

	// Draw the marker lines for the first and last row.
	Rect chartRect = GetChartRect();
	Draw::fill(chartRect.sliceT(1), Color(128, 128, 128));
	Draw::fill(chartRect.sliceB(1), Color(128, 128, 128));

	// Draw the view range.
	if (state->notesH > 0)
	{
		double top = View::yToOffset(0);
		double btm = View::yToOffset(View::getHeight());
		DrawRegion(rect.l, rect.r, top, btm, Color(255, 204, 102, 128));
	}

	// Draw the minimap image.
	int vp[NumPieces * 8], y, dy;
	int xl = chartRect.cx() - MapWidth / 2;
	int xr = xl + MapWidth;

	if (Settings::view().reverseScroll)
	{
		y = chartRect.b;
		dy = -TextureHeight;
	}
	else
	{
		y = chartRect.t;
		dy = TextureHeight;
	}

	for (int i = 0; i < NumPieces; ++i, y += dy)
	{
		int y2 = y + dy;
		int v[8] = {xl, y, xr, y, xl, y2, xr, y2};
		memcpy(vp + i * 8, v, sizeof(int) * 8);
	}

	Renderer::resetColor();
	Renderer::bindTextureAndShader(state->image);
	Renderer::drawQuads(NumPieces, vp, state->uvs);
	Renderer::popScissorRect();
}

// =====================================================================================================================
// Minimap :: Mode.

template<>
const char* Enum::toString<Minimap::Mode>(Minimap::Mode value)
{
	switch (value)
	{
	case Minimap::Mode::Notes:
		return "notes";
	case Minimap::Mode::Density:
		return "density";
	}
	return "unknown";
}

template<>
optional<Minimap::Mode> Enum::fromString<Minimap::Mode>(stringref str)
{
	if (str == "notes")
		return Minimap::Mode::Notes;
	if (str == "density")
		return Minimap::Mode::Density;
	return std::nullopt;
}

} // namespace AV
