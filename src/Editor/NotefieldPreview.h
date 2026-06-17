#pragma once

#include <Editor/Common.h>

namespace Vortex {

struct NotefieldPreview {
    enum DrawMode { CMOD, XMOD, VARIABLE };

    static void create(XmrNode& settings);
    static void destroy();

    virtual void saveSettings(XmrNode& settings) = 0;

    virtual void draw() = 0;

    virtual int getY() = 0;

    virtual void setGuideCMod(int value) = 0;
    virtual int getGuideCMod() = 0;

    virtual void setGuideXMod(double value) = 0;
    virtual double getGuideXMod() = 0;

    virtual void setMode(DrawMode mode) = 0;
    virtual void setMode(int mode) = 0;
    virtual DrawMode getMode() = 0;

    virtual void toggleEnabled() = 0;
    virtual void toggleShowBeatLines() = 0;
    virtual void toggleReverseScroll() = 0;
    virtual void toggleTransparentNotes() = 0;
    virtual void toggleTransparentRegions() = 0;
    virtual void toggleGuideCMod() = 0;
    virtual void toggleGuideXMod() = 0;
    virtual void toggleGuideCrop() = 0;

    virtual bool hasEnabled() = 0;
    virtual bool hasShowBeatLines() = 0;
    virtual bool hasReverseScroll() = 0;
    virtual bool hasTransparentNotes() = 0;
    virtual bool hasTransparentRegions() = 0;
    virtual bool hasGuideCMod() = 0;
    virtual bool hasGuideXMod() = 0;
    virtual bool hasGuideCrop() = 0;
};

extern NotefieldPreview* gNotefieldPreview;

};  // namespace Vortex