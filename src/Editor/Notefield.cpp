#include <Editor/Notefield.h>

#include <math.h>
#include <stdint.h>

#include <Core/Draw.h>
#include <Core/Gui.h>
#include <Core/Reference.h>
#include <Core/Vector.h>
#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/QuadBatch.h>
#include <Core/Xmr.h>
#include <Core/ImageLoader.h>
#include <Core/Text.h>

#include <System/System.h>
#include <System/File.h>

#include <Simfile/TimingData.h>
#include <Simfile/SegmentGroup.h>

#include <Managers/MetadataMan.h>
#include <Managers/NoteskinMan.h>
#include <Managers/StyleMan.h>
#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Music.h>
#include <Editor/Selection.h>
#include <Editor/Editing.h>
#include <Editor/Common.h>
#include <Editor/NotefieldPreview.h>
#include <Editor/TextOverlay.h>
#include <Editor/View.h>
#include <Editor/Editor.h>
#include <Editor/Menubar.h>
#include <Editor/Action.h>
#include <Editor/Waveform.h>
#include <Editor/TempoBoxes.h>

namespace Vortex {

namespace {

static int RoundUp(int value, int multiple)
{
	return ((value + multiple - 1) / multiple) * multiple;
}

struct TweakInfoBox : public InfoBox
{
	void draw(recti r);
	int height() { return 100; }
};

struct DrawPosHelper
{
	DrawPosHelper()
		: tracker(gTempo->getTimingData())
		, baseY(gView->offsetToY(0))
		, deltaY(gView->getPixPerOfs())
	{
		if(gView->isTimeBased())
		{
			advanceFunc = TimeBasedAdvance;
			getFunc = TimeBasedGet;
		}
		else
		{
			advanceFunc = RowBasedAdvance;
			getFunc = RowBasedGet;
		}
	}

	inline int advance(int row)
	{
		return advanceFunc(this, row);
	}
	inline int get(int row, double time) const
	{
		return getFunc(this, row, time);
	}
	static int RowBasedAdvance(DrawPosHelper* dp, int row)
	{
		return (int)(dp->baseY + dp->deltaY * (double)row);
	}
	static int RowBasedGet(const DrawPosHelper* dp, int row, double time)
	{
		return (int)(dp->baseY + dp->deltaY * row);
	}
	static int TimeBasedAdvance(DrawPosHelper* dp, int row)
	{
		return (int)(dp->baseY + dp->deltaY * dp->tracker.advance(row));
	}
	static int TimeBasedGet(const DrawPosHelper* dp, int row, double time)
	{
		return (int)(dp->baseY + dp->deltaY * time);
	}

	typedef int (*AdvanceFunc)(DrawPosHelper*, int);
	typedef int (*GetFunc)(const DrawPosHelper*, int, double);

