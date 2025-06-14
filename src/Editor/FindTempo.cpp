#include <Core/Polyfit.h>
#include <Core/Core.h>
#include <Core/AlignedMemory.h>

#include <System/Thread.h>

#include <Simfile/Common.h>

#include <Editor/FindTempo.h>
#include <Editor/FindOnsets.h>
#include <Editor/Music.h>

#include <algorithm>
#include <functional>
#include <atomic>
#include <vector>

#define MarkProgress(number, text) { if(*data->terminate) {return;} data->progress = number; }

typedef double real;

namespace Vortex {
namespace {

static const real MinimumBPM = 89.0;
static const real MaximumBPM = 205.0;
static const int IntervalDelta = 10;
static const int IntervalDownsample = 3;
static const int MaxThreads = 8;

// ================================================================================================
// Helper structs.

typedef std::vector<TempoResult> TempoResults;

struct TempoSort {
	bool operator()(const TempoResult& a, const TempoResult& b) {
		return a.fitness > b.fitness;
	}
};

struct SerializedTempo
{
	float* samples;
	int samplerate;
	int numFrames;
	int numThreads;
	uchar* terminate;
	std::atomic_int progress;
	TempoResults result;
};

struct GapData
{
	GapData(int numThreads, int maxInterval, int downsample, int numOnsets, const Onset* onsets);
	~GapData();

	const Onset* onsets;
	int* wrappedPos;
	real* wrappedOnsets;
	real* window;
	int bufferSize, numOnsets, windowSize, downsample;
};

struct IntervalTester
{
	IntervalTester(int samplerate, int numOnsets, const Onset* onsets);
	~IntervalTester();

