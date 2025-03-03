#include <Precomp.h>

#include <random>

#include <Core/Utils/Util.h>

#include <Core/System/System.h>

#include <Simfile/NoteSet.h>

#include <Vortex/Editor.h>
#include <Vortex/Application.h>

#include <Vortex/Edit/StreamGenerator.h>

#include <Vortex/View/View.h>

#include <Vortex/Managers/GameMan.h>

namespace AV {

using namespace std;
using namespace Util;

static constexpr int HistorySize = 16;
static constexpr float MaxStepDist = 2.2f;
static constexpr float MaxSpreadDist = 3.1f;

struct Foot
{
	static constexpr int L = 0;
	static constexpr int R = 1;
};

static const int RowSnapTypes[(int)SnapType::Count] =
{
	1, 48, 24, 16, 12, 10, 8, 6, 4, 3, 2, 1
};

static float GetPadDist(Vec2* pad, int colA, int colB)
{
	float dx = float(pad[colA].x - pad[colB].x);
	float dy = float(pad[colA].y - pad[colB].y);
	return sqrt(dx * dx + dy * dy);
}

struct StreamPlanner;

struct FootPlanner
{
	FootPlanner();

	float getDist(int colA, int colB);
	int getNextCol(int xmin, int xmax);

	Vec2* pad;
	float* weights;
	float* stepDists;
	StreamPlanner* owner;
	FootPlanner* otherFoot;

	int curCol, prevCol;
	int repetitions;
	float avgStepDist;
};

struct StreamPlanner
{
	StreamPlanner(Vec2 feetCols, const StreamGenerator* sg);

	float random();
	int getNextCol();

	mt19937 randMt;
	uniform_real<float> randDistr;
	FootPlanner feet[2];

	vector<float> weights;
	vector<float> stepDists;
	vector<Vec2> pad;
	vector<int> histograms[2];
	vector<int> history[2];

	int numCols;
	int nextFoot;
	int maxColReps;
	int maxBoxReps;
	bool disableTripleRepeat;
	float targetStepDistance;
	float targetStepDistanceBias;
};

FootPlanner::FootPlanner()
{
	curCol = 0;
	prevCol = -1;
	repetitions = 0;
}

int FootPlanner::getNextCol(int xmin, int xmax)
{
	int numCols = owner->numCols;

	// Determine the weights of every option.
	float minStepDist = MaxStepDist, maxStepDist = 0.0f;
	for (int col = 0; col < numCols; ++col)
	{
		weights[col] = 0.0f;
		stepDists[col] = 0.0f;

		// Make sure the target column is a valid option.
		if (col == otherFoot->curCol || pad[col].x < xmin || pad[col].x > xmax) continue;

		// Skip options that are beyond the max step and spread distance.
		float stepDist = GetPadDist(pad, curCol, col);
		if (stepDist > MaxStepDist) continue;

		float spreadDist = GetPadDist(pad, otherFoot->curCol, col);
		if (spreadDist > MaxSpreadDist) continue;

		weights[col] = 1.0f;
		stepDists[col] = stepDist;
	}

	// Avoid repeating the current column if the maximum repetition count is reached.
	if (repetitions >= owner->maxColReps)
	{
		weights[curCol] = 0;
	}
	else if (repetitions >= owner->maxBoxReps && otherFoot->repetitions > owner->maxBoxReps)
	{
		weights[curCol] = 0;
	}

	// Adjust the weights based on the target average step distance.
	float scaledTarget = max(0.1f, owner->targetStepDistance - 0.6f);
	float scaledAvg = max(0.1f, avgStepDist - 0.6f);
	scaledTarget = scaledTarget + (scaledTarget - scaledAvg) * 0.5f;
	for (int col = 0; col < numCols; ++col)
	{
		if (weights[col] > 0.0f)
		{
			float scaledDist = max(0.1f, stepDists[col] - 0.6f);
			float delta = abs(scaledDist - scaledTarget);
			float w = weights[col] / (0.1f + delta);
			weights[col] = lerp(weights[col], w, owner->targetStepDistanceBias);
		}
	}

	// Pick random option with propabilities equal to the weights.
	int nextCol = curCol;
	{
		float weightSum = 0.0f;
		for (int col = 0; col < numCols; ++col)
		{
			weightSum += weights[col];
		}
		float randomWeight = owner->random() * weightSum;
		float currentWeight = 0.0f;
		for (int col = 0; col < numCols; ++col)
		{
			if (weights[col] > 0 && randomWeight > currentWeight)
			{
				nextCol = col;
			}
			currentWeight += weights[col];
		}
	}

	repetitions = (curCol == nextCol) ? (repetitions + 1) : 1;
	prevCol = curCol;
	curCol = nextCol;

	// Update the average step distance.
	avgStepDist = avgStepDist * 0.65f + stepDists[nextCol] * 0.35f;

	return nextCol;
}

StreamPlanner::StreamPlanner(Vec2 feetCols, const StreamGenerator* sg)
{
	auto chart = Editor::currentChart();

	numCols = chart ? chart->gameMode->numCols : 0;
	nextFoot = sg->startWithRight ? Foot::R : Foot::L;
	maxColReps = clamp(sg->maxColRep, 1, 16);
	maxBoxReps = clamp(sg->maxBoxRep, 1, 16);

	feet[Foot::L].curCol = feetCols.x;
	feet[Foot::R].curCol = feetCols.y;

	randMt.seed((ulong)(System::getElapsedTime() * 1000.0));
	pad.resize(numCols);
	weights.resize(numCols);
	stepDists.resize(numCols);

	if (chart == nullptr)
	{
		for (int col = 0; col < numCols; ++col)
		{
			pad[col] = {0, 0};
		}
	}
	else
	{
		for (int col = 0; col < numCols; ++col)
		{
			pad[col] = chart->gameMode->padColPositions[col];
		}
	}

	// Determine the average step distance to aim for.
	float maxStepDist = 0.0f;
	for (int i = 0; i < numCols; ++i)
	{
		for (int j = i + 1; j < numCols; ++j)
		{
			float dist = GetPadDist(pad.data(), i, j);
			maxStepDist = max(maxStepDist, dist);
		}
	}
	maxStepDist = min(MaxStepDist, maxStepDist);
	targetStepDistance = lerp(0.2f, maxStepDist, sg->patternDifficulty);
	targetStepDistanceBias = abs(sg->patternDifficulty * 2 - 1);

	// Initialize the individual feet planners.
	for (int f = 0; f < 2; ++f)
	{
		auto& foot = feet[f];

		histograms[f] = vector<int>(numCols, 0);
		history[f] = vector<int>(HistorySize, -1);

		foot.avgStepDist = targetStepDistance;
		foot.weights = weights.data();
		foot.stepDists = stepDists.data();
		foot.pad = pad.data();

		foot.owner = this;
		foot.otherFoot = &feet[1 - f];
	}	
}

float StreamPlanner::random()
{
	return randDistr(randMt);
}

int StreamPlanner::getNextCol()
{
	int out = 0;

	if (nextFoot == Foot::L)
		out = feet[Foot::L].getNextCol(0, pad[feet[Foot::R].curCol].x);
	else
		out = feet[Foot::R].getNextCol(pad[feet[Foot::L].curCol].x, INT_MAX);

	nextFoot = 1 - nextFoot;

	return out;
}

StreamGenerator::StreamGenerator()
{
	allowBoxes = true;
	startWithRight = true;
	patternDifficulty = 0.5f;
	maxColRep = 3;
	maxBoxRep = 2;
}

void StreamGenerator::generate(Row row, Row endRow, Vec2 feetCols, SnapType spacing)
{
	auto chart = Editor::currentChart();

	/* TODO: reimplement, make better

	if (!chart || spacing < S4 || spacing > S192) return;

	StreamPlanner stream(this);

	ChartModification mod;
	mod.removePolicy = ChartModification::REMOVE_ENTIRE_REGION;

	int facingLeft = 0;
	int facingRight = 0;
	Row rowDelta = RowSnapTypes[spacing];
	Row rowDeltaIndex = 0;
	while (row < endRow)
	{
		// Generate an arrow for the current row.
		int col = stream.getNextCol();
		mod.notesToAdd.append({row, row, 0, (uint)col, 0, NoteType::Step});

		// Store the current facing.
		int yl = stream.pad[stream.feet[FOOT_L].curCol].y;
		int yr = stream.pad[stream.feet[FOOT_R].curCol].y;
		facingLeft += yl < yr;
		facingRight += yr < yl;

		// Advance to the next row position.
		if (rowDelta == 10)
		{
			static const int deltas[5] = {10, 9, 10, 9, 10};
			row += deltas[rowDeltaIndex];
			if (++rowDeltaIndex == 5) rowDeltaIndex = 0;
		}
		else
		{
			row += rowDelta;
		}
	}

	// Some debug statistics.
	Note* prev = nullptr;
	map<int, int> colcount;
	for (auto& n : notes)
		colcount[n.col]++;
	for (auto& it : colcount)
		HudInfo("Col %i : %i", it.first, it.second);
	HudInfo("Facing left: %i", facingLeft);
	HudInfo("Facing right: %i", facingRight);
	
	Editor::currentChart()->notes->modify(mod);

	*/
}

} // namespace AV
