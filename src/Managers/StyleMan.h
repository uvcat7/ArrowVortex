#pragma once

#include <Core/Core.h>

namespace Vortex {

/// Holds data that determines the play style of a chart.
struct Style {
    int index;

    std::string id;
    std::string name;

    int numCols;
    int numPlayers;

    int* mirrorTableH;  // [numCols]
    int* mirrorTableV;  // [numCols]

    int padWidth;
    int padHeight;

    vec2i* padColPositions;     // [numCols]
    vec2i* padInitialFeetCols;  // [numPlayers]
};

/// Manages the style of the active simfile.
struct StyleMan {
    static void create();
    static void destroy();

    // Called when the active chart changes.
    virtual void update(Chart* chart) = 0;

    /// Returns the first style that matches the given id, or null if none was
    /// found.
    virtual const Style* findStyle(const std::string& id) = 0;

    /// Returns the first style that matches the given column and player count.
    /// If a match is not found, a new style with the given parameters is
    /// created.
    virtual const Style* findStyle(const std::string& chartName, int numCols,
                                   int numPlayers) = 0;

    /// Returns the first style that matches the given id, column, and player
    /// count. If a match is not found, a new style with the given parameters is
    /// created.
    virtual const Style* findStyle(const std::string& chartName, int numCols,
                                   int numPlayers, const std::string& id) = 0;

    // Returns the number of available styles.
    virtual int getNumStyles() const = 0;

    // Returns the number of columns of the active style.
    virtual int getNumCols() const = 0;

    // Returns the number of players of the active style.
    virtual int getNumPlayers() const = 0;

    // Returns the style data of the style at index.
    virtual Style* get(int index) const = 0;

    // Returns the style data of the active chart.
    virtual Style* get() const = 0;
};

extern StyleMan* gStyle;

};  // namespace Vortex
