#pragma once

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Colorf.h>

#include <Core/Utils/Enum.h>

namespace AV {
namespace Waveform {

enum class Preset { Vortex, DDReam };
enum class WaveShape { Rectified, Signed };
enum class Luminance { Uniform, Amplitude };
enum class FilterType { HighPass, LowPass };

void initialize(const XmrDoc& settings);
void deinitialize();

void drawBackground();
void drawPeaks();

void clearBlocks();

void overlayFilter(bool enabled);
void enableFilter(FilterType type, double strength);
void disableFilter();

void setPreset(Preset preset);

int getWidth();

} // namespace Waveform
} // namespace AV
