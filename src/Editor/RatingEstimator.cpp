#include <Editor/RatingEstimator.h>

#include <System/File.h>

#include <Managers/ChartMan.h>
#include <Managers/TempoMan.h>
#include <Managers/NoteMan.h>

#include <Simfile/TimingData.h>

#include <algorithm>
#include <vector>

namespace Vortex {

enum Constants
{
	HM_NUM_SLICES = 10,
	HM_VALS_PER_SLICE = 20,
	HM_NUM_WEIGHTS = HM_VALS_PER_SLICE * HM_NUM_SLICES,
};

RatingEstimator::RatingEstimator(const char* databaseFile)
{
	bool success;
	std::vector<double> values;
	for(auto& line : File::getLines(databaseFile, &success))
	{
		if(line.len() && line[0] == '#') continue;
		values.push_back(atof(line.begin()));
	}
	if(values.size() != HM_NUM_WEIGHTS) return;

	myWeights = new double[HM_NUM_WEIGHTS];
	for(int i = 0; i < HM_NUM_WEIGHTS; ++i) myWeights[i] = values[i];
}

RatingEstimator::~RatingEstimator()
{
	delete[] myWeights;
}

static std::vector<double> CalcDensities()
{
	std::vector<double> stamps, out;
	TempoTimeTracker tracker(gTempo->getTimingData());
	for(auto& n : *gNotes)
	{
		if(!(n.isMine | n.isWarped))
		{
			stamps.push_back(tracker.advance(n.row));
		}
	}

	// List densities per second of song.
	std::vector<double> densities;
	if(stamps.size())
	{
		double timeWindow = 1.0;
		double curTime = stamps[0];
		double* curStamp = stamps.data();
		for(auto& s : stamps)
		{
			if(s > *curStamp + timeWindow)
			{
				int numArrows = &s - curStamp;
				double dt = s - *curStamp;
				densities.push_back((double)numArrows / dt);
				curStamp = &s;
			}
		}
		std::sort(densities.begin(), densities.end());
	}

	// Estimation parameters.
	int sliceSize[HM_NUM_SLICES] = {2, 4, 7, 13, 22, 38, 65, 90, 150, 240};
	int totalSize = (int)densities.size();
	for(int i = 0; i < HM_NUM_SLICES; ++i)
	{
		if(totalSize >= sliceSize[i])
		{
			double n = 0;
			for(int j = totalSize - sliceSize[i]; j != totalSize; ++j)
			{
				n += densities[j];
			}
			out.push_back(n / sliceSize[i]);
		}
		else
		{
			out.push_back(0.0);
		}
	}

	return out;
}

inline int getDensityBin(double density)
{
	int bin = (int)density;
	return std::min(std::max(0, bin), HM_VALS_PER_SLICE - 2);
}

double RatingEstimator::estimateRating()
{
	std::vector<double> densities = CalcDensities();

	double maxRating = 1.0;
	for(int i = 0; i < HM_NUM_SLICES; ++i)
	{
		double density = densities[i], rating;
		if(density > 0.0)
		{
			int bin = getDensityBin(density);
			double frac = std::min(std::max(0.0, density - (double)bin), 1.0);
			const double* sliceWeights = myWeights + i * HM_VALS_PER_SLICE;
			double diffA = sliceWeights[bin];
			double diffB = sliceWeights[bin + 1];
			rating = diffA + (diffB - diffA) * frac;
			maxRating = std::max(maxRating, rating);
		}
	}

	return maxRating;
}

}; // namespace Vortex
