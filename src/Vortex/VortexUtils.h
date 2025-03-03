#pragma once

#include <Simfile/Common.h>
#include <Simfile/Chart.h>
#include <Simfile/Tempo.h>

#include <Vortex/Common.h>

namespace AV {

TimingMode GetTimingMode(Chart* chart);

BpmRange GetBpmRange(Tempo* tempo);

} // namespace AV
