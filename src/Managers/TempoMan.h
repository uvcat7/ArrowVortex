#pragma once

#include <Editor/Clipboard.h>

#include <Simfile/Tempo.h>
#include <Simfile/SegmentGroup.h>

namespace Vortex {

/// Manages the tempo of the active chart/simfile.
struct TempoMan {
    static void create();
    static void destroy();

    static const char* clipboardTag;

    /// Indicates if the simfile has split timing, and if so, which timing is
    /// used.
    enum TimingMode {
        TIMING_UNIFIED,  ///< The simfile does not have split timing.
        TIMING_SONG,   ///< The simfile has split timing, but the current chart
                       ///< uses song timing.
        TIMING_STEPS,  ///< The simfile has split timing, and the current chart
                       ///< uses split timing.
    };

    /// Indicates which tempo value is being tweaked, when tweak mode is active.
    enum TweakMode {
        TWEAK_NONE,    ///< Tweak mode is currently not active.
        TWEAK_OFFSET,  ///< The music offset is being tweaked.
        TWEAK_BPM,     ///< The BPM at the tweak row is being tweaked.
        TWEAK_STOP,    ///< The stop at the tweak row is being tweaked.
    };

    /// Determines how the start and end value in selectRange are interpreted.
    enum SelectionRange {
        RANGE_ROWS,  ///< Start and end represent rows values.
        RANGE_TIME,  ///< Start and end represent time values.
    };

    // Called when the active chart or simfile changes.
    virtual void update(Simfile* simfile, Chart* chart) = 0;

    /// Converts a time offset to a row offset.
    virtual int timeToRow(double time) const = 0;

    /// Converts a time offset to a beat offset.
    virtual double timeToBeat(double time) const = 0;

    /// Converts a row offset to a time offset.
    virtual double rowToTime(int row) const = 0;

    /// Converts a beat offset to a time offset.
    virtual double beatToTime(double beat) const = 0;

    /// Converts a beat offset to a measure offset.
    virtual double beatToMeasure(double beat) const = 0;

    /// Returns the BPM at the given row.
    virtual double getBpm(int row) const = 0;

    /// Returns the translated row at a given row.
    virtual double rowToScroll(int row) const = 0;

    /// Returns the translated beat at a given beat.
    virtual double beatToScroll(double beat) const = 0;

    /// Returns the Speed value at a given position. Both values are needed as
    /// speed can use beats or time.
    virtual double positionToSpeed(double beat, double time) const = 0;

    /// Editing functions.
    virtual void modify(const SegmentEdit& edit) = 0;
    virtual void modify(const SegmentEdit& edit, bool clearRegion) = 0;
    virtual void insertRows(int row, int numRows, bool curChartOnly) = 0;
    virtual void removeSelectedSegments() = 0;

    /// Clipboard
    virtual int minSelectionRow() const = 0;
    virtual void pasteFromClipboard(ClipboardData clipboard, bool insert) = 0;
    virtual void copyToClipboard(std::string& out, int minRow) = 0;

    /// Sets the global music offset.
    virtual void setOffset(double offset) = 0;

    /// Sets the BPM display to actual values.
    virtual void setDefaultBpm() = 0;

    /// Sets the BPM display to random values.
    virtual void setRandomBpm() = 0;

    /// Sets the BPM display to custom values.
    virtual void setCustomBpm(BpmRange range) = 0;

    /// Start tweaking the music offset.
    virtual void startTweakingOffset() = 0;

    /// Start tweaking the BPM value at the given row.
    virtual void startTweakingBpm(int row) = 0;

    /// Start tweaking the stop value at the given row.
    virtual void startTweakingStop(int row) = 0;

    /// Sets a new tweak value, if tweaking is currently active.
    virtual void setTweakValue(double value) = 0;

    /// Stops tweaking. If apply is false then the original value is restored.
    virtual void stopTweaking(bool apply = false) = 0;

    /// Returns the tweak mode that is currently active.
    virtual TweakMode getTweakMode() const = 0;

    /// Returns the current tweak value.
    virtual double getTweakValue() const = 0;

    /// Returns the current tweak row.
    virtual int getTweakRow() const = 0;

    /// Returns the number of seconds the music is delayed, relative to the
    /// first beat.
    virtual double getOffset() const = 0;

    /// Returns the timing mode of the current chart.
    virtual TimingMode getTimingMode() const = 0;

    /// Returns the type of the display BPM.
    virtual DisplayBpm getDisplayBpmType() const = 0;

    /// Returns the range of the display BPM.
    virtual BpmRange getDisplayBpmRange() const = 0;

    /// Returns the minimum and maximum BPM of the simfile, ignoring negative
    /// BPM values.
    virtual BpmRange getBpmRange() const = 0;

    /// Returns the timing data of the active chart/simfile.
    virtual const TimingData& getTimingData() const = 0;

    /// Returns the list of segments for the active tempo.
    virtual const SegmentGroup* getSegments() const = 0;

    /// Adds a segment to the tempo.
    template <typename T>
    void addSegment(const T& segment) {
        SegmentEdit edit;
        edit.add.append(segment);
        modify(edit);
    }

    /// Remove a segment from the tempo.
    void removeSegment(Segment::Type type, int row) {
        SegmentEdit edit;
        edit.rem.append(type, row);
        modify(edit);
    }

    /// Add a (normally redundant) BPM change at `targetRow` equal to BPM at
    /// that row
    virtual void injectBoundingBpmChange(const int targetRow) = 0;
    /// Enable destructive visual sync so that calls to `tickVisualSync` will
    ///   shift `targetRow`, starting a history chain so it can be undone in one
    ///   action
    virtual void startDestructiveVisualSync(const int targetRow) = 0;
    /// Enable non-destructive visual sync
    ///   so that calls to `tickVisualSync` will
    ///   shift `targetRow`, starting a history chain so it can be undone in one
    ///   action
    virtual void startNondestructiveVisualSync(const int targetRow) = 0;
    /// Shift previously configured target row to `targetTime`
    virtual void tickVisualSync(const double targetTime) = 0;
    /// Record BPM changes to history and disable visual sync
    virtual void endVisualSync() = 0;
    /// Return whether we're currently visually syncing a row
    virtual bool isInVisualSync() = 0;
};

extern TempoMan* gTempo;

};  // namespace Vortex