#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/File.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo.h>

#include <Vortex/VortexUtils.h>
#include <Vortex/Editor.h>

namespace AV {

using namespace std;
using namespace Util;

TimingMode GetTimingMode(Chart* chart)
{
	if (chart)
	{
		bool splitTempo = false;
		for (auto chart : chart->simfile->charts)
		{
			if (chart->tempo)
			{
				splitTempo = true;
				break;
			}
		}
		if (splitTempo)
		{
			return chart->tempo ? TimingMode::Chart : TimingMode::Song;
		}
	}
	return TimingMode::Unified;
}

BpmRange GetBpmRange(Tempo* tempo)
{
	double low = DBL_MAX, high = 0;
	if (tempo)
	{
		auto it = tempo->bpmChanges.begin();
		auto end = tempo->bpmChanges.end();
		for (; it != end; ++it)
		{
			if (it->value.bpm >= 0)
			{
				low = min(low, it->value.bpm);
				high = max(high, it->value.bpm);
			}
		}
	}
	return (high >= low) ? BpmRange{low, high} : BpmRange{0, 0};
}

} // namespace AV
