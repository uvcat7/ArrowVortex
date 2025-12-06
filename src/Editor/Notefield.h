#pragma once

#include <Editor/Common.h>

namespace Vortex {

struct Notefield {
    static void create();
    static void destroy();

    /// Called by the editor when changes were made to the simfile.
    virtual void onChanges(int changes) = 0;

    virtual void setBgAlpha(int percent) = 0;
    virtual int getBgAlpha() = 0;

    virtual void draw() = 0;

    virtual void drawGhostNote(const Note& n) = 0;

    virtual void toggleShowWaveform() = 0;
    virtual void toggleShowBeatLines() = 0;
    virtual void toggleShowNotes() = 0;
    virtual void toggleShowSongPreview() = 0;

    virtual bool hasShowWaveform() = 0;
    virtual bool hasShowBeatLines() = 0;
    virtual bool hasShowNotes() = 0;
    virtual bool hasShowSongPreview() = 0;
};

extern Notefield* gNotefield;

};  // namespace Vortex