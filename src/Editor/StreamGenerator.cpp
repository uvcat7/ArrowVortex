#include <Editor/StreamGenerator.h>
#include <Editor/View.h>

#include <System/System.h>
#include <Editor/Editor.h>

#include <Core/String.h>
#include <Core/Utils.h>

#include <Managers/MetadataMan.h>
#include <Managers/ChartMan.h>
#include <Managers/StyleMan.h>
#include <Managers/NoteMan.h>

#include <math.h>
#include <random>
#include <vector>

namespace Vortex {
namespace {

static const int HISTORY_SIZE = 16;
static const float MAX_STEP_DIST = 2.2f;
static const float MAX_SPREAD_DIST = 3.1f;

enum Foot
{
	FOOT_L = 0,
	FOOT_R = 1
};

static float GetPadDist(vec2i* pad, int colA, int colB)
{
	float dx = (float)(pad[colA].x - pad[colB].x);
	float dy = (float)(pad[colA].y - pad[colB].y);
	return sqrt(dx * dx + dy * dy);
}

struct StreamPlanner;

struct FootPlanner
{
	FootPlanner();

	float getDist(int colA, int colB);
	int getNextCol(int xmin, int xmax);

	vec2i* pad;
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
	StreamPlanner(const StreamGenerator* sg);

	float random();
	int getNextCol();

	std::random_device random_device;
	std::mt19937 random_generator;
	std::uniform_real_distribution<float> random_distribution;

	FootPlanner feet[2];

	std::vector<float> weights;
	std::vector<float> stepDists;
	std::vector<vec2i> pad;
	std::vector<int> histograms[2];
	std::vector<int> history[2];

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
	float minStepDist = MAX_STEP_DIST, maxStepDist = 0.0f;
	for(int col = 0; col < numCols; ++col)
	{
		weights[col] = 0.0f;
		stepDists[col] = 0.0f;

		// Make sure the target column is a valid option.
		if(col == otherFoot->curCol || pad[col].x < xmin || pad[col].x > xmax) continue;

		// Skip options that are beyond the max step and spread distance.
		float stepDist = GetPadDist(pad, curCol, col);
		if(stepDist > MAX_STEP_DIST) continue;

		float spreadDist = GetPadDist(pad, otherFoot->curCol, col);
		if(spreadDist > MAX_SPREAD_DIST) continue;

		weights[col] = 1.0f;
		stepDists[col] = stepDist;
	}

	// Avoid repeating the current column if the maximum repetition count is reached.
	if(repetitions >= owner->maxColReps)
	{
		weights[curCol] = 0;
	}
	else if(repetitions >= owner->maxBoxReps && otherFoot->repetitions > owner->maxBoxReps)
	{
		weights[curCol] = 0;
	}

	// Adjust the weights based on the target average step distance.
	float scaledTarget = max(0.1f, owner->targetStepDistance - 0.6f);
	float scaledAvg = max(0.1f, avgStepDist - 0.6f);
	scaledTarget = scaledTarget + (scaledTarget - scaledAvg) * 0.5f;
	for(int col = 0; col < numCols; ++col)
	{
		if(weights[col] > 0.0f)
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
		for(int col = 0; col < numCols; ++col)
		{
			weightSum += weights[col];
		}
		float randomWeight = owner->random() * weightSum;
		float currentWeight = 0.0f;
		for(int col = 0; col < numCols; ++col)
		{
			if(weights[col] > 0 && randomWeight > currentWeight)
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

StreamPlanner::StreamPlanner(const StreamGenerator* sg)
	: random_generator(random_device()), random_distribution(0.0f, 1.0f)
{
	auto style = gStyle->get();

	numCols = gStyle->getNumCols();
	nextFoot = sg->startWithRight ? FOOT_R : FOOT_L;
	maxColReps = clamp(sg->maxColRep, 1, 16);
	maxBoxReps = clamp(sg->maxBoxRep, 1, 16);

	feet[FOOT_L].curCol = sg->feetCols.x;
	feet[FOOT_R].curCol = sg->feetCols.y;

	pad.resize(numCols);
	weights.resize(numCols);
	stepDists.resize(numCols);

	if(style == nullptr)
	{
		for(int col = 0; col < numCols; ++col)
		{
			pad[col] = {0, 0};
		}
	}
	else
	{
		for(int col = 0; col < numCols; ++col)
		{
			pad[col] = style->padColPositions[col];
		}
	}

	// Determine the average step distance to aim for.
	float maxStepDist = 0.0f;
	for(int i = 0; i < numCols; ++i)
	{
		for(int j = i + 1; j < numCols; ++j)
		{
			float dist = GetPadDist(pad.data(), i, j);
			maxStepDist = max(maxStepDist, dist);
		}
	}
	maxStepDist = min(MAX_STEP_DIST, maxStepDist);
	targetStepDistance = lerp(0.2f, maxStepDist, sg->patternDifficulty);
	targetStepDistanceBias = abs(sg->patternDifficulty * 2 - 1);

	// Initialize the individual feet planners.
	for(int f = 0; f < 2; ++f)
	{
		auto& foot = feet[f];

		histograms[f].resize(numCols, 0);
		history[f].resize(HISTORY_SIZE, -1);

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
	return random_distribution(random_generator);
}

int StreamPlanner::getNextCol()
{
	int out = 0;

	if(nextFoot == FOOT_L)
		out = feet[FOOT_L].getNextCol(0, pad[feet[FOOT_R].curCol].x);
	else
		out = feet[FOOT_R].getNextCol(pad[feet[FOOT_L].curCol].x, INT_MAX);

	nextFoot = 1 - nextFoot;

	return out;
}

}; // anonymous namespace.

StreamGenerator::StreamGenerator()
{
	feetCols = {0, 1};
	allowBoxes = true;
	startWithRight = true;
	patternDifficulty = 0.5f;
	maxColRep = 3;
	maxBoxRep = 2;
}

void StreamGenerator::generate(int row, int endRow, SnapType spacing)
{
	if(gChart->isClosed() || spacing < ST_4TH || spacing > ST_192TH) return;

	StreamPlanner stream(this);

	NoteEdit edit;

	int facingLeft = 0;
	int facingRight = 0;
	int rowDelta = sRowSnapTypes[spacing];
	int rowDeltaIndex = 0;
	while(row < endRow)
	{
		// Generate an arrow for the current row.
		int col = stream.getNextCol();
		edit.add.append({row, row, (uint)col, 0, NOTE_STEP_OR_HOLD, 192});

		// Store the current facing.
		int yl = stream.pad[stream.feet[FOOT_L].curCol].y;
		int yr = stream.pad[stream.feet[FOOT_R].curCol].y;
		facingLeft += yl < yr;
		facingRight += yr < yl;

		row += rowDelta;
	}

	// Some debug statistics.
	/*
	Note* prev = nullptr;
	map<int, int> colcount;
	for(auto& n : notes)
		colcount[n.col]++;
	for(auto& it : colcount)
		HudInfo("Col %i : %i", it.first, it.second);
	HudInfo("Facing left: %i", facingLeft);
	HudInfo("Facing right: %i", facingRight);
	*/
	
	gNotes->modify(edit, true, nullptr);
}

}; // namespace Vortex
