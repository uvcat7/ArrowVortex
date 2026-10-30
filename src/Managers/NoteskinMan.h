#pragma once

#include <Core/Draw.h>

namespace Vortex {

struct BatchSprite;

/// Holds data that determines the appearance of the notefield for a style.
struct Noteskin {
    typedef BatchSprite Spr;

    int leftX;
    int rightX;

    Spr* note;       // [numPlayers][numCols][NUM_ROW_TYPES]
    Spr* mine;       // [numPlayers][numCols]
    Spr* holdBody;   // [2][numCols]
    Spr* holdTail;   // [2][numCols]
    Spr* recepOn;    // [numCols]
    Spr* recepOff;   // [numCols]
    Spr* recepGlow;  // [numCols]

    int* colX;   // [numCols]
    int* holdY;  // [2][numCols]

    Texture noteTex;
    Texture recepTex;
    Texture glowTex;
};

/// Manages the noteskin of the active chart.
struct NoteskinMan {
    static void create(XmrNode& settings);
    static void destroy();

    virtual void saveSettings(XmrNode& settings) = 0;

    // Called when the active chart changes.
    virtual void update(Chart* chart) = 0;

    // Returns the number of available noteskin types.
    virtual int getNumTypes() const = 0;

    // Returns the name of the noteskin type.
    virtual const std::string& getName(int type) const = 0;

    // Returns true if the noteskin type supports the active style, false
    // otherwise.
    virtual bool isSupported(int type) const = 0;

    // Loads a new noteskin type for the active style.
    virtual void setType(int type) = 0;

    // Returns the noteskin type of the active style.
    virtual int getType() const = 0;

    /// Returns the noteskin data of the active style.
    virtual const Noteskin* get() const = 0;
};

extern NoteskinMan* gNoteskin;

};  // namespace Vortex
