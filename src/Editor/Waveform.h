#pragma once

#include <Core/Draw.h>
#include <Core/Vector.h>

namespace Vortex {

struct Waveform {
   public:
    struct ColorScheme {
        colorf bg, wave, filter;
    };

    enum Preset { PRESET_VORTEX, PRESET_DDREAM };
    enum WaveShape { WS_RECTIFIED, WS_SIGNED };
    enum Luminance { LL_UNIFORM, LL_AMPLITUDE };
    enum FilterType { FT_HIGH_PASS, FT_LOW_PASS };

    static void create(XmrNode& settings);
    static void destroy();

    virtual void saveSettings(XmrNode& settings) = 0;

    virtual void onChanges(int changes) = 0;

    virtual void tick() = 0;
    virtual void drawBackground() = 0;
    virtual void drawPeaks() = 0;

    virtual void clearBlocks() = 0;

    virtual void setOverlayFilter(bool enabled) = 0;
    virtual bool getOverlayFilter() = 0;
    virtual void enableFilter(FilterType type, double strength) = 0;
    virtual void disableFilter() = 0;

    virtual void setPreset(Preset preset) = 0;

    virtual void setColors(ColorScheme scheme) = 0;
    virtual ColorScheme getColors() = 0;

    virtual void setLuminance(Luminance lum) = 0;
    virtual Luminance getLuminance() = 0;

    virtual void setWaveShape(WaveShape style) = 0;
    virtual WaveShape getWaveShape() = 0;

    virtual void setAntiAliasing(int level) = 0;
    virtual int getAntiAliasing() = 0;

    virtual int getWidth() = 0;
};

extern Waveform* gWaveform;

};  // namespace Vortex