	int minInterval;
	int maxInterval;
	int numIntervals;
	int samplerate;
	int gapWindowSize;
	int numOnsets;
	const Onset* onsets;
	real* fitness;
	real coefs[4];
};

// ================================================================================================
// Audio processing

// Creates weights for a hamming window of length n.
static void CreateHammingWindow(real* out, int n)
{
	const real t = 6.2831853071795864 / (real)(n - 1);
	for(int i = 0; i < n; ++i) out[i] = 0.54 - 0.46 * cos((real)i * t);
}

// Normalizes the given fitness value based the given 3rd order poly coefficients and interval.
static void NormalizeFitness(real& fitness, const real* coefs, real interval)
{
	real x = interval, x2 = x * x, x3 = x2 * x;
	fitness -= coefs[0] + coefs[1] * x + coefs[2] * x2 + coefs[3] * x3;
}

// ================================================================================================
// Gap confidence evaluation

GapData::GapData(int numThreads, int bufferSize, int downsample, int numOnsets, const Onset* onsets)
	: numOnsets(numOnsets)
	, onsets(onsets)
	, downsample(downsample)
	, windowSize(2048 >> downsample)
	, bufferSize(bufferSize)
{
	window = AlignedMalloc<real>(windowSize);
	wrappedPos = AlignedMalloc<int>(numOnsets * numThreads);
	wrappedOnsets = AlignedMalloc<real>(bufferSize * numThreads);
	CreateHammingWindow(window, windowSize);
}

GapData::~GapData()
{
	AlignedFree(window);
	AlignedFree(wrappedPos);
	AlignedFree(wrappedOnsets);
}

// Returns the confidence value that indicates how many onsets are close to the given gap position.
static real GapConfidence(const GapData& gapdata, int threadId, int gapPos, int interval)
{
	int numOnsets = gapdata.numOnsets;
	int windowSize = gapdata.windowSize;
	int halfWindowSize = windowSize / 2;
	const real* window = gapdata.window;
	const real* wrappedOnsets = gapdata.wrappedOnsets + gapdata.bufferSize * threadId;
	real area = 0.0;

	int beginOnset = gapPos - halfWindowSize;
	int endOnset = gapPos + halfWindowSize;

	if(beginOnset < 0)
	{
		int wrappedBegin = beginOnset + interval;
		for(int i = wrappedBegin; i < interval; ++i)
		{
			int windowIndex = i - wrappedBegin;
			area += wrappedOnsets[i] * window[windowIndex];
		}
		beginOnset = 0;
	}
	if(endOnset > interval)
	{
		int wrappedEnd = endOnset - interval;
		int indexOffset = windowSize - wrappedEnd;
		for(int i = 0; i < wrappedEnd; ++i)
		{
			int windowIndex = i + indexOffset;
			area += wrappedOnsets[i] * window[windowIndex];
		}
		endOnset = interval;
	}
	for(int i = beginOnset; i < endOnset; ++i)
	{
		int windowIndex = i - beginOnset;
		area += wrappedOnsets[i] * window[windowIndex];
	}

	return area;
}

// Returns the confidence of the best gap value for the given interval.
static real GetConfidenceForInterval(const GapData& gapdata, int threadId, int interval)
{
	int downsample = gapdata.downsample;
	int numOnsets = gapdata.numOnsets;
	const Onset* onsets = gapdata.onsets;

	int* wrappedPos = gapdata.wrappedPos + gapdata.numOnsets * threadId;
	real* wrappedOnsets = gapdata.wrappedOnsets + gapdata.bufferSize * threadId;
	memset(wrappedOnsets, 0, sizeof(real) * gapdata.bufferSize);

	// Make a histogram of onset strengths for every position in the interval.
	int reducedInterval = interval >> downsample;
	for(int i = 0; i < numOnsets; ++i)
	{
		int pos = (onsets[i].pos % interval) >> downsample;
		wrappedPos[i] = pos;
		wrappedOnsets[pos] += onsets[i].strength;
	}

	// Record the amount of support for each gap value.
	real highestConfidence = 0.0;
	for(int i = 0; i < numOnsets; ++i)
	{
		int pos = wrappedPos[i];
		real confidence = GapConfidence(gapdata, threadId, pos, reducedInterval);
		int offbeatPos = (pos + reducedInterval / 2) % reducedInterval;
		confidence += GapConfidence(gapdata, threadId, offbeatPos, reducedInterval) * 0.5;

		if(confidence > highestConfidence)
		{
			highestConfidence = confidence;
		}
	}

	return highestConfidence;
}

// Returns the confidence of the best gap value for the given BPM value.
static real GetConfidenceForBPM(const GapData& gapdata, int threadId, IntervalTester& test, real bpm)
{
	int numOnsets = gapdata.numOnsets;
	const Onset* onsets = gapdata.onsets;

	int* wrappedPos = gapdata.wrappedPos + gapdata.numOnsets * threadId;
	real* wrappedOnsets = gapdata.wrappedOnsets + gapdata.bufferSize * threadId;
	memset(wrappedOnsets, 0, sizeof(real) * gapdata.bufferSize);

	// Make a histogram of i strengths for every position in the interval.
	real intervalf = test.samplerate * 60.0 / bpm;
	int interval = (int)(intervalf + 0.5);
	for(int i = 0; i < numOnsets; ++i)
	{
		int pos = (int)fmod((real)onsets[i].pos, intervalf);
		wrappedPos[i] = pos;
		wrappedOnsets[pos] += onsets[i].strength;
	}

	// Record the amount of support for each gap value.
	real highestConfidence = 0.0;
	for(int i = 0; i < numOnsets; ++i)
	{
		int pos = wrappedPos[i];
		real confidence = GapConfidence(gapdata, threadId, pos, interval);
		int offbeatPos = (pos + interval / 2) % interval;
		confidence += GapConfidence(gapdata, threadId, offbeatPos, interval) * 0.5;

		if(confidence > highestConfidence)
		{
			highestConfidence = confidence;
		}
	}

	// Normalize the confidence value.
	NormalizeFitness(highestConfidence, test.coefs, intervalf);

	return highestConfidence;
}

// ================================================================================================
// Interval testing

IntervalTester::IntervalTester(int samplerate, int numOnsets, const Onset* onsets)
	: samplerate(samplerate)
	, numOnsets(numOnsets)
	, onsets(onsets)
{
	minInterval = (int)(samplerate * 60.0 / MaximumBPM + 0.5);
	maxInterval = (int)(samplerate * 60.0 / MinimumBPM + 0.5);
	numIntervals = maxInterval - minInterval;

	fitness = AlignedMalloc<real>(numIntervals);
}

IntervalTester::~IntervalTester()
{
	AlignedFree(fitness);
}

static real IntervalToBPM(const IntervalTester& test, int i)
{
	return (test.samplerate * 60.0) / (i + test.minInterval);
}

static void FillCoarseIntervals(IntervalTester& test, GapData& gapdata, int numThreads)
{
	int numCoarseIntervals = (test.numIntervals + IntervalDelta - 1) / IntervalDelta;
	if(numThreads > 1)
	{
		struct IntervalThreads : public ParallelThreads
		{
			IntervalTester* test;
			GapData* gapdata;
			void exec(int i, int thread)
			{
				int index = i * IntervalDelta;
				int interval = test->minInterval + index;
				test->fitness[index] = std::max(0.001, GetConfidenceForInterval(*gapdata, thread, interval));
			}
		};
		IntervalThreads threads;
		threads.test = &test;
		threads.gapdata = &gapdata;
		threads.run(numCoarseIntervals, numThreads);
	}
	else
	{
		for(int i = 0; i < numCoarseIntervals; ++i)
		{
			int index = i * IntervalDelta;
			int interval = test.minInterval + index;
			test.fitness[index] = std::max(0.001, GetConfidenceForInterval(gapdata, 0, interval));
		}
	}
}

static vec2i FillIntervalRange(IntervalTester& test, GapData& gapdata, int begin, int end)
{
	begin = std::max(begin, 0);
	end = std::min(end, test.numIntervals);
	real* fit = test.fitness + begin;
	for(int i = begin, interval = test.minInterval + begin; i < end; ++i, ++interval, ++fit)
	{
		if(*fit == 0)
		{
			*fit = GetConfidenceForInterval(gapdata, 0, interval);
			NormalizeFitness(*fit, test.coefs, (real)interval);
			*fit = std::max(*fit, 0.1);
		}
	}
	return{begin, end};
}

static int FindBestInterval(const real* fitness, int begin, int end)
{
	int bestInterval = 0;
	real highestFitness = 0.0;
	for(int i = begin; i < end; ++i)
	{
		if(fitness[i] > highestFitness)
		{
			highestFitness = fitness[i];
			bestInterval = i;
		}
	}
	return bestInterval;
}

// ================================================================================================
// BPM testing

// Removes BPM values that are near-duplicates or multiples of a better BPM value.
static void RemoveDuplicates(TempoResults& tempo)
{
	for(int i = 0; i < tempo.size(); ++i)
	{
		real bpm = tempo[i].bpm, doubled = bpm * 2.0, halved = bpm * 0.5;
		for(int j = tempo.size() - 1; j > i; --j)
		{
			real v = tempo[j].bpm;
			if(std::min(std::min(abs(v - bpm), abs(v - doubled)), abs(v - halved)) < 0.1)
			{
				tempo.erase(tempo.begin() + j);
			}
		}
	}
}

// Rounds BPM values that are close to integer values.
static void RoundBPMValues(IntervalTester& test, GapData& gapdata, TempoResults& tempo)
{
	for(auto& t : tempo)
	{
		real roundBPM = round(t.bpm);
		real diff = abs(t.bpm - roundBPM);
		if(diff < 0.01)
		{
			t.bpm = roundBPM;
		}
		else if(diff < 0.05)
		{
			real old = GetConfidenceForBPM(gapdata, 0, test, t.bpm);
			real cur = GetConfidenceForBPM(gapdata, 0, test, roundBPM);
			if(cur > old * 0.99) t.bpm = roundBPM;
		}
	}
}

// Finds likely BPM candidates based on the given note onset values.
static void CalculateBPM(SerializedTempo* data, Onset* onsets, int numOnsets)
{
	auto& tempo = data->result;

	// In order to determine the BPM, we need at least two onsets.
	if(numOnsets < 2)
	{
		tempo.push_back({SIM_DEFAULT_BPM, 0.0, 1.0});
		return;
	}

	IntervalTester test(data->samplerate, numOnsets, onsets);
	GapData* gapdata = new GapData(data->numThreads, test.maxInterval, IntervalDownsample, numOnsets, onsets);

	// Loop through every 10th possible BPM, later we will fill in those that look interesting.
	memset(test.fitness, 0, test.numIntervals * sizeof(real));
	FillCoarseIntervals(test, *gapdata, data->numThreads);
	int numCoarseIntervals = (test.numIntervals + IntervalDelta - 1) / IntervalDelta;
	MarkProgress(2, "Fill course intervals");

	// Determine the polynomial coefficients to approximate the fitness curve and normalize the current fitness values.
	mathalgo::polyfit(3, test.coefs, test.fitness, numCoarseIntervals, test.minInterval);
	real maxFitness = 0.001;
	for(int i = 0; i < test.numIntervals; i += IntervalDelta)
	{
		NormalizeFitness(test.fitness[i], test.coefs, (real)(test.minInterval + i));
		maxFitness = std::max(maxFitness, test.fitness[i]);
	}

	// Refine the intervals around the best intervals.
	real fitnessThreshold = maxFitness * 0.4;
	for(int i = 0; i < test.numIntervals; i += IntervalDelta)
	{
		if(test.fitness[i] > fitnessThreshold)
		{
			vec2i range = FillIntervalRange(test, *gapdata, i - IntervalDelta, i + IntervalDelta);
			int best = FindBestInterval(test.fitness, range.x, range.y);
			tempo.push_back({IntervalToBPM(test, best), 0.0, test.fitness[best]});
		}
	}
	MarkProgress(3, "Refine intervals");

	// At this point we stop the downsampling and upgrade to a more precise gap window.
	delete gapdata;
	gapdata = new GapData(data->numThreads, test.maxInterval, 0, numOnsets, onsets);

	// Round BPM values to integers when possible, and remove weaker duplicates.
	std::stable_sort(tempo.begin(), tempo.end(), TempoSort());
	RemoveDuplicates(tempo);
	RoundBPMValues(test, *gapdata, tempo);

	// If the fitness of the first and second option is very close, we ask for a second opinion.
	if(tempo.size() >= 2 && tempo[0].fitness / tempo[1].fitness < 1.05)
	{
		for(auto& t : tempo)
			t.fitness = GetConfidenceForBPM(*gapdata, 0, test, t.bpm);
		std::stable_sort(tempo.begin(), tempo.end(), TempoSort());
	}

	// In all 300 test cases the correct BPM value was part of the top 3 choices,
	// so it seems reasonable to discard anything below the top 3 as irrelevant.
	if(tempo.size() > 3) tempo.resize(3);

	// Cleanup.
	delete gapdata;
}

// ================================================================================================
// Offset testing

static void ComputeSlopes(const float* samples, real* out, int numFrames, int samplerate)
{
	memset(out, 0, sizeof(real) * numFrames);

	int wh = samplerate / 20;
	if(numFrames < wh * 2) return;

	// Initial sums of the left/right side of the window.
	real sumL = 0, sumR = 0;
	for(int i = 0, j = wh; i < wh; ++i, ++j)
	{
		sumL += abs(samples[i]);
		sumR += abs(samples[j]);
	}

	// Slide window over the samples.
	real scalar = 1.0 / (real)wh;
	for(int i = wh, end = numFrames - wh; i < end; ++i)
	{
		// Determine slope value.
		out[i] = std::max(0.0, (real)(sumR - sumL) * scalar);

		// Move window.
		real cur = abs(samples[i]);
		sumL -= abs(samples[i - wh]);
		sumL += cur;
		sumR -= cur;
		sumR += abs(samples[i + wh]);
	}
}

// Returns the most promising offset for the given BPM value.
static real GetBaseOffsetValue(const GapData& gapdata, int samplerate, real bpm)
{
	int numOnsets = gapdata.numOnsets;
	const Onset* onsets = gapdata.onsets;

	int* wrappedPos = gapdata.wrappedPos;
	real* wrappedOnsets = gapdata.wrappedOnsets;
	memset(wrappedOnsets, 0, sizeof(real) * gapdata.bufferSize);

	// Make a histogram of onset strengths for every position in the interval.
	real intervalf = samplerate * 60.0 / bpm;
	int interval = (int)(intervalf + 0.5);
	memset(wrappedOnsets, 0, sizeof(real) * interval);
	for(int i = 0; i < numOnsets; ++i)
	{
		int pos = (int)fmod((real)onsets[i].pos, intervalf);
		wrappedPos[i] = pos;
		wrappedOnsets[pos] += 1.0;
	}

	// Record the amount of support for each gap value.
	real highestConfidence = 0.0;
	int offsetPos = 0;
	for(int i = 0; i < numOnsets; ++i)
	{
		int pos = wrappedPos[i];
		real confidence = GapConfidence(gapdata, 0, pos, interval);
		int offbeatPos = (pos + interval / 2) % interval;
		confidence += GapConfidence(gapdata, 0, offbeatPos, interval) * 0.5;

		if(confidence > highestConfidence)
		{
			highestConfidence = confidence;
			offsetPos = pos;
		}
	}

	return (real)offsetPos / (real)samplerate;
}

// Compares each offset to its corresponding offbeat value, and selects the most promising one.
static real AdjustForOffbeats(SerializedTempo* data, real offset, real bpm)
{
	int samplerate = data->samplerate;
	int numFrames = data->numFrames;

	// Create a slope representation of the waveform.
	real* slopes = AlignedMalloc<real>(numFrames);
	ComputeSlopes(data->samples, slopes, numFrames, samplerate);

	// Determine the offbeat sample position.
	real secondsPerBeat = 60.0 / bpm;
	real offbeat = offset + secondsPerBeat * 0.5;
	if(offbeat > secondsPerBeat) offbeat -= secondsPerBeat;

	// Calculate the support for both sample positions.
	real end = (real)numFrames;
	real interval = secondsPerBeat * samplerate;
	real posA = offset * samplerate, sumA = 0.0;
	real posB = offbeat * samplerate, sumB = 0.0;
	for(; posA < end && posB < end; posA += interval, posB += interval)
	{
		sumA += slopes[(int)posA];
		sumB += slopes[(int)posB];
	}
	AlignedFree(slopes);

	// Return the offset with the highest support.
	return (sumA >= sumB) ? offset : offbeat;
}

// Selects the best offset value for each of the BPM candidates.
static void CalculateOffset(SerializedTempo* data, Onset* onsets, int numOnsets)
{
	auto& tempo = data->result;
	int samplerate = data->samplerate;

	// Create gapdata buffers for testing.
	real maxInterval = 0.0;
	for(auto& t : tempo) maxInterval = std::max(maxInterval, samplerate * 60.0 / t.bpm);
	GapData gapdata(1, (int)(maxInterval + 1.0), 1, numOnsets, onsets);

	// Fill in onset values for each BPM.
	for(auto& t : tempo)
		t.offset = GetBaseOffsetValue(gapdata, samplerate, t.bpm);

	// Test all onsets against their offbeat values, pick the best one.
	for(auto& t : tempo)
		t.offset = AdjustForOffbeats(data, t.offset, t.bpm);
}

// ================================================================================================
// BPM testing wrapper class

static const char* sProgressText[]
{
	"[1/6] Looking for onsets",
	"[2/6] Scanning intervals",
	"[3/6] Refining intervals",
	"[4/6] Selecting BPM values",
	"[5/6] Calculating offsets",
	"BPM detection results"
};

class TempoDetectorImp : public TempoDetector, public BackgroundThread
{
public:
	TempoDetectorImp(int firstFrame, int numFrames);
	~TempoDetectorImp();

