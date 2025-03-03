#include <Precomp.h>

#include <Vortex/View/Dialogs/DancingBot.h>

#include <Core/Utils/Util.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>

#include <Core/Interface/Widgets/Checkbox.h>

#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/NoteUtils.h>

#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/Managers/GameMan.h>
#include <Vortex/Edit/TempoTweaker.h>

#include <Vortex/View/View.h>

#define Dlg DialogDancingBot

namespace AV {

using namespace std;
using namespace Util;

enum class TileType
{
	Button,
	Empty,
};

template <typename R, typename T>
R Lerp(R begin, R end, T t)
{
	return begin + (end - begin) * t;
}

template <typename T>
T LerpDelta(T begin, T end, T x)
{
	return (x - begin) / (end - begin);
}

template <typename T>
T SmoothStep(T begin, T end, float t)
{
	return Lerp(begin, end, t*t*(3 - 2 * t));
}

// =====================================================================================================================
// Feet Planner.

/*

struct FeetPlanner
{
	enum class { Both = -1, LF = 0, RF = 1 };

	void setFoot(const Note* n, int foot, Vec2 pos);
	void setFeet(const Note* l, const Note* r, Vec2 posL, Vec2 posR);

	void planFootswitch(int foot, Vec2 pos);
	void planCrossover(int foot, Vec2 pos);

	void planStep();
	void planJump();
	void plan(int pn);

	struct NotePair { const Note* a, *b; };
	bool footswitch, crossover;
	NotePair curNote, nextNote[4];
	uint* outBits;
	const GameMode* mode;
	const Note* noteBegin;
	Vec2 curFeetPos[2];
	int lastUsedFoot;
	int lastUsedCol;
	int skipNotes;
};

void FeetPlanner::setFoot(const Note* n, int foot, Vec2 pos)
{
	int i = n - noteBegin;
	outBits[i >> 5] |= foot << (i & 31);
	lastUsedFoot = foot;
	lastUsedCol = n->col;
	curFeetPos[foot] = pos;
}

void FeetPlanner::setFeet(const Note* l, const Note* r, Vec2 posL, Vec2 posR)
{
	int il = l - noteBegin;
	int ir = r - noteBegin;
	outBits[il >> 5] |= LF << (il & 31);
	outBits[ir >> 5] |= RF << (ir & 31);
	curFeetPos[LF] = posL;
	curFeetPos[RF] = posR;
	lastUsedFoot = BOTH;
}

void FeetPlanner::planFootswitch(int foot, Vec2 posA)
{
	// See if we can perform a footswitch on the next few notes.
	if (footswitch && nextNote[0].a && !nextNote[0].b)
	{
		Vec2 posB = gameMode->padColPositions[nextNote[0].a->col];
		if ((foot == RF && posB.x < posA.x) ||
			(foot == LF && posB.x > posA.x))
		{
			setFoot(curNote.a, foot, posA);
			setFoot(nextNote[0].a, 1 - foot, posB);
			++skipNotes;
			return;
		}
		else if (posA.x == posB.x && nextNote[1].a && !nextNote[1].b)
		{
			Vec2 posC = gameMode->padColPositions[nextNote[1].a->col];
			if ((foot == RF && posC.x > posA.x) ||
				(foot == LF && posC.x < posA.x))
			{
				setFoot(curNote.a, foot, posA);
				setFoot(nextNote[0].a, 1 - foot, posB);
				++skipNotes;
				return;
			}
		}
	}

	// No footswitch, just use the other foot.
	setFoot(curNote.a, 1 - foot, posA);
}

void FeetPlanner::planCrossover(int foot, Vec2 posA)
{
	// See if we can perform a crossover on the next few notes.
	if (crossover && nextNote[0].a && !nextNote[0].b)
	{
		Vec2 posB = gameMode->padColPositions[nextNote[0].a->col];
		if (posA != posB)
		{
			if ((foot == LF && posB.x >= posA.x) ||
				(foot == RF && posB.x <= posA.x))
			{
				setFoot(curNote.a, foot, posA);
				setFoot(nextNote[0].a, 1 - foot, posB);
				++skipNotes;
				return;
			}
			else if (nextNote[1].a && !nextNote[1].b)
			{
				Vec2 posC = gameMode->padColPositions[nextNote[1].a->col];
				if (posB != posC)
				{
					if ((foot == LF && posC.x <= posB.x) ||
						(foot == RF && posC.x >= posB.x))
					{
						setFoot(curNote.a, foot, posA);
						setFoot(nextNote[0].a, 1 - foot, posB);
						setFoot(nextNote[1].a, foot, posC);
						skipNotes += 2;
						return;
					}
				}
			}
		}
	}

	// No crossover, just use the other foot.
	setFoot(curNote.a, 1 - foot, posA);
}

void FeetPlanner::planStep()
{
	auto n = curNote.a;
	Vec2 pos = gameMode->padColPositions[n->col];
	Vec2 posL = curFeetPos[LF], posR = curFeetPos[RF];
	if (lastUsedFoot == LF)
	{
		if (pos == posL)
		{
			planFootswitch(RF, pos);
		}
		else if (pos.x < posL.x)
		{
			planCrossover(RF, pos);
		}
		else
		{
			setFoot(n, RF, pos);
		}
	}
	else if (lastUsedFoot == RF)
	{
		if (pos == posR)
		{
			planFootswitch(LF, pos);
		}
		else if (pos.x > posR.x)
		{
			planCrossover(LF, pos);
		}
		else
		{
			setFoot(n, LF, pos);
		}
	}
	else if (pos == posL)
	{
		setFoot(n, LF, pos);
	}
	else if (pos == posR)
	{
		setFoot(n, RF, pos);
	}
	else if (pos.x < posL.x)
	{
		setFoot(n, LF, pos);
	}
	else if (pos.x > posR.x)
	{
		setFoot(n, RF, pos);
	}
	else
	{
		if (nextNote[0].a && !nextNote[0].b)
		{
			Vec2 nextPos = gameMode->padColPositions[nextNote[0].a->col];
			if (nextPos.x < pos.x)
			{
				setFoot(n, RF, pos);
			}
			else
			{
				setFoot(n, LF, pos);
			}
		}
		else
		{
			setFoot(n, RF, pos);
		}
	}
}

void FeetPlanner::planJump()
{
	auto a = curNote.a, b = curNote.b;
	Vec2 posA = gameMode->padColPositions[a->col];
	Vec2 posB = gameMode->padColPositions[b->col];
	if (posA.x < posB.x)
	{
		setFeet(a, b, posA, posB);
	}
	else if (posB.x < posA.x)
	{
		setFeet(b, a, posB, posA);
	}
	else if (posA == curFeetPos[LF] || posB == curFeetPos[RF])
	{
		setFeet(a, b, posA, posB);
	}
	else
	{
		setFeet(b, a, posB, posA);
	}
}

inline bool IgnoreNote(const Note* n)
{
	switch (n->type)
	{
	case NoteType::Mine:
	case NoteType::Fake:
	case NoteType::AUTOMATIC_KEYSOUND:
		return true;
	default:
		return false;
	}
}

inline bool IgnoreNote(const Note* n, int pn)
{
	switch (n->type)
	{
	case NoteType::Mine:
	case NoteType::Fake:
	case NoteType::AUTOMATIC_KEYSOUND:
		return true;
	default:
		return n->player != pn;
	}
}

void FeetPlanner::plan(int pn)
{
	auto chart = Editor::currentChart();
	if (!chart || chart->notes->list.isEmpty()) return;

	noteBegin = chart->notes->list.begin();

	lastUsedFoot = BOTH;
	lastUsedCol = -1;
	curFeetPos[0] = gameMode->padColPositions[gameMode->padInitialFeetCols[pn].x];
	curFeetPos[1] = gameMode->padColPositions[gameMode->padInitialFeetCols[pn].y];
	skipNotes = 0;

	// Fill up the next note array.
	memset(nextNote, 0, sizeof(NotePair) * 4);
	auto n = noteBegin;
	auto end = chart->notes->list.end();
	while (n != end && nextNote[0].a == nullptr)
	{
		// Move all upcomming notes one index ahead.
		nextNote[0] = nextNote[1];
		nextNote[1] = nextNote[2];
		nextNote[2] = nextNote[3];

		// Insert new notes in the last index.
		while (n != end && IgnoreNote(n, pn)) ++n;
		if (n != end)
		{
			auto m = nextNote[3].a = n++;
			while (n != end && IgnoreNote(n, pn)) n++;
			if (n != end && n->row == m->row) nextNote[3].b = n++;
			while (n != end && n->row == m->row) ++n;
		}
		else
		{
			nextNote[3] = {nullptr, nullptr};
		}
	}

	while (n != end || nextNote[0].a)
	{
		// Move all upcomming notes one index ahead.
		curNote = nextNote[0];
		nextNote[0] = nextNote[1];
		nextNote[1] = nextNote[2];
		nextNote[2] = nextNote[3];

		// Insert new notes in the last index.
		while (n != end && IgnoreNote(n, pn)) ++n;
		if (n != end)
		{
			nextNote[3].b = nullptr;
			auto m = nextNote[3].a = n++;
			while (n != end && IgnoreNote(n, pn)) n++;
			if (n != end && n->row == m->row) nextNote[3].b = n++;
			while (n != end && n->row == m->row) ++n;
		}
		else
		{
			nextNote[3] = {nullptr, nullptr};
		}

		// Skip notes that were already planned (as part of a crossover/footswitch).
		if (skipNotes > 0)
		{
			--skipNotes;
		}
		else if (curNote.b == nullptr) // Most common case, single note.
		{
			planStep();
		}
		else // Multiple notes on the same row, a jump.
		{
			planJump();
		}
	}
}

*/

// =====================================================================================================================
// DialogDancingBot :: Dancing bot widget.

struct Dlg::PadWidget : public Widget
{
	PadWidget(Dlg* owner);
	void onMeasure() override;
	void draw(bool enabled) override;
	Dlg* myOwner;
};

Dlg::PadWidget::PadWidget(Dlg* owner)
	: myOwner(owner)
{
}

void Dlg::PadWidget::onMeasure()
{
	int w = 64, h = 24;
	auto chart = Editor::currentChart();
	if (chart && chart->gameMode->padWidth > 0)
	{
		w = max(w, chart->gameMode->padWidth * 64);
		h = max(h, chart->gameMode->padHeight * 64);
	}
	myWidth = myMinWidth = w;
	myHeight = myMinHeight = h;
}

void Dlg::PadWidget::draw(bool enabled)
{
	Rect r = myRect;
	r.l = r.cx() - myWidth / 2;
	r.t = r.cy() - myHeight / 2;
	r.r = r.l + myWidth;
	r.b = r.t + myHeight;
	myOwner->drawPad(r);
}

// =====================================================================================================================
// DialogDancingBot :: FormOptions.

struct Dlg::FormOptions
{
	FormOptions();

	shared_ptr<WCheckbox> footswitch;
	shared_ptr<WCheckbox> crossover;
	shared_ptr<WGrid> grid;
};

Dlg::FormOptions::FormOptions()
{
	// TODO: reimplement
	/*
	const WGrid::Span sRows[1] = {
		{24, 10000, 0, 4},
	};
	grid.setRows(sRows, 1);

	const WGrid::Span sCols[2] = {
		{96, 10000, 1, 4},
		{96, 10000, 1, 4},
	};
	grid.setCols(sCols, 2);

	const WGrid::Cell sCells[2] = {
		{0, 0, 1, 1, &cFootswitches},
		{0, 1, 1, 1, &cCrossovers},
	};
	grid.setCells(sCells, 2);
	*/
}

// =====================================================================================================================
// Dancing Bot Dialog.

Dlg::~Dlg()
{
	delete myFormOptions;
}

Dlg::Dlg()
{
	setTitle("DANCING BOT");

	setResizeBehaviorH(ResizeBehavior::Unbound);
	setResizeBehaviorV(ResizeBehavior::Bound);

	SpriteEx::fromTileset(myPadSpr, 6, 4, 2, 64, 64);
	SpriteEx::fromTileset(myFeetSpr, 2, 2, 1, 96, 96);
	myPadTex = Texture("assets/dancing bot pad.png");
	myFeetTex = Texture("assets/dancing bot feet.png", TextureFormat::RGBA);

	myInitWidgets();
	myAssignFeetToNotes();

	mySubscriptions.add<Editor::ActiveChartChanged>(this, &Dlg::myHandleChartChanged);
	mySubscriptions.add<Chart::NotesChangedEvent>(this, &Dlg::myAssignFeetToNotes);
}

void Dlg::draw(bool enabled)
{
}

void Dlg::myInitWidgets()
{
	// ==========================================
	// FormOptions

	auto options = myFormOptions = new FormOptions;

	auto footswitch = make_shared<WCheckbox>("Footswitch");
	footswitch->onChange = bind(&Dlg::onDoFootswitchesChanged, this);
	footswitch->setTooltip("If enabled, the dancing bot will attempt footswitches");
	myFormOptions->footswitch = footswitch;

	auto crossover = make_shared<WCheckbox>("Crossover");
	crossover->onChange = bind(&Dlg::onDoCrossoversChanged, this);
	crossover->setTooltip("If enabled, the dancing bot will attempt crossovers");
	myFormOptions->crossover = crossover;

	// ==========================================
	// Tabs

	myPadWidget = make_shared<PadWidget>(this);
	myContent = myTabs = make_shared<WVerticalTabs>();

	auto tabPad = myTabs->addTab(myPadWidget, "Pad");
	auto tabOptions = myTabs->addTab(options->grid, "Options");
}

void Dlg::drawPad(Rect rect)
{
	auto chart = Editor::currentChart();
	auto tempo = Editor::currentTempo();

	/*
	if (chart && myFeetBits.size() && chart->gameMode->padWidth > 0)
	{
		auto gameMode = chart->gameMode;

		double startTime = View::getCursorTime();
		auto prevNotes = GetNotesBeforeTime(chart, startTime);

		Renderer::setShader(Renderer::Shader::FLAT_TEXTURE);
		Renderer::bindTexture(myPadTex);

		// utility functions.

		auto getDrawPos = [&](Vec2 colRow) -> Vec2
		{
			return {
				rect.l + 32 + colRow.x * 64,
				rect.t + 32 + colRow.y * 64
			};
		};

		auto pushArrow = [&](QuadBatchTC* batch, int x, int y)
		{
			static int spr[9] = {0, 1, 0, 1, 2, 1, 0, 1, 0};
			static float rot[9] = {2, 2, 3, 1, 0, 3, 1, 0, 0};
			int i = (y % 3) * 3 + (x % 3);
			vec2f pos = ToVec2f(getDrawPos(Vec2(x, y)));
			myPadSpr[spr[i] + 3].draw(batch, pos.x, pos.y, rot[i] * float(M_PI / 2.0), 1.f);
		};

		// Pad layout.
		auto batch = Renderer::startBatch(Renderer::FillType::COLORED_TEXTURED_QUADS);
		for (int x = 0; x < gameMode->padWidth; ++x)
		{
			for (int y = 0; y < gameMode->padHeight; ++y)
			{
				int tile = myPadLayout[y * gameMode->padWidth + x];
				Vec2 pos = getDrawPos(Vec2(x, y));
				myPadSpr[tile].draw(&batch, 256, pos.x, pos.y, (uchar)255);
				if (tile == TT_BUTTON) pushArrow(&batch, x, y);
			}
		}

		// Panel highlights.
		auto& timing = tempo->timing;
		for (int col = 0; col < gameMode->numCols; ++col)
		{
			int alpha = 0;
			auto n = prevNotes[col];
			if (n)
			{
				double endTime = timing.rowToTime(n->endRow);
				double dist = (n && !IgnoreNote(n)) ? (startTime - endTime) : 1000.0;
				alpha = min(max(int((1.5 - dist * 6.0) * 255.0), 0), 255);
			}
			if (alpha > 0)
			{
				Vec2 pos = getDrawPos(gameMode->padColPositions[col]);
				myPadSpr[2].draw(&batch, 256, pos.x, pos.y, (uchar)alpha);
			}
		}
		batch.flush();

		// Feet sprites.
		Renderer::bindTexture(myFeetTex);
		batch = Renderer::startBatch(Renderer::FillType::COLORED_TEXTURED_QUADS);
		for (int p = 0; p < gameMode->numPlayers; ++p)
		{
			uint color = p ? ToColor32({.5f, .5f, 1, 1}) : ToColor32({1, .5f, .5f, 1});

			// Get current and target note for each foot.
			vec3f feetPos[2];
			myGetFeetPositions(rect, feetPos, p);
			vec3f l(feetPos[0]), r(feetPos[1]);

			// Determine feet rotations.
			float rotation = 0.f;
			if (abs(l.x - r.l) + abs(l.y - r.t) > 0.1f)
			{
				rotation = (float)atan2(r.t - l.y, r.l - l.x);
				rotation = min(max(rotation, -0.8f), 0.8f);
			}

			// Draw the feet.
			myFeetSpr[0].draw(&batch, l.x, l.y, rotation, l.z * 0.625f, color);
			myFeetSpr[1].draw(&batch, r.l, r.t, rotation, r.z * 0.625f, color);
		}
		batch.flush();
	}
	else // If there is no pad layout information.
	{
		TextStyle textStyle = UiText::regular;
		auto text = textStyle.arrange(TextAlign::MC, 12, "- nothing to dance -");
		Text::draw(rect);
	}*/
}

void Dlg::myHandleChartChanged()
{
	// Update the pad layout.
	auto chart = Editor::currentChart();
	if (chart == nullptr || chart->gameMode->padWidth == 0 || chart->gameMode->padHeight == 0)
	{
		myPadLayout.clear();
	}
	else
	{
		auto gameMode = chart->gameMode;

		// Copy the layout of the buttons.
		myPadLayout.resize(gameMode->padWidth * gameMode->padHeight);
		for (int i = 0; i < gameMode->padWidth * gameMode->padHeight; ++i)
		{
			myPadLayout[i] = (int)TileType::Empty;
		}
		for (int col = 0; col < gameMode->numCols; ++col)
		{
			Vec2 pos = gameMode->padColPositions[col];
			myPadLayout[pos.y * gameMode->padWidth + pos.x] = (int)TileType::Button;
		}
	}
}

void Dlg::onDoFootswitchesChanged()
{
	myAssignFeetToNotes();
}

void Dlg::onDoCrossoversChanged()
{
	myAssignFeetToNotes();
}

void Dlg::myAssignFeetToNotes()
{
	myFeetBits.clear();
	/*
	auto chart = Editor::currentChart();
	int numNotes = chart ? chart->notes->list.size() : 0;
	if (numNotes > 0 && chart->gameMode->padWidth > 0)
	{
		myFeetBits.resize(numNotes / 32 + 1);
		memset(myFeetBits.data(), 0, sizeof(uint) * myFeetBits.size());
		for (int pn = 0; pn != chart->gameMode->numPlayers; ++pn)
		{
			FeetPlanner planner;
			planner.gameMode = chart->gameMode;
			planner.outBits = myFeetBits.data();
			planner.crossover = myDoCrossovers;
			planner.footswitch = myDoFootswitches;
			planner.plan(pn);
		}
	}
	*/
}

void Dlg::myGetFeetPositions(Rect rect, FootCoord* out, int pn)
{
	auto chart = Editor::currentChart();
	auto tempo = Editor::currentTempo();

	int prevNote[2] = {-1, -1};
	int nextNote[2] = {-1, -1};

	out[0] = { 0, 0, 1 };
	out[1] = { 0, 0, 1 };

	/*
	double startTime = View::getCursorTime();
	Row row = View::getCursorRowI();
	auto notes = chart ? chart->notes->list.begin() : nullptr;
	int noteCount = chart ? chart->notes->list.size() : 0;
	auto feetbits = myFeetBits.data();

	// Find the previous note for each foot.
	for (int n = 0; n != noteCount && notes[n].row < row; ++n)
	{
		int foot = (feetbits[n >> 5] >> (n & 31)) & 1;
		if (IgnoreNote(notes + n, pn)) continue;
		prevNote[foot] = n;
	}

	// Find the next note for each foot.
	for (int n = noteCount - 1; n != -1 && notes[n].row >= row; --n)
	{
		int foot = (feetbits[n >> 5] >> (n & 31)) & 1;
		if (IgnoreNote(notes + n, pn)) continue;
		nextNote[foot] = n;
	}

	// Determine interpolated feet positions.
	auto gameMode = chart->gameMode;
	Vec2 initialPos[2] =
	{
		gameMode->padColPositions[gameMode->padInitialFeetCols[pn].x],
		gameMode->padColPositions[gameMode->padInitialFeetCols[pn].y]
	};
	auto& timing = tempo->timing;
	for (int f = 0; f < 2; ++f)
	{
		// Position of current button.
		double curTime;
		Vec2 curButton;
		if (prevNote[f] >= 0)
		{
			curButton = gameMode->padColPositions[notes[prevNote[f]].col];
			curTime = timing.rowToTime(notes[prevNote[f]].row);
		}
		else
		{
			curButton = initialPos[f];
			curTime = 0.0;
		}

		// Position of next button.
		double endtime = curTime;
		Vec2 endButton = curButton;
		if (nextNote[f] >= 0)
		{
			endButton = gameMode->padColPositions[notes[nextNote[f]].col];
			endtime = timing.rowToTime(notes[nextNote[f]].row);
		}

		// Interpolated feet position.
		vec2f curPos = ToVec2f(Vec2(
			rect.l + 32 + curButton.x * 64,
			rect.t + 32 + curButton.y * 64
		));
		vec2f endPos = ToVec2f(Vec2(
			rect.l + 32 + endButton.x * 64,
			rect.t + 32 + endButton.y * 64
		));
		if (endtime > curTime)
		{
			double startTime = max(curTime, endtime - 0.5);
			double delta = LerpDelta(startTime, endtime, startTime);
			curPos = SmoothStep(curPos, endPos, (float)min(max(delta, 0.0), 1.0));
		}

		// Determine feet scale.
		double dt = min(fabs(curTime - startTime), fabs(endtime - startTime));
		float scale = (float)min(1.0, 0.8 + dt * 6.0);

		out[f] = {curPos.x, curPos.y, scale};
	}
	*/
}

} // namespace AV
