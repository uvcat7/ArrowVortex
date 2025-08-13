#pragma once

#include <Simfile/Simfile.h>

namespace Vortex {

/// Manages the active simfile.
struct SimfileMan {
    static void create();
    static void destroy();

    /// Called by the editor when changes were made to the simfile.
    virtual void onChanges(int changes) = 0;

    /// Loads a new simfile from file and opens it for editing.
    virtual bool load(const std::string& path) = 0;

    /// Saves the simfile that is currently open for editing.
    virtual bool save(const std::string& dir, const std::string& name,
                      SimFormat format) = 0;

    /// Closes the simfile that is currently open for editing.
    virtual void close() = 0;

    /// Returns true if a simfile is open for editing, false otherwise.
    virtual bool isOpen() const = 0;

    /// Returns false if a simfile is open for editing, true otherwise.
    virtual bool isClosed() const = 0;

    /// Inserts a new chart into the chart list and opens it for editing.
    virtual void addChart(const Style* style, std::string artist,
                          Difficulty diff, int meter) = 0;

    /// Removes a chart from the chart list, and closes it if necessary.
    virtual void removeChart(const Chart* chart) = 0;

    /// Sort chart order based on properties.
    virtual void sortCharts() = 0;

    /// Opens one of the charts in the simfile for editing.
    virtual void openChart(int index) = 0;

    /// Opens one of the charts in the simfile for editing.
    virtual void openChart(const Chart* chart) = 0;

    /// Opens the next chart in the simfile for editing.
    virtual void nextChart() = 0;

    /// Opens the previous chart in the simfile for editing.
    virtual void previousChart() = 0;

    /// Returns the directory of the active simfile.
    virtual std::string getDir() const = 0;

    /// Returns the filename (without extension) of the active simfile.
    virtual std::string getFile() const = 0;

    /// Returns the number of charts in the active simfile.
    virtual int getNumCharts() const = 0;

    /// Returns the chart at the given index.
    virtual const Chart* getChart(int index) const = 0;

    /// Returns the end row of the active simfile.
    virtual int getEndRow() const = 0;

    /// Returns the active simfile.
    virtual const Simfile* get() const = 0;
};

extern SimfileMan* gSimfile;

};  // namespace Vortex