	void exec();

	bool hasSamples() { return (data_.samples != nullptr); }
	const char* getProgress() const { return sProgressText[data_.progress]; }
	bool hasResult() const { return isDone(); }
	const std::vector<TempoResult>& getResult() const { return data_.result; }

private:
	SerializedTempo data_;
};

TempoDetectorImp::TempoDetectorImp(int firstFrame, int numFrames)
{
	auto& music = gMusic->getSamples();

	data_.terminate = &terminationFlag_;
	data_.progress = 0;
	
	data_.numThreads = ParallelThreads::concurrency();
	data_.numFrames = numFrames;
	data_.samplerate = music.getFrequency();

	data_.samples = AlignedMalloc<float>(numFrames);
	if(!data_.samples) return;

	// Copy the input samples.
	const short* l = music.samplesL() + firstFrame;
	const short* r = music.samplesR() + firstFrame;
	for(int i = 0; i < numFrames; ++i, ++l, ++r)
	{
		data_.samples[i] = (float)((int)*l + (int)*r) / 65536.0f;
	}

	start();
}

TempoDetectorImp::~TempoDetectorImp()
{
	terminate();

	AlignedFree(data_.samples);
}

void TempoDetectorImp::exec()
{
	SerializedTempo* data = &data_;

	// Run the aubio onset tracker to find note onsets.
	std::vector<Onset> onsets;
	FindOnsets(data->samples, data->samplerate, data->numFrames, 1, onsets);
	MarkProgress(1, "Find onsets");

	for(int i = 0; i < std::min(onsets.size(), size_t(100)); ++i)
	{
		int a = std::max(0, onsets[i].pos - 100);
		int b = std::min(data->numFrames, onsets[i].pos + 100);
		float v = 0.0f;
		for(int j = a; j < b; ++j)
		{
			v += abs(data->samples[j]);
		}
		v /= (float)std::max(1, b - a);
		onsets[i].strength = v;
	}

	// Find BPM values.
	CalculateBPM(data, onsets.data(), onsets.size());
	MarkProgress(4, "Find BPM");

	// Find offset values.
	CalculateOffset(data, onsets.data(), onsets.size());
	MarkProgress(5, "Find offsets");
}

}; // anonymous namespace

TempoDetector* TempoDetector::New(double time, double len)
{
	auto& music = gMusic->getSamples();

	// Check if the music is finished loading first.
	if(!music.isCompleted())
	{
		HudInfo("The music is still loading, wait a bit longer before using BPM detection.");
		return nullptr;
	}

	// Check if the number of frames is non-zero.
	int firstFrame = std::max(0, (int)(time * music.getFrequency()));
	int numFrames = std::max(0, (int)(len * music.getFrequency()));
	numFrames = std::min(numFrames, music.getNumFrames() - firstFrame);
	if(numFrames <= 0)
	{
		HudInfo("There is no audio selected to perform BPM detection on.");
		return nullptr;
	}

	// If so, we can detect the BPM.
	auto detector = new TempoDetectorImp(firstFrame, numFrames);
	if(!detector->hasSamples())
	{
		HudError("Insufficient memory to perform BPM detection.");
		delete detector;
		detector = nullptr;
	}

	return detector;
}

}; // namespace Vortex
