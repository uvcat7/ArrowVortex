#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Xmr.h>

#include <Core/System/System.h>
#include <Core/System/File.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Image.h>
#include <Core/Graphics/Text.h>
#include <Core/Graphics/Texture.h>

#include <Core/Interface/UiMan.h>
#include <Core/Interface/UiStyle.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo/TimingData.h>
#include <Simfile/Tempo/SegmentSet.h>
#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/NoteUtils.h>

#include <Vortex/Common.h>
#include <Vortex/Editor.h>
#include <Vortex/Settings.h>
#include <Vortex/Application.h>

#include <Vortex/Noteskin/NoteskinMan.h>
#include <Vortex/Managers/GameMan.h>
#include <Vortex/Edit/TempoTweaker.h>
#include <Vortex/Managers/MusicMan.h>
#include <Vortex/Managers/DialogMan.h>

#include <Vortex/Edit/Selection.h>
#include <Vortex/Edit/Editing.h>
#include <Vortex/Edit/Selection.h>

#include <Vortex/Notefield/Notefield.h>
#include <Vortex/Notefield/Background.h>
#include <Vortex/Notefield/Waveform.h>
#include <Vortex/Notefield/SegmentBoxes.h>
#include <Vortex/Notefield/NotefieldPosTracker.h>
#include <Vortex/Notefield/BeatLines.h>
#include <Vortex/Notefield/BeatRegions.h>

#include <Vortex/View/Hud.h>
#include <Vortex/View/View.h>

namespace AV {

using namespace std;
using namespace Util;

static int RoundUp(int value, int multiple)
{
	return ((value + multiple - 1) / multiple) * multiple;
}

struct TweakInfoBox : public InfoBox
{
	void draw(Rect r) override;
	int height() { return 100; }
};

// ======================================================================================================================
// Notefield :: state data.

struct NotefieldUi : public UiElement
{
	void draw(bool enabled) override;
};

namespace Notefield
{
	struct State
	{
		Sprite songBg;
	
		unique_ptr<Texture> selectionTex;
		unique_ptr<Texture> snapIconsTex;
		unique_ptr<Texture> tempoIconsTex;
		unique_ptr<Texture> noteLabelsTex;
		unique_ptr<Texture> tempoBoxesTex;
	
		SpriteEx snapIcons[int(SnapType::Count)];
		SpriteEx noteLabels[3];
	
		TweakInfoBox* infoBox = nullptr;
	
		float colX[SimfileConstants::MaxColumns] = {};
		float cX = 0.0;
		float x = 0.0;
		float y = 0.0;
		float w = 0.0;
	
		double firstVisibleTor = 0.0;
		double lastVisibleTor = 0.0;
	
		EventSubscriptions subscriptions;
	};
	static State* state = nullptr;
}
using Notefield::state;

// ======================================================================================================================
// Events.

static void HandleBackgroundChanged()
{
	auto sim = Editor::currentSimfile();
	if (sim && sim->background.get().length())
	{
		auto& filename = sim->background.get();
		auto path = FilePath(sim->directory, filename);
		state->songBg.texture = make_shared<Texture>(path, TextureFormat::RGBA);
		if (!state->songBg.texture)
			Hud::warning("Could not open background: %s", filename.data());
	}
	else
	{
		state->songBg.texture.reset();
	}
}

static void HandleSettingChanged(const Setting::Changed& arg)
{
	auto& settings = AV::Settings::view();
	if (arg == settings.bgBrightness)
		Hud::note("BG Brightness: %i%%", settings.bgBrightness.value());
}

// =====================================================================================================================
// Notefield :: constructor and destructor.

void Notefield::initialize(const XmrDoc& settings)
{
	state = new State();
	UiMan::add(make_unique<NotefieldUi>());

	state->selectionTex = make_unique<Texture>("assets/selection box.png");
	state->snapIconsTex = make_unique<Texture>("assets/icons snap.png");
	state->noteLabelsTex = make_unique<Texture>("assets/note labels.png");
	state->tempoBoxesTex = make_unique<Texture>("assets/icons tempo.png");

	SpriteEx::fromTileset(state->snapIcons, (size_t)SnapType::Count, 4, 4, 32, 32);
	SpriteEx::fromTileset(state->noteLabels, 3, 2, 2, 32, 32);

	// TODO: fix subscription.

	state->subscriptions.add<Simfile::BackgroundChanged>(HandleBackgroundChanged);
	state->subscriptions.add<Editor::ActiveSimfileChanged>(HandleBackgroundChanged);
	state->subscriptions.add<Setting::Changed>(HandleSettingChanged);
}

void Notefield::deinitialize()
{
	delete state->infoBox;
	Util::reset(state);
}

// =====================================================================================================================
// Notefield :: segments.

static void DrawReceptors()
{
	auto tempo = Editor::currentTempo();
	auto chart = Editor::currentChart();

	if (chart && tempo)
	{
		auto noteskin = NoteskinMan::getNoteskin(chart->gameMode);

		const int cols = noteskin->gameMode->numCols;
		const float scale = float(View::getZoomScale());
		SnapType snapType = View::getSnapType();

		// Calculate the beat pulse value for the receptors.
		double beat = tempo->timing.timeToRow(View::getCursorTime()) * BeatsPerRow;
		float beatfrac = float(beat - floor(beat));
		uchar beatpulse = (uchar)min(max(int((2 - beatfrac * 4) * 255), 0), 255);

		// Draw the receptors.
		Renderer::resetColor();
		for (int c = 0; c < cols; ++c)
		{
			auto& column = noteskin->columns[c];
			Renderer::bindTextureAndShader(column.receptorsTex);
			Renderer::startBatch(Renderer::FillType::ColoredTexturedQuads, Renderer::VertexType::Float);
			column.receptorOff.drawBatched(scale, state->colX[c], state->y, (uchar)255);
			column.receptorOn.drawBatched(scale, state->colX[c], state->y, beatpulse);
			Renderer::flushBatch();
		}
	}
	else
	{
		// No receptor to draw, draw a line instead.
		auto c = View::getReceptorCoords();
		Draw::fill(Rectf(c.xl, c.y, c.xr, c.y + 1), Color::White);
	}
}

static void DrawReceptorGlow()
{
	auto tempo = Editor::currentTempo();
	auto chart = Editor::currentChart();

	if (!chart) return;

	const float scale = float(View::getZoomScale());
	const int numCols = chart->gameMode->numCols;
	double cursorTime = View::getCursorTime();
	int cursorRow = View::getCursorRowI();
	auto noteskin = NoteskinMan::getNoteskin(chart->gameMode);

	Renderer::resetColor();
	Renderer::setBlendMode(Renderer::BlendMode::Add);
	for (int c = 0; c < numCols; ++c)
	{
		auto& column = noteskin->columns[c];

		Renderer::bindTextureAndShader(column.glowTex);
		Renderer::startBatch(Renderer::FillType::ColoredTexturedQuads, Renderer::VertexType::Float);

		auto note = GetNoteBefore(chart, cursorRow, c);
		if (!note) continue;
		if (note->type == (uint)NoteType::Mine || note->isWarped || (note->type == (uint)NoteType::Fake)) continue;

		double endTime = tempo->timing.rowToTime(note->endRow);
		double lum = 1.5 - (cursorTime - endTime) * 6.0;
		uchar alpha = (uchar)clamp(int(lum * 255.0), 0, 255);
		if (alpha > 0)
			column.receptorGlow.drawBatched(scale, state->colX[c], state->y, alpha);

		Renderer::flushBatch();
	}
	Renderer::resetBlendMode();
}

static void DrawSnapDiamonds()
{
	SnapType snapType = View::getSnapType();
	float scale = float(View::getZoomScale());

	auto coords = View::getReceptorCoords();
	float x[2] = { coords.xl, coords.xr };

	Renderer::resetColor();
	Renderer::bindTextureAndShader(state->snapIconsTex.get());

	// Row snap diamonds.
	Renderer::startBatch(Renderer::FillType::TexturedQuads, Renderer::VertexType::Float);
	for (int i = 0; i < 2; ++i)
	{
		float vx = x[i] + float(View::getZoomScale() * (i * 40 - 20));
		state->snapIcons[int(snapType)].drawBatched(scale, vx, state->y);
	}
	Renderer::flushBatch();
}

static void DrawHold(float scale, float x, float y, float by, const SpriteEx& body, const SpriteEx& tail)
{
	float holdHeight = by - y;
	float tailHalfHeight = tail.dy() * scale;
	float tailHalfWidth = tail.dx() * scale;

	if (holdHeight < tailHalfHeight)
	{
		const float t = 0.5f - 0.5f * holdHeight / tailHalfHeight;
		const float* uvs = tail.uvs();
		float scaledUvs[] =
		{
			uvs[0] + (uvs[4] - uvs[0]) * t,
			uvs[1] + (uvs[5] - uvs[1]) * t,
			uvs[2] + (uvs[6] - uvs[2]) * t,
			uvs[3] + (uvs[7] - uvs[3]) * t,
			uvs[4],
			uvs[5],
			uvs[6],
			uvs[7],
		};
		Renderer::pushQuad(
			x - tailHalfWidth,
			y,
			x + tailHalfWidth,
			by + tailHalfHeight,
			scaledUvs);
	}
	else
	{
		float taily = by - tailHalfHeight;
		float bodyHalfHeight = body.dy() * scale;
		float bodyHalfWidth = body.dx() * scale;
		float t = (holdHeight - tailHalfHeight) / (bodyHalfHeight * 2);

		const float* uvs = body.uvs();
		float scaledUvs[] =
		{
			uvs[4] + (uvs[0] - uvs[4]) * t,
			uvs[5] + (uvs[1] - uvs[5]) * t,
			uvs[6] + (uvs[2] - uvs[6]) * t,
			uvs[7] + (uvs[3] - uvs[7]) * t,
			uvs[4],
			uvs[5],
			uvs[6],
			uvs[7],
		};
		Renderer::pushQuad(
			x - bodyHalfWidth,
			y,
			x + bodyHalfWidth,
			taily,
			scaledUvs);

		Renderer::pushQuad(
			x - tailHalfWidth,
			taily,
			x + tailHalfWidth,
			by + tailHalfHeight,
			tail.uvs());
	}
}

static void DrawNotes()
{
	auto tempo = Editor::currentTempo();
	auto chart = Editor::currentChart();
	if (!chart) return;

	auto& settings = Settings::view();

	auto noteskin = NoteskinMan::getNoteskin(chart->gameMode);
	const bool showPassedNotes = settings.showPassedNotes || MusicMan::isPaused();
	const int numCols = noteskin->gameMode->numCols;
	const int centerX = View::rect().cx();
	const float scale = float(View::getZoomScale());
	const float signedScale = settings.reverseScroll ? -scale : scale;
	const float minY = float(showPassedNotes ? -32 : int(View::getReceptorCoords().y));
	const float maxY = float(View::getHeight() + 32);
	const int numPlayers = noteskin->gameMode->numPlayers;

	Renderer::resetColor();
	Renderer::bindShader(Renderer::Shader::Texture);

	NotefieldPosTracker drawPos;

	/* TODO: enable

	// Render hold bodies.
	for (int col = 0; col < numCols; ++col)
	{
		auto& column = noteskin->columns[col];

		Renderer::bindTexture(column.holdsTex);
		Renderer::startBatch(Renderer::FillType::TexturedQuads, Renderer::VertexType::Float);

		drawPos.reset();
		float x = state->colX[col];
		for (auto& note : chart->notes[col])
		{
			if (note.row == note.endRow)
				continue;

			// Determine the y-position of the note.
			float y = float(drawPos.advance(note.row));
			float by = float(drawPos.advance(note.endRow));

			// Check if the note is visible.
			if (max(y, by) < minY || min(y, by) > maxY) continue;

			// If passed notes are hidden, clip the hold body to the receptor Y.
			if (!showPassedNotes)
				y = max(y, float(minY));

			// Draw the hold body sprite.
			if (note.type == (uint)NoteType::Roll)
				DrawHold(scale, x, y, by, column.rollBody, column.rollTail);
			else
				DrawHold(scale, x, y, by, column.holdBody, column.holdTail);
		}

		Renderer::flushBatch();
	}

	*/

	// Render arrows/holds/mines interleaved, so the z-order is correct.

	for (int col = 0; col < numCols; ++col)
	{
		auto& column = noteskin->columns[col];

		Renderer::bindTexture(column.notesTex);
		Renderer::startBatch(Renderer::FillType::TexturedQuads, Renderer::VertexType::Float);

		drawPos.reset();
		float x = float(state->colX[col]);
		for (auto& note : chart->notes[col])
		{
			// Determine the y-position of the note.
			float y = float(drawPos.advance(note.row));

			// Check if the note is visible.
			if (y < minY || y > maxY)
				continue;

			int quantization = int(ToQuantization(note.row));

			// Note sprite.
			switch ((NoteType)note.type)
			{
			case NoteType::Step:
			case NoteType::Hold:
			case NoteType::Roll:
			case NoteType::Lift:
			case NoteType::Fake: {
				column.players[note.player].notes[quantization].drawBatched(scale, x, y);
				break; }
			case NoteType::Mine: {
				column.players[note.player].mine.drawBatched(scale, x, y);
				break; }
			}
		}
		Renderer::flushBatch();
	}

	// Draw indicator sprites for fake notes and lift notes.
	Renderer::bindTexture(state->noteLabelsTex.get());
	Renderer::startBatch(Renderer::FillType::TexturedQuads, Renderer::VertexType::Float);
	for (int col = 0; col < numCols; ++col)
	{
		drawPos.reset();
		float x = state->colX[col];
		for (auto& note : chart->notes[col])
		{
			if (note.type == (uint)NoteType::Lift || note.type == (uint)NoteType::Fake || note.keysoundId != 0)
			{
				float y = float(drawPos.advance(note.row));
				if (y < -32 || y > maxY) continue;

				if (note.type == (uint)NoteType::Lift)
				{
					state->noteLabels[0].drawBatched(scale, x, y);
				}
				else if (note.type == (uint)NoteType::Fake)
				{
					state->noteLabels[1].drawBatched(scale, x, y);
				}
				else
				{
					state->noteLabels[2].drawBatched(scale, x, y);
				}
			}
		}
	}
	Renderer::flushBatch();

	// Draw the keysound text.
	for (int col = 0; col < numCols; ++col)
	{
		drawPos.reset();
		float x = state->colX[col];
		for (auto& note : chart->notes[col])
		{
			if (note.keysoundId != 0)
			{
				float y = float(drawPos.advance(note.row));
				if (y < -32 || y > maxY) continue;

				string str = String::fromUint(note.keysoundId);

				Text::setStyle(UiText::regular);
				Text::format(TextAlign::MC, str.data());
				Text::draw(int(x), int(y));
			}
		}
	}

	// Draw selection boxes over the selected notes.
	if (Selection::hasNoteSelection(chart))
	{
		auto tex = state->selectionTex.get();
		SpriteEx select(float(tex->width()), float(tex->height()));
		Renderer::bindTexture(state->selectionTex.get());
		Renderer::startBatch(Renderer::FillType::TexturedQuads, Renderer::VertexType::Float);
		for (int col = 0; col < numCols; ++col)
		{
			drawPos.reset();
			float x = state->colX[col];
			for (auto& note : chart->notes[col])
			{
				if (Selection::hasSelectedNote(col, note))
				{
					float y = float(drawPos.advance(note.row));
					if (y < -32 || y > maxY) continue;
					select.drawBatched(scale, x, y);
				}
			}
		}
		Renderer::flushBatch();
	}
}

// =====================================================================================================================
// Notefield :: drawing routines.

static void DrawBackground()
{
	Rect view = View::rect();

	// Background image.
	Background::draw(state->songBg);

	// Faded notefield overlay.
	auto& settings = Settings::view();
	if (settings.showWaveform && settings.isTimeBased)
	{
		Waveform::drawBackground();
	}
	else
	{
		Draw::fill(Rect(int(state->x), view.t, int(state->x + state->w), view.b), Color(0, 0, 0, 128));
	}
}

void NotefieldUi::draw(bool enabled)
{
	auto sim = Editor::currentSimfile();
	if (!sim) return;

	int cx = View::rect().cx();
	auto& settings = Settings::view();
	bool drawWaveform = settings.showWaveform && settings.isTimeBased;

	// Update the tweak info box if necessary.
	auto mode = TempoTweaker::getTweakMode();
	if (mode == TweakMode::None && state->infoBox)
	{
		Util::reset(state->infoBox);
	}
	else if (mode != TweakMode::None && !state->infoBox)
	{
		state->infoBox = new TweakInfoBox;
	}

	// Calculate the x-position of each note column.
	auto chart = Editor::currentChart();
	if (chart)
	{
		auto mode = chart->gameMode;
		auto noteskin = NoteskinMan::getNoteskin(mode);
		if (noteskin)
		{
			for (int c = 0; c < mode->numCols; ++c)
			{
				state->colX[c] = View::columnToX(c);
			}
			for (int c = mode->numCols; c < SimfileConstants::MaxColumns; ++c)
			{
				state->colX[c] = 0;
			}
		}
	}

	auto coords = View::getNotefieldCoords();
	state->x = coords.xl;
	state->y = coords.y;
	state->cX = coords.xc;
	state->w = coords.xr - coords.xl;

	// Determine the first and last startTime/row visible on screen.
	state->firstVisibleTor = View::yToOffset(-20);
	state->lastVisibleTor = View::yToOffset(View::getHeight() + 20);
	sort(state->firstVisibleTor, state->lastVisibleTor);

	// Draw stuff.
	DrawBackground();

	if (settings.showBeatLines)
		BeatLines::drawBeats(state->x, state->w);

	if (drawWaveform)
		Waveform::drawPeaks();

	BeatRegions::drawStopAndWarpRegions(state->x, state->w);

	if (settings.showTempoBoxes)
		SegmentBoxes::drawBoxes(state->x, state->x + state->w, state->tempoBoxesTex.get());

	DrawReceptors();
	DrawSnapDiamonds();

	if (Editor::currentChart())
	{
		if (settings.showNotes)
			DrawNotes();

		if (!MusicMan::isPaused())
			DrawReceptorGlow();
	}

	if (DialogMan::dialogIsOpen(DialogId::SongProperties))
		BeatRegions::drawSongPreview(state->x, state->w);

	Editing::drawGhostNotes();

	Selection::drawRegionSelection();
	Selection::drawSelectionBox();
}

void Notefield::drawGhostNote(int col, const Note& n)
{
	auto chart = Editor::currentChart();
	if (!chart) return;

	auto noteskin = NoteskinMan::getNoteskin(chart->gameMode);
	int numCols = noteskin->gameMode->numCols;

	if (col < 0 || col >= numCols) return;
	auto& column = noteskin->columns[col];

	float y = View::rowToY(n.row);

	const float scale = float(View::getZoomScale());
	const float signedScale = Settings::view().reverseScroll ? -scale : scale;

	Renderer::setColor(Color(255, 255, 255, 192));

	int index = col * 2 + (n.type == (uint)NoteType::Roll);
	float by = View::rowToY(n.endRow);
	float x = state->colX[col];

	if (n.endRow > n.row)
	{
		// Render ghost hold/roll body.
		Renderer::startBatch(Renderer::FillType::TexturedQuads, Renderer::VertexType::Float);
		Renderer::bindTextureAndShader(column.holdsTex);
		column.holdBody.drawBatched(scale, x, y, by);

		Renderer::flushBatch();
	}

	Renderer::bindTextureAndShader(column.notesTex);
	Renderer::startBatch(Renderer::FillType::TexturedQuads, Renderer::VertexType::Float);

	// Render ghost step.
	int quantization = int(ToQuantization(n.row));
	column.players[0].notes[quantization].drawBatched(scale, state->colX[col], y);

	Renderer::flushBatch();
}

// =====================================================================================================================
// TweakInfoBox.

void TweakInfoBox::draw(Rect r)
{
	int cx = r.cx();

	TextStyle textStyle = UiText::regular;
	auto mode = TempoTweaker::getTweakMode();

	const char* name[] = { "none", "offset", "BPM", "stop" };
	auto fmt = format("Tweak {} :: {:.3f}",
		name[int(mode)],
		TempoTweaker::getTweakValue());

	Text::setStyle(textStyle);
	Text::format(TextAlign::MC, fmt.data());
	Text::draw(r.l, r.t + 16);

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

	for (int i = 0; i < 4; ++i)
	{
		Text::setStyle(textStyle, Color(255, 255, 255, 128));
		Text::format(TextAlign::TR, keys[i]);
		Text::draw(cx - 8, r.t + 32 + i * 14);

		Text::setStyle(textStyle);
		Text::format(TextAlign::TL, desc[i]);
		Text::draw(cx + 8, r.t + 32 + i * 14);
	}
}

} // namespace AV