	double baseY, deltaY;
	TempoTimeTracker tracker;
	AdvanceFunc advanceFunc;
	GetFunc getFunc;
};

}; // anonymous namespace

// ================================================================================================
// NotefieldImpl :: member data.

struct NotefieldImpl : public Notefield {

Texture mySongBg;
Texture mySelectionTex;
Texture mySnapIconsTex;
Texture myTempoIconsTex;
Texture myNoteLabelsTex;

BatchSprite mySnapIcons[NUM_SNAP_TYPES];
BatchSprite myNoteLabels[2];

Reference<TweakInfoBox> myTweakInfoBox;

uint32_t mySongBgColor;
int myBgBrightness;

int myColX[SIM_MAX_COLUMNS], myCX, myX, myY, myW;
double myFirstVisibleTor, myLastVisibleTor;

bool myShowWaveform;
bool myShowBeatLines;
bool myShowNotes;
bool myShowSongPreview;

// ================================================================================================
// NotefieldImpl :: constructor and destructor.

~NotefieldImpl()
{
}

NotefieldImpl()
{
	mySongBgColor = RGBAtoColor32(255, 255, 255, 255);
	myBgBrightness = 50;

	myShowWaveform = true;
	myShowBeatLines = true;
	myShowNotes = true;
	myShowSongPreview = false;

	mySelectionTex = Texture("assets/selection box.png", true);
	mySnapIconsTex = Texture("assets/icons snap.png", false);
	myNoteLabelsTex = Texture("assets/note labels.png", false);

	BatchSprite::init(mySnapIcons, NUM_SNAP_TYPES, 4, 4, 32, 32);
	BatchSprite::init(myNoteLabels, 2, 2, 1, 32, 32);
}

// ================================================================================================
// NotefieldImpl :: member functions.

void onChanges(int changes)
{
	if(changes & VCM_BACKGROUND_PATH_CHANGED)
	{
		mySongBg = Texture();
		std::string filename;
		if(gSimfile->isOpen())
		{
			filename = gSimfile->get()->background;
		}
		if(filename.length())
		{
			std::string path = gSimfile->getDir() + filename;
			ImageLoader::Data img = ImageLoader::load(path.c_str(), ImageLoader::RGBA);
			if(img.pixels == nullptr)
			{
				HudWarning("Could not open \"%s\".", filename.c_str());
			}
			else
			{
				const uint8_t* src = img.pixels;
				int64_t rsum = 0, gsum = 0, bsum = 0;
				int numPixels = img.width * img.height;
				for(int i = 0; i < numPixels; ++i)
				{
					rsum += src[0];
					gsum += src[1];
					bsum += src[2];
					src += 4;
				}
				int r = (int)(0.5f * (float)rsum / (float)numPixels);
				int g = (int)(0.5f * (float)gsum / (float)numPixels);
				int b = (int)(0.5f * (float)bsum / (float)numPixels);
				mySongBgColor = RGBAtoColor32(r, g, b, 255);
				mySongBg = Texture(img.width, img.height, img.pixels, false, Texture::RGBA);
				ImageLoader::release(img);
			}
		}
	}
	if(changes & (VCM_ZOOM_CHANGED | VCM_MUSIC_IS_LOADED))
	{
		gWaveform->clearBlocks();
	}
}

void setBgAlpha(int percent)
{
	percent = min(max(percent, 0), 100);
	if(myBgBrightness != percent)
	{
		myBgBrightness = percent;
		HudNote("BG Brightness: %i%%", percent);
	}
}

int getBgAlpha()
{
	return myBgBrightness;
}

// ================================================================================================
// NotefieldImpl :: drawing routines.

void draw()
{
	int cx = CenterX(gView->getRect());
	int scale = gView->getNoteScale();
	bool drawWaveform = myShowWaveform && gView->isTimeBased();

	// Update the tweak info box if necessary.
	TempoMan::TweakMode mode = gTempo->getTweakMode();
	if(mode == TempoMan::TWEAK_NONE && myTweakInfoBox)
	{
		myTweakInfoBox.destroy();
	}
	else if(mode != TempoMan::TWEAK_NONE && !myTweakInfoBox)
	{
		myTweakInfoBox.create();
	}

	BatchSprite::setScale(scale);

	// Calculate the x-position of each note column.
	auto noteskin = gNoteskin->get();
	if(noteskin)
	{
		for(int c = 0; c < gStyle->getNumCols(); ++c)
		{
			myColX[c] = gView->columnToX(c);
		}
		for(int c = gStyle->getNumCols(); c < SIM_MAX_COLUMNS; ++c)
		{
			myColX[c] = 0;
		}
	}

	auto coords = gView->getNotefieldCoords();
	myX = coords.xl, myY = coords.y;
	myCX = coords.xc;
	myW = coords.xr - coords.xl;

	// Determine the first and last time/row visible on screen.
	myFirstVisibleTor = gView->yToOffset(-20);
	myLastVisibleTor = gView->yToOffset(gView->getHeight() + 20);
	if(myFirstVisibleTor > myLastVisibleTor)
	{
		swapValues(myFirstVisibleTor, myLastVisibleTor);
	}

	// Draw stuff.
	drawBackground();

	gNotefieldPreview->draw();

	if(myShowBeatLines) drawBeatLines();
	if(drawWaveform) gWaveform->drawPeaks();
	if(gTempoBoxes->hasShowBoxes()) drawStopsAndWarps();

	drawReceptors();
	drawSnapDiamonds();

	if(gChart->isOpen())
	{
		if(myShowNotes) drawNotes();
		if(!gMusic->isPaused()) drawReceptorGlow();
	}

	if(myShowSongPreview) drawSongPreviewArea();

	gEditing->drawGhostNotes();

	gTempoBoxes->draw();

	gSelection->drawRegionSelection();
	gSelection->drawSelectionBox();
}

void drawBackground()
{
	recti view = gView->getRect();

	// Background image.
	auto style = (BackgroundStyle)gEditor->getBackgroundStyle();	
	if(mySongBg.handle() && myBgBrightness != 0)
	{
		recti r = view;
		vec2i size = mySongBg.size();
		if(style == BG_STYLE_LETTERBOX || style == BG_STYLE_CROP)
		{
			double bgRatio = (double)size.x / (double)size.y;
			double viewRatio = (double)r.w / (double)r.h;
			
			if((bgRatio > viewRatio) == (style == BG_STYLE_LETTERBOX))
			{
				r.h = size.y * r.w / size.x;
				r.y = view.h / 2 - r.h / 2;
			}
			else
			{
				r.w = size.x * r.h / size.y;
				r.x = view.w / 2 - r.w / 2;
			}
		}

		int alpha = myBgBrightness * 255 / 100;
		if(style == BG_STYLE_LETTERBOX)
		{
			mySongBgColor = Color32a(mySongBgColor, alpha);
			Draw::fill(view, mySongBgColor);
		}
		Draw::fill(r, Color32(alpha), mySongBg.handle());
	}

	// Faded notefield overlay.
	if(myShowWaveform && gView->isTimeBased())
	{
		gWaveform->drawBackground();
	}
	else
	{
		Draw::fill({myX, view.y, myW, view.h}, RGBAtoColor32(0, 0, 0, 128));
	}
}

void drawBeatLines()
{
	Renderer::resetColor();
	Renderer::bindShader(Renderer::SH_COLOR);

	bool zoomedIn = (gView->getScaleLevel() >= 2);
	int viewH = gView->getHeight();

	// We keep track of the measure labels to render them afterwards.
	struct MeasureLabel { int measure, y; };
	Vector<MeasureLabel> labels(8);

	// Determine the first row and last row that should show beat lines.
	int drawBeginRow = max(0, gView->offsetToRow(myFirstVisibleTor));
	int drawEndRow = min(gView->offsetToRow(myLastVisibleTor), gSimfile->getEndRow()) + 1;

	auto& sigs = gTempo->getTimingData().sigs;
	auto it = sigs.begin(), end = sigs.end();

	// Skip over time signatures that are not drawn.
	auto next = it + 1;
	while(next != end && next->row <= drawBeginRow)
	{
		it = next, ++next;
	}

	// Skip over measures at the start of the time signature that are not drawn.
	int measure = it->measure, row = it->row;
	if(drawBeginRow > row + it->rowsPerMeasure)
	{
		int skip = (drawBeginRow - it->row - it->rowsPerMeasure) / it->rowsPerMeasure;
		measure += skip;
		row += skip * it->rowsPerMeasure;
	}

	// Start drawing measures and beat lines.
	auto batch = Renderer::batchC();
	uint32_t halfColor = ToColor32({1, 1, 1, 0.4f});
	uint32_t fullColor = ToColor32({1, 1, 1, 0.7f});
	DrawPosHelper drawPos;
	while(it != end && row < drawEndRow)
	{
		int endRow = drawEndRow;
		if(next != end) endRow = min(endRow, next->row);
		while(row < endRow)
		{
			// Measure line and measure label.
			int y = drawPos.advance(row);
			Draw::fill(&batch, {myX, y, myW, 1}, fullColor);
			labels.push_back({measure, y});

			// Beat lines.
			if(zoomedIn)
			{
				int beatRow = row + ROWS_PER_BEAT;
				int measureEnd = row + it->rowsPerMeasure;
				while(beatRow < measureEnd)
				{
					if(beatRow > drawEndRow)
						break;

					int y = drawPos.advance(beatRow);
					Draw::fill(&batch, {myX, y, myW, 1}, halfColor);
					beatRow += ROWS_PER_BEAT;
				}
			}

			++measure;
			row += it->rowsPerMeasure;
		}
		it = next, ++next;
	}
	batch.flush();

	// Draw the measure labels.
	TextStyle textStyle;
	int dist = gView->applyZoom(10) + 2;
	if(!zoomedIn) textStyle.fontSize = 9;
	for(const auto& label : labels)
	{
		std::string info = Str::val(label.measure);
		Text::arrange(Text::MR, textStyle, info.c_str());
		Text::draw(vec2i{myX - dist, label.y});
	}
}

// ================================================================================================
// NotefieldImpl :: segments.

bool validSegmentRegion(int& t, int& b, int viewTop, int viewBtm)
{
	bool draw = (t > viewTop && t < viewBtm) || (b > viewTop && b < viewBtm) || (t > viewTop && b < viewBtm) || (b > viewTop && t < viewBtm);
	if(draw)
	{
		t = clamp(t, viewTop, viewBtm);
		b = clamp(b, viewTop, viewBtm);
		return true;
	}
	return false;
}

void drawStopsAndWarps()
{
	Renderer::resetColor();
	Renderer::bindShader(Renderer::SH_COLOR);

	double oy = gView->offsetToY(0.0);
	double dy = gView->getPixPerOfs();
	int viewTop = gView->getRect().y;
	int viewBtm = viewTop + gView->getHeight();

	auto& events = gTempo->getTimingData().events;
	auto batch = Renderer::batchC();
	if(gView->isTimeBased())
	{
		for(auto it = events.begin(), end = events.end(); it != end; ++it)
		{
			if(it->rowTime > it->time)
			{
				int t = (int)(oy + dy * it->time);
				int b = (int)(oy + dy * it->rowTime);
				if(validSegmentRegion(t, b, viewTop, viewBtm))
				{
					uint32_t col = RGBAtoColor32(26, 128, 128, 128);
					Draw::fill(&batch, {myX, t, myW, b - t}, col);
				}
			}
			if(it->endTime > it->rowTime)
			{
				int t = (int)(oy + dy * it->rowTime);
				int b = (int)(oy + dy * it->endTime);
				if(validSegmentRegion(t, b, viewTop, viewBtm))
				{
					uint32_t col = RGBAtoColor32(128, 128, 51, 128);
					Draw::fill(&batch, {myX, t, myW, b - t}, col);
				}
			}
		}
	}
	else
	{
		for(auto it = events.begin(), last = events.end() - 1; it < last; ++it)
		{
			if(it->spr == 0.0)
			{
				int t = (int)(oy + dy * it->row);
				int b = (int)(oy + dy * (it + 1)->row);
				if(validSegmentRegion(t, b, viewTop, viewBtm))
				{
					uint32_t col = RGBAtoColor32(128, 26, 51, 128);
					Draw::fill(&batch, {myX, t, myW, b - t}, col);
				}
			}
		}
	}
	batch.flush();
}

void drawReceptors()
{
	const int cols = gStyle->getNumCols();
	const int scale = gView->getNoteScale();
	SnapType snapType = gView->getSnapType();

	if(gChart->isOpen())
	{
		auto noteskin = gNoteskin->get();

		Renderer::resetColor();
		Renderer::bindShader(Renderer::SH_TEXTURE);
		Renderer::bindTexture(noteskin->recepTex.handle());
	
		// Calculate the beat pulse value for the receptors.
		double beat = gTempo->timeToBeat(gView->getCursorTime());
		float beatfrac = (float)(beat - floor(beat));
		uint8_t beatpulse = (uint8_t)min(max((int)((2 - beatfrac * 4)*255), 0), 255);

		// Draw the receptors.
		auto batch = Renderer::batchTC();
		for(int c = 0; c < cols; ++c)
		{
			noteskin->recepOff[c].draw(&batch, myColX[c], myY, (uint8_t)255);
			noteskin->recepOn[c].draw(&batch, myColX[c], myY, beatpulse);
		}
		batch.flush();
	}
	else
	{
		// No receptor to draw, draw a line instead.
		auto c = gView->getReceptorCoords();
		Draw::fill({c.xl, c.y, c.xr - c.xl, 1}, Colors::white);
	}
}

void drawReceptorGlow()
{
	if(gChart->isClosed()) return;

	auto noteskin = gNoteskin->get();
	auto notes = gNotes->begin();
	const int numCols = gStyle->getNumCols();
	double time = gView->getCursorTime();
	auto prevNotes = gNotes->getNotesBeforeTime(time);

	Renderer::resetColor();
	Renderer::bindShader(Renderer::SH_TEXTURE);
	Renderer::bindTexture(noteskin->glowTex.handle());

	// Draw the receptor glow based on the time elapsed since the previous note.
	auto batch = Renderer::batchTC();
	for(int c = 0; c < numCols; ++c)
	{
		auto note = prevNotes[c];
		if(!note) continue;

		double lum = 1.5 - (time - note->endtime) * 6.0;
		uint8_t alpha = (uint8_t)clamp((int)(lum * 255.0), 0, 255);
		if(alpha > 0)
		{
			noteskin->recepGlow[c].draw(&batch, myColX[c], myY, alpha);
		}
	}
	batch.flush();
}

void drawSnapDiamonds()
{
	SnapType snapType = gView->getSnapType();

	auto coords = gView->getReceptorCoords();
	int x[2] = {coords.xl, coords.xr};

	Renderer::resetColor();
	Renderer::bindShader(Renderer::SH_TEXTURE);
	Renderer::bindTexture(mySnapIconsTex.handle());

	// Row snap diamonds.
	auto batch = Renderer::batchT();
	for(int i = 0; i < 2; ++i)
	{
		int vx = x[i] + gView->applyZoom(i * 40 - 20);
		mySnapIcons[snapType].draw(&batch, vx, myY);
	}
	batch.flush();

	// Snap quantization
	if (gView->getSnapType() != ST_NONE)
	{
		TextStyle textStyle;
		textStyle.fontSize = gView->getZoomLevel() >= 4 ? 11 : 9;

		std::string snap = Str::val(gView->getSnapQuant());

		for (int i = 0; i < 2; ++i)
		{
			int vx = x[i] + gView->applyZoom(i * 40 - 20);

			Text::arrange(Text::MC, textStyle, snap.c_str());
			Text::draw(vec2i{ vx, myY - 1 });
		}
	}
}

void drawNotes()
{
	const int numCols = gStyle->getNumCols();
	const int scale = gView->getNoteScale();
	const int signedScale = gView->hasReverseScroll() ? -scale : scale;
	const int maxY = gView->getHeight() + 32;
	const bool isPreview = !gMusic->isPaused() && gView->hasChartPreview();
	const double currentRow = gView->getCursorBeat() * ROWS_PER_BEAT;

	auto noteskin = gNoteskin->get();

	Renderer::resetColor();
	Renderer::bindShader(Renderer::SH_TEXTURE);
	Renderer::bindTexture(noteskin->noteTex.handle());

	int targetY = gView->getNotefieldCoords().y;

	// Render arrows/holds/mines interleaved, so the z-order is correct.
	bool hasLabels = false;	
	auto batch = Renderer::batchT();
	DrawPosHelper drawPos;
	for(auto& note : *gNotes)
	{
		// Determine the y-position of the note.
		int y = drawPos.get(note.row, note.time), by;
		if(note.row == note.endrow)
		{
			by = y;
		}
		else
		{
			by = drawPos.get(note.endrow, note.endtime);
		}

		// Simulate chart preview. We want to not show arrows that go past the targets (mines go past the targets in Stepmania, so we keep those.)
		if(isPreview && note.type != NOTE_MINE && note.endrow < currentRow)
		{
			continue;
		}

		// Don't show notes off the screen
		if(max(y, by) < -32 || min(y, by) > maxY) continue;

		int rowtype = ToRowType(note.row);
		int col = note.col, x = myColX[col];

		// Body and tail for holds.
		if(note.endrow > note.row)
		{
			int index = note.isRoll * numCols + col;

			auto& body = noteskin->holdBody[index];
			auto& tail = noteskin->holdTail[index];

			int bodyY = noteskin->holdY[index] * signedScale / 256;
			int tailH = tail.height * signedScale / 512;

			//If we are doing chart preview, only show the part of the tail past the targets, and don't show the arrow
			if(isPreview && currentRow >= note.row && currentRow <= note.endrow)
			{
				body.draw(&batch, x, targetY + bodyY, by + bodyY);
				tail.draw(&batch, x, by - tailH, by + tailH);
				continue;
			}
			else
			{
				body.draw(&batch, x, y + bodyY, by + bodyY);
				tail.draw(&batch, x, by - tailH, by + tailH);
			}
		}

		// Note sprite.
		switch(note.type)
		{
		case NOTE_STEP_OR_HOLD:
		case NOTE_ROLL: {
			int index = (note.player * numCols + note.col) * NUM_ROW_TYPES + rowtype;
			noteskin->note[index].draw(&batch, x, y);
			break; }
		case NOTE_MINE: {
			int index = note.player * numCols + note.col;
			noteskin->mine[index].draw(&batch, x, y);
			break; }
		case NOTE_LIFT:
		case NOTE_FAKE: {
			int index = (note.player * numCols + note.col) * NUM_ROW_TYPES + rowtype;
			noteskin->note[index].draw(&batch, x, y);
			hasLabels = true;
			break; }
		}
	}
	batch.flush();

	// Draw indicator sprites for fake notes and lift notes.
	Renderer::bindTexture(myNoteLabelsTex.handle());
	batch = Renderer::batchT();
	for(auto& note : *gNotes)
	{
		if(note.type == NOTE_LIFT || note.type == NOTE_FAKE)
		{
			int y = drawPos.get(note.row, note.time);
			if(y < -32 || y > maxY) continue;
			int col = note.col, x = myColX[col];
			myNoteLabels[note.type == NOTE_FAKE].draw(&batch, x, y);
		}
	}
	batch.flush();

	// Draw selection boxes over the selected notes.
	Renderer::bindTexture(mySelectionTex.handle());
	batch = Renderer::batchT();
	BatchSprite select(mySelectionTex.size().x, mySelectionTex.size().y);
	for(auto& note : *gNotes)
	{
		if(note.isSelected)
		{
			int y = drawPos.get(note.row, note.time);
			if(y < -32 || y > maxY) continue;
			select.draw(&batch, myColX[note.col], (int)y);
		}
	}
	batch.flush();
}

void drawGhostNote(const Note& n)
{
	if(n.col < 0 || (int)n.col >= gStyle->getNumCols()) return;

	auto noteskin = gNoteskin->get();
	int numCols = gStyle->getNumCols();
	int y = gView->rowToY(n.row);

	const int scale = gView->getNoteScale();
	const int signedScale = gView->hasReverseScroll() ? -scale : scale;

	// Render ghost hold.
	Renderer::setColor(RGBAtoColor32(255, 255, 255, 192));
	Renderer::bindShader(Renderer::SH_TEXTURE);
	Renderer::bindTexture(noteskin->noteTex.handle());

	auto batch = Renderer::batchT();
	if(n.endrow > n.row)
	{
		int index = (n.type == NOTE_ROLL) * numCols + n.col;

		auto& body = noteskin->holdBody[index];
		auto& tail = noteskin->holdTail[index];

		int byi = (int)gView->rowToY(n.endrow);
		int bodyY = noteskin->holdY[index] * signedScale / 256;
		int tailH = tail.height * signedScale / 512;

		body.draw(&batch,  myColX[n.col], y + bodyY, byi + bodyY);
		tail.draw(&batch,  myColX[n.col], byi - tailH, byi + tailH);
	}

	// Render ghost step.
	int index = ((n.player * numCols) + n.col) * NUM_ROW_TYPES + ToRowType(n.row);
	noteskin->note[index].draw(&batch, myColX[n.col], y);

	batch.flush();
}

void drawSongPreviewArea()
{
	double start = gSimfile->get()->previewStart;
	double end = start + gSimfile->get()->previewLength;
	if(end > start)
	{
		int yt = max(0, gView->timeToY(start));
		int yb = min(gView->getHeight(), gView->timeToY(end));
		Draw::fill({myX, yt, myW, yb - yt}, RGBAtoColor32(255, 255, 255, 64));
		if(gView->getScaleLevel() > 2)
		{
			TextStyle textStyle;
			Text::arrange(Text::TL, textStyle, "SONG PREVIEW");
			Text::draw(vec2i{myX + 2, yt + 2});
		}
	}
}

// ================================================================================================
// NotefieldImpl :: toggle/check functions.

void NotefieldImpl::toggleShowWaveform()
{
	myShowWaveform = !myShowWaveform;
	gMenubar->update(Menubar::SHOW_WAVEFORM);
}

void NotefieldImpl::toggleShowBeatLines()
{
	myShowBeatLines = !myShowBeatLines;
	gMenubar->update(Menubar::SHOW_BEATLINES);
}

void NotefieldImpl::toggleShowNotes()
{
	myShowNotes = !myShowNotes;
	gMenubar->update(Menubar::SHOW_NOTES);
}

void NotefieldImpl::toggleShowSongPreview()
{
	myShowSongPreview = !myShowSongPreview;
}

bool hasShowWaveform()
{
	return myShowWaveform;
}

bool hasShowBeatLines()
{
	return myShowBeatLines;
}

bool hasShowNotes()
{
	return myShowNotes;
}

bool hasShowSongPreview()
{
	return myShowSongPreview;
}

}; // NotefieldImpl

// ================================================================================================
// TweakInfoBox.

void TweakInfoBox::draw(recti r)
{
	int mode = gTempo->getTweakMode();
	r.x += r.w / 2;

	const char* name[] = {"none", "offset", "BPM", "stop"};
	Str::fmt str("Tweak %1 :: %2");
	str.arg(name[mode]).arg(gTempo->getTweakValue(), 3, 3);

	Text::arrange(Text::MC, str);
	Text::draw(vec2i{r.x, r.y + 16});

	const char* keys[] =
	{
		"scrollwheel + shift",
		"scrollwheel + alt",
		"escape / RMB",
		"return / LMB",
	};
	const char* desc[] =
	{
		"adjust (coarse)",
		"adjust (precise)",
		"cancel adjustment",
		"apply adjustment",
	};

	TextStyle textStyle;
	for(int i = 0; i < 4; ++i)
	{
		textStyle.textColor = RGBAtoColor32(255, 255, 255, 128);
		Text::arrange(Text::TR, textStyle, keys[i]);
		Text::draw(vec2i{r.x - 8, r.y + 32 + i * 14});

		textStyle.textColor = Colors::white;
		Text::arrange(Text::TL, textStyle, desc[i]);
		Text::draw(vec2i{r.x + 8, r.y + 32 + i * 14});
	}
}

// ================================================================================================
// Notefield API.

Notefield* gNotefield = nullptr;

void Notefield::create()
{
	gNotefield = new NotefieldImpl;
}

void Notefield::destroy()
{
	delete (NotefieldImpl*)gNotefield;
	gNotefield = nullptr;
}

}; // namespace Vortex