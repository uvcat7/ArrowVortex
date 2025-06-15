#include <Dialogs/DancingBot.h>

#include <Core/Draw.h>
#include <Core/Utils.h>
#include <Core/Draw.h>
#include <Core/Widgets.h>
#include <Core/WidgetsLayout.h>

#include <Managers/NoteMan.h>
#include <Managers/ChartMan.h>
#include <Editor/View.h>
#include <Managers/StyleMan.h>
#include <Editor/Editor.h>

#define _USE_MATH_DEFINES
#include <math.h>

namespace Vortex {

enum TileType
{
	TT_BUTTON,
	TT_EMPTY,
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
	return Lerp(begin, end, t*t*(3-2*t));
}

// ================================================================================================
// Feet Planner.

struct FeetPlanner
{
	enum { BOTH = -1, LF = 0, RF = 1 };

	void setFoot(const ExpandedNote* n, int foot, vec2i pos);
	void setFeet(const ExpandedNote* l, const ExpandedNote* r, vec2i posL, vec2i posR);

	void planFootswitch(int foot, vec2i pos);
	void planCrossover(int foot, vec2i pos);

	void planStep();
	void planJump();
	void plan(int pn);

	struct NotePair { const ExpandedNote* a, *b; };
	bool footswitch, crossover;
	NotePair curNote, nextNote[4];
	uint* outBits;
	const Style* style;
	const ExpandedNote* noteBegin;
	vec2i curFeetPos[2];
	int lastUsedFoot;
	int skipNotes;
};

void FeetPlanner::setFoot(const ExpandedNote* n, int foot, vec2i pos)
{
	int i = n - noteBegin;
	outBits[i >> 5] |= foot << (i & 31);
	lastUsedFoot = foot;
	curFeetPos[foot] = pos;
}

void FeetPlanner::setFeet(const ExpandedNote* l, const ExpandedNote* r, vec2i posL, vec2i posR)
{
	int il = l - noteBegin;
	int ir = r - noteBegin;
	outBits[il >> 5] |= LF << (il & 31);
	outBits[ir >> 5] |= RF << (ir & 31);
	curFeetPos[LF] = posL;
	curFeetPos[RF] = posR;
	lastUsedFoot = BOTH;
}

void FeetPlanner::planFootswitch(int foot, vec2i posA)
{
	// See if we can perform a footswitch on the next few notes.
	if(footswitch && nextNote[0].a && !nextNote[0].b)
	{
		vec2i posB = style->padColPositions[nextNote[0].a->col];
		if((foot == RF && posB.x < posA.x) ||
		   (foot == LF && posB.x > posA.x))
		{
			setFoot(curNote.a, foot, posA);
			setFoot(nextNote[0].a, 1 - foot, posB);
			++skipNotes;
			return;
		}
		else if(posA.x == posB.x && nextNote[1].a && !nextNote[1].b)
		{
			vec2i posC = style->padColPositions[nextNote[1].a->col];
			if((foot == RF && posC.x > posA.x) ||
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

void FeetPlanner::planCrossover(int foot, vec2i posA)
{
	// See if we can perform a crossover on the next few notes.
	if(crossover && nextNote[0].a && !nextNote[0].b)
	{
		vec2i posB = style->padColPositions[nextNote[0].a->col];
		if(posA != posB)
		{
			if((foot == LF && posB.x >= posA.x) ||
			   (foot == RF && posB.x <= posA.x))
			{
				setFoot(curNote.a, foot, posA);
				setFoot(nextNote[0].a, 1 - foot, posB);
				++skipNotes;
				return;
			}
			else if(nextNote[1].a && !nextNote[1].b)
			{
				vec2i posC = style->padColPositions[nextNote[1].a->col];
				if(posB != posC)
				{
					if((foot == LF && posC.x <= posB.x) ||
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
	vec2i pos = style->padColPositions[n->col];
	vec2i posL = curFeetPos[LF], posR = curFeetPos[RF];
	if(lastUsedFoot == LF)
	{
		if(pos == posL)
		{
			planFootswitch(RF, pos);
		}
		else if(pos.x < posL.x)
		{
			planCrossover(RF, pos);
		}
		else
		{
			setFoot(n, RF, pos);
		}
	}
	else if(lastUsedFoot == RF)
	{
		if(pos == posR)
		{
			planFootswitch(LF, pos);
		}
		else if(pos.x > posR.x)
		{
			planCrossover(LF, pos);
		}
		else
		{
			setFoot(n, LF, pos);
		}
	}
	else if(pos == posL)
	{
		setFoot(n, LF, pos);
	}
	else if(pos == posR)
	{
		setFoot(n, RF, pos);
	}
	else if(pos.x < posL.x)
	{
		setFoot(n, LF, pos);
	}
	else if(pos.x > posR.x)
	{
		setFoot(n, RF, pos);
	}
	else
	{
		if(nextNote[0].a && !nextNote[0].b)
		{
			vec2i nextPos = style->padColPositions[nextNote[0].a->col];
			if(nextPos.x < pos.x)
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
	vec2i posA = style->padColPositions[a->col];
	vec2i posB = style->padColPositions[b->col];
	if(posA.x < posB.x)
	{
		setFeet(a, b, posA, posB);
	}
	else if(posB.x < posA.x)
	{
		setFeet(b, a, posB, posA);
	}
	else if(posA == curFeetPos[LF] || posB == curFeetPos[RF])
	{
		setFeet(a, b, posA, posB);
	}
	else
	{
		setFeet(b, a, posB, posA);
	}
}

void FeetPlanner::plan(int pn)
{
	if(gNotes->begin() == gNotes->end()) return;

	noteBegin = gNotes->begin();

	lastUsedFoot = BOTH;
	curFeetPos[0] = style->padColPositions[style->padInitialFeetCols[pn].x];
	curFeetPos[1] = style->padColPositions[style->padInitialFeetCols[pn].y];
	skipNotes = 0;

	// Fill up the next note array.
	memset(nextNote, 0, sizeof(NotePair) * 4);
	auto n = noteBegin, end = gNotes->end();
	while(n != end && nextNote[0].a == nullptr)
	{
		// Move all upcomming notes one index ahead.
		nextNote[0] = nextNote[1];
		nextNote[1] = nextNote[2];
		nextNote[2] = nextNote[3];

		// Insert new notes in the last index.
		while(n != end && (n->isMine || n->player != pn)) ++n;
		if(n != end)
		{
			auto m = nextNote[3].a = n++;
			while(n != end && (n->isMine || n->player != pn)) n++;
			if(n != end && n->row == m->row) nextNote[3].b = n++;
			while(n != end && n->row == m->row) ++n;
		}
		else
		{
			nextNote[3] = {nullptr, nullptr};
		}
	}

	while(n != end || nextNote[0].a)
	{
		// Move all upcomming notes one index ahead.
		curNote = nextNote[0];
		nextNote[0] = nextNote[1];
		nextNote[1] = nextNote[2];
		nextNote[2] = nextNote[3];

		// Insert new notes in the last index.
		while(n != end && (n->isMine || n->player != pn)) ++n;
		if(n != end)
		{
			nextNote[3].b = nullptr;
			auto m = nextNote[3].a = n++;
			while(n != end && (n->isMine || n->player != pn)) n++;
			if(n != end && n->row == m->row) nextNote[3].b = n++;
			while(n != end && n->row == m->row) ++n;
		}
		else
		{
			nextNote[3] = {nullptr, nullptr};
		}

		// Skip notes that were already planned (as part of a crossover/footswitch).
		if(skipNotes > 0)
		{
			--skipNotes;
		}
		else if(curNote.b == nullptr) // Most common case, single note.
		{
			planStep();
		}
		else // Multiple notes on the same row, a jump.
		{
			planJump();
		}
	}
}

// ================================================================================================
// Dancing Bot Dialog.

DialogDancingBot::~DialogDancingBot()
{
}

DialogDancingBot::DialogDancingBot()
{
	setTitle("DANCING BOT");

	BatchSprite::init(myPadSpr, 6, 4, 2, 64, 64);
	BatchSprite::init(myFeetSpr, 2, 2, 1, 96, 96);
	myPadTex = Texture("assets/dancing bot pad.png");
	myFeetTex = Texture("assets/dancing bot feet.png", true);

	myDoFootswitches = true;
	myDoCrossovers = true;

	myCreateWidgets();

	onChanges(VCM_ALL_CHANGES);
}

void DialogDancingBot::myCreateWidgets()
{
	myLayout.row().col(104).col(100);

	WgCheckbox* footswitch = myLayout.add<WgCheckbox>();
	footswitch->text.set("Footswitch");
	footswitch->value.bind(&myDoFootswitches);
	footswitch->onChange.bind(this, &DialogDancingBot::onDoFootswitchesChanged);
	footswitch->setTooltip("If enabled, the dancing bot will attempt footswitches");

	WgCheckbox* crossover = myLayout.add<WgCheckbox>();
	crossover->text.set("Crossover");
	crossover->value.bind(&myDoCrossovers);
	crossover->onChange.bind(this, &DialogDancingBot::onDoCrossoversChanged);
	crossover->setTooltip("If enabled, the dancing bot will attempt crossovers");
}

void DialogDancingBot::onUpdateSize()
{
	int w = 200, h = 64;
	auto style = gStyle->get();
	if(style && style->padWidth > 0)
	{
		w = max(w, style->padWidth * 64 + 8);
		h = max(h, style->padHeight * 64 + 24);
	}
	setWidth(w);
	setHeight(h);
}

void DialogDancingBot::onDraw()
{
	EditorDialog::onDraw();

	Renderer::pushScissorRect(getInnerRect());
	auto style = gStyle->get();
	if(style && style->padWidth > 0)
	{
		double time = gView->getCursorTime();

		auto notes = gNotes->begin();

		auto prevNotes = gNotes->getNotesBeforeTime(time);

		Renderer::bindShader(Renderer::SH_TEXTURE);
		Renderer::bindTexture(myPadTex.handle());

		// Pad layout.
		auto batch = Renderer::batchTC();
		BatchSprite::setScale(256);
		for(int x = 0; x < style->padWidth; ++x)
		{
			for(int y = 0; y < style->padHeight; ++y)
			{
				int tile = myPadLayout[y * style->padWidth + x];
				vec2i pos = myGetDrawPos({x, y});
				myPadSpr[tile].draw(&batch, pos.x, pos.y, (uchar)255);
				if(tile == TT_BUTTON) myPushArrow(&batch, x, y);
			}
		}

		// Panel highlights.
		for(int col = 0; col < prevNotes.size(); ++col)
		{
			auto n = prevNotes[col];
			double dist = n ? (time - n->endtime) : 1000.0;
			int alpha = min(max((int)((1.5 - dist * 6.0) * 255.0), 0), 255);
			if(alpha > 0)
			{
				vec2i pos = myGetDrawPos(style->padColPositions[col]);
				myPadSpr[2].draw(&batch, pos.x, pos.y, (uchar)alpha);
			}
		}
		batch.flush();

		// Feet sprites.
		Renderer::bindTexture(myFeetTex.handle());
		batch = Renderer::batchTC();
		BatchSprite::setScale(160);
		for(int p = 0; p < gStyle->getNumPlayers(); ++p)
		{
			uint color = p ? ToColor32({.5f, .5f, 1, 1}) : ToColor32({1, .5f, .5f, 1});

			// Get current and target note for each foot.
			vec3f feetPos[2];
			myGetFeetPositions(feetPos, p);
			vec3f l(feetPos[0]), r(feetPos[1]);

			// Determine feet rotations.
			float rotation = 0.f;
			if(abs(l.x - r.x) + abs(l.y - r.y) > 0.1f)
			{
				rotation = atan2(r.y - l.y, r.x - l.x);
				rotation = min(max(rotation, -0.8f), 0.8f);
			}

			// Draw the feet.
			myFeetSpr[0].draw(&batch, l.x, l.y, rotation, l.z, color);
			myFeetSpr[1].draw(&batch, r.x, r.y, rotation, r.z, color);
		}
		batch.flush();
	}
	else // If there is no pad layout information.
	{
		Text::arrange(Text::MC, 12, "- nothing to dance -");
		Text::draw(getInnerRect());
	}
	Renderer::popScissorRect();
}

void DialogDancingBot::onChanges(int changes)
{
	if(changes & VCM_CHART_CHANGED)
	{
		auto style = gStyle->get();
		if(style == nullptr || style->padWidth == 0 || style->padHeight == 0)
		{
			myPadLayout.clear();
		}
		else
		{
			// Copy the layout of the buttons.
			myPadLayout.resize(style->padWidth * style->padHeight);
			for(int i = 0; i < style->padWidth * style->padHeight; ++i)
			{
				myPadLayout[i] = TT_EMPTY;
			}
			for(int col = 0; col < gStyle->getNumCols(); ++col)
			{
				vec2i pos = style->padColPositions[col];
				myPadLayout[pos.y * style->padWidth + pos.x] = TT_BUTTON;
			}
		}
	}
	if(changes & VCM_NOTES_CHANGED)
	{
		myAssignFeetToNotes();
	}
}

void DialogDancingBot::onDoFootswitchesChanged()
{
	myAssignFeetToNotes();
}

void DialogDancingBot::onDoCrossoversChanged()
{
	myAssignFeetToNotes();
}

void DialogDancingBot::myPushArrow(QuadBatchTC* batch, int x, int y)
{
	static int spr[9]   = {0, 1, 0, 1, 2, 1, 0, 1, 0};
	static float rot[9] = {2, 2, 3, 1, 0, 3, 1, 0, 0};
	int i = (y % 3) * 3 + (x % 3);
	vec2f pos = ToVec2f(myGetDrawPos({x, y}));
	myPadSpr[spr[i] + 3].draw(batch, pos.x, pos.y, rot[i] * (float)(M_PI / 2.0), 1.f);
}

vec2i DialogDancingBot::myGetDrawPos(vec2i colRow)
{
	recti r = getInnerRect();
	return{r.x + 36 + colRow.x * 64, r.y + 52 + colRow.y * 64};
}

void DialogDancingBot::myAssignFeetToNotes()
{
	myFeetBits.clear();
	int numNotes = gNotes->end() - gNotes->begin();
	auto style = gStyle->get();
	if(numNotes > 0 && style && style->padWidth > 0)
	{
		myFeetBits.resize(numNotes / 32 + 1);
		memset(myFeetBits.data(), 0, sizeof(uint) * myFeetBits.size());
		for(int pn = 0; pn != gStyle->getNumPlayers(); ++pn)
		{
			FeetPlanner planner;
			planner.style = style;
			planner.outBits = myFeetBits.data();
			planner.crossover = myDoCrossovers;
			planner.footswitch = myDoFootswitches;
			planner.plan(pn);
		}
	}
}

void DialogDancingBot::myGetFeetPositions(vec3f* out, int pn)
{
	auto style = gStyle->get();

	int prevNote[2] = {-1, -1};
	int nextNote[2] = {-1, -1};

	double time = gView->getCursorTime();
	auto notes = gNotes->begin();
	int noteCount = gNotes->end() - gNotes->begin();
	auto feetbits = myFeetBits.data();

	// Find the previous note for each foot.
	for(int n = 0; n != noteCount && notes[n].time < time; ++n)
	{
		int foot = (feetbits[n >> 5] >> (n & 31)) & 1;
		if(notes[n].isMine || notes[n].player != pn) continue;
		prevNote[foot] = n;
	}

	// Find the next note for each foot.
	for(int n = noteCount - 1; n != -1 && notes[n].time > time; --n)
	{
		int foot = (feetbits[n >> 5] >> (n & 31)) & 1;
		if(notes[n].isMine || notes[n].player != pn) continue;
		nextNote[foot] = n;
	}

	// Determine interpolated feet positions.
	vec2i initialPos[2] =
	{
		style->padColPositions[style->padInitialFeetCols[pn].x],
		style->padColPositions[style->padInitialFeetCols[pn].y]
	};
	for(int f = 0; f < 2; ++f)
	{
		// Position of current button.
		double curTime;
		vec2i curButton;
		if(prevNote[f] >= 0)
		{
			curButton = style->padColPositions[notes[prevNote[f]].col];
			curTime = notes[prevNote[f]].time;
		}
		else
		{
			curButton = initialPos[f];
			curTime = 0.0;
		}

		// Position of next button.
		double endtime = curTime;
		vec2i endButton = curButton;
		if(nextNote[f] >= 0)
		{
			endButton = style->padColPositions[notes[nextNote[f]].col];
			endtime = notes[nextNote[f]].time;
		}

		// Interpolated feet position.
		vec2f curPos = ToVec2f(myGetDrawPos(curButton));
		vec2f endPos = ToVec2f(myGetDrawPos(endButton));
		if(endtime > curTime)
		{
			double startTime = max(curTime, endtime - 0.5);
			double delta = LerpDelta(startTime, endtime, time);
			curPos = SmoothStep(curPos, endPos, (float)min(max(delta, 0.0), 1.0));
		}

		// Determine feet scale.
		double dt = min(fabs(curTime - time), fabs(endtime - time));
		float scale = (float)min(1.0, 0.8 + dt * 6.0);

		out[f] = {curPos.x, curPos.y, scale};
	}
}

}; // namespace Vortex
