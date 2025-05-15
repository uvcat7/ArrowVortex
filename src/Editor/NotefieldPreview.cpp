#include <Editor/NotefieldPreview.h>

#include <math.h>
#include <stdint.h>

#include <System/Debug.h>

#include <Core/Draw.h>
#include <Core/Gui.h>
#include <Core/Utils.h>
#include <Core/ImageLoader.h>
#include <Core/QuadBatch.h>

#include <Simfile/TimingData.h>

#include <Managers/TempoMan.h>
#include <Managers/NoteskinMan.h>
#include <Managers/StyleMan.h>
#include <Managers/ChartMan.h>
#include <Managers/NoteMan.h>

#include <Editor/Music.h>
#include <Editor/View.h>

namespace Vortex {

namespace {

typedef View::Coords Coords;

struct DrawPosHelper
{
	DrawPosHelper()
		: tracker(gTempo->getTimingData())
	{
		deltaY = -gView->getPixPerRow();
		baseY = -floor(gTempo->beatToScroll(gView->getCursorBeat()) * deltaY);
		advanceFunc = RowBasedAdvance;
		getFunc = RowBasedGet;
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
		return (int)(dp->baseY + dp->deltaY * gTempo->rowToScroll(row));
	}
	static int RowBasedGet(const DrawPosHelper* dp, int row, double time)
	{
		return (int)(dp->baseY + dp->deltaY * gTempo->rowToScroll(row));
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
// NotefieldPreviewImpl :: member data.

struct NotefieldPreviewImpl : public NotefieldPreview {

Texture myNoteLabelsTex;
BatchSprite myNoteLabels[2];

int myColX[SIM_MAX_COLUMNS], myX, myY, myW;

bool myEnabled;

// ================================================================================================
// NotefieldPreviewImpl :: constructor and destructor.

~NotefieldPreviewImpl()
{
}

NotefieldPreviewImpl()
{
	myEnabled = true;
	myNoteLabelsTex = Texture("assets/note labels.png", false);

	BatchSprite::init(myNoteLabels, 2, 2, 1, 32, 32);
}

// ================================================================================================
// NotefieldPreviewImpl :: drawing routines.
void updateNotefieldSize()
{
	Coords coords = gView->getReceptorCoords();
	int ofs = gView->applyZoom(32);
	coords.xl -= ofs, coords.xr += ofs;

	myW = coords.xr - coords.xl;
	myX = coords.xl + gView->getPreviewOffset();
	myY = coords.y;
}

void draw()
{
	if(!myEnabled || gChart->isClosed()) return;

	int scale = gView->getNoteScale();

	updateNotefieldSize();

	BatchSprite::setScale(scale);

	// Calculate the x-position of each note column.
	auto noteskin = gNoteskin->get();
	if(noteskin)
	{
		for(int c = 0; c < gStyle->getNumCols(); ++c)
		{
			myColX[c] = gView->columnToX(c) + gView->getPreviewOffset();
		}
	}

	recti view = gView->getRect();
	Draw::fill({ myX, view.y, myW, view.h }, COLOR32(0, 0, 0, 128));
	Draw::fill({ myX, view.y, 1, view.h }, COLOR32(255, 255, 255, 128));
	Draw::fill({ myX+myW, view.y, 1, view.h }, COLOR32(255, 255, 255, 128));

	drawReceptors();
	drawNotes();
	if(!gMusic->isPaused()) drawReceptorGlow();
}

// ================================================================================================
// NotefieldPreviewImpl :: segments.

void drawReceptors()
{
	const int cols = gStyle->getNumCols();

	auto noteskin = gNoteskin->get();

	Renderer::resetColor();
	Renderer::bindShader(Renderer::SH_TEXTURE);
	Renderer::bindTexture(noteskin->recepTex.handle());
	
	// Calculate the beat pulse value for the receptors.
	double beat = gTempo->timeToBeat(gView->getCursorTime());
	float beatfrac = (float)(beat - floor(beat));
	uchar beatpulse = (uchar)min(max((int)((2 - beatfrac * 4)*255), 0), 255);

	// Draw the receptors.
	auto batch = Renderer::batchTC();
	for(int c = 0; c < cols; ++c)
	{
		noteskin->recepOff[c].draw(&batch, myColX[c], myY, (uchar)255);
		noteskin->recepOn[c].draw(&batch, myColX[c], myY, beatpulse);
	}
	batch.flush();
}

void drawReceptorGlow()
{
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
		if(note->isMine | note->isWarped | (note->type == NOTE_FAKE)) continue;

		double lum = 1.5 - (time - note->endtime) * 6.0;
		uchar alpha = (uchar)clamp((int)(lum * 255.0), 0, 255);
		if(alpha > 0)
		{
			noteskin->recepGlow[c].draw(&batch, myColX[c], myY, alpha);
		}
	}
	batch.flush();
}

void drawNotes()
{
	const int numCols = gStyle->getNumCols();
	const int scale = gView->getNoteScale();
	const int signedScale = gView->hasReverseScroll() ? -scale : scale;
	const int maxY = gView->getHeight() + 32;
	const int currentRow = gView->getCursorBeat() * ROWS_PER_BEAT;
	const double speed = gTempo->beatToSpeed(gView->getCursorBeat());

	auto noteskin = gNoteskin->get();

	Renderer::resetColor();
	Renderer::bindShader(Renderer::SH_TEXTURE);
	Renderer::bindTexture(noteskin->noteTex.handle());

	// Render arrows/holds/mines interleaved, so the z-order is correct.
	auto batch = Renderer::batchT();
	DrawPosHelper drawPos;
	for(auto& note : *gNotes)
	{
		// Determine the y-position of the note.
		int y = myY - drawPos.get(note.row, note.time), 
			by = myY - (note.row == note.endrow ? y : drawPos.get(note.endrow, note.endtime));

		// Simulate chart preview. We want to not show arrows that go past the targets (mines go past the targets in Stepmania, so we keep those.)
		// Notes 20 beats into the future are also not rendered in game.
		if(note.type != NOTE_MINE && note.endrow < currentRow || note.row > currentRow + 20 * ROWS_PER_BEAT)
		{
			continue;
		}

		// Modify y to account for Tempo Speed
		y = myY + ((y - myY) * speed);
		by = myY + ((by - myY) * speed);

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

			// Only show the part of the tail past the targets, and don't show the arrow.
			if(currentRow > note.row && currentRow <= note.endrow)
			{
				body.draw(&batch, x, myY + bodyY, by + bodyY);
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
			int y = myY - drawPos.get(note.row, note.time);
			if(y < -32 || y > maxY) continue;
			int col = note.col, x = myColX[col];
			myNoteLabels[note.type == NOTE_FAKE].draw(&batch, x, y);
		}
	}
	batch.flush();
}

// ================================================================================================
// NotefieldPreviewImpl :: toggle/check functions.

void NotefieldPreviewImpl::toggleEnabled()
{
	myEnabled = !myEnabled;
	//gMenubar->update(Menubar::SHOW_NOTES);
}

};

// ================================================================================================
// NotefieldPreview API.

NotefieldPreview* gNotefieldPreview = nullptr;

void NotefieldPreview::create()
{
	gNotefieldPreview = new NotefieldPreviewImpl;
}

void NotefieldPreview::destroy()
{
	delete (NotefieldPreviewImpl*)gNotefieldPreview;
	gNotefieldPreview = nullptr;
}

}; // namespace Vortex