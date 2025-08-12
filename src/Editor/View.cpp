#include <Editor/View.h>

#include <algorithm>

#include <Core/Utils.h>
#include <Core/Xmr.h>
#include <Core/Draw.h>
#include <Core/Gui.h>

#include <System/System.h>

#include <Editor/Music.h>
#include <Editor/Editor.h>
#include <Editor/Action.h>
#include <Editor/Menubar.h>
#include <Managers/MetadataMan.h>
#include <Managers/StyleMan.h>
#include <Managers/ChartMan.h>
#include <Managers/NoteskinMan.h>
#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Notefield.h>
#include <Editor/NotefieldPreview.h>
#include <Editor/Editing.h>
#include <Editor/Minimap.h>
#include <Editor/Selection.h>
#include <Editor/Common.h>
#include <Editor/Waveform.h>

namespace Vortex {
namespace {};  // anonymous namespace

// ================================================================================================
// ViewImpl :: member data.

struct ViewImpl : public View, public InputHandler {
    recti rect_;
    double myChartTopY;
    double myCursorTime, myCursorBeat;
    double myPixPerSec, myPixPerRow;
    int myCursorRow;

    int myReceptorY, myReceptorX, myPreviewOffset;
    double myZoomLevel, myScaleLevel;
    bool myIsDraggingReceptors, myIsDraggingReceptorsPreview;
    bool myUseTimeBasedView;
    bool myUseReverseScroll;
    bool myUseChartPreview;
    int myCustomSnap;
    int myCustomSnapSteps[193];
    SnapType mySnapType;

    // ================================================================================================
    // ViewImpl :: constructor / destructor.

    ~ViewImpl() {}

    ViewImpl()
        : myChartTopY(0.0),
          myCursorTime(0.0),
          myCursorBeat(0.0),
          myCursorRow(0),
          myReceptorY(192),
          myReceptorX(0),
          myPreviewOffset(640),
          myZoomLevel(8),
          myScaleLevel(4),
          mySnapType(ST_NONE),
          myCustomSnap(20),
          myUseTimeBasedView(true),
          myUseReverseScroll(false),
          myUseChartPreview(false),
          myIsDraggingReceptors(false),
          myIsDraggingReceptorsPreview(false),
          myPixPerSec(32),
          myPixPerRow(16 * BEATS_PER_ROW) {
        vec2i windowSize = gSystem->getWindowSize();
        rect_ = {0, 0, windowSize.x, windowSize.y};
    }

    // ================================================================================================
    // ViewImpl :: load / save settings.

    void loadSettings(XmrNode& settings) {
        XmrNode* view = settings.child("view");
        if (view) {
            view->get("useTimeBasedView", &myUseTimeBasedView);
            view->get("useReverseScroll", &myUseReverseScroll);
            view->get("useChartPreview", &myUseChartPreview);
            view->get("customSnap", &myCustomSnap);
            view->get("zoomLevel", &myZoomLevel);
            view->get("scaleLevel", &myScaleLevel);
            view->get("receptorX", &myReceptorX);
            view->get("receptorY", &myReceptorY);
            view->get("previewOffset", &myPreviewOffset);

            myCustomSnap = min(max(myCustomSnap, 5), 191);
            myZoomLevel = min(max(myZoomLevel, -2.0), 16.0);
            myScaleLevel = min(max(myScaleLevel, 1.0), 10.0);
        }

        updateScrollValues();
        updateCustomSnapSteps();
    }

    void saveSettings(XmrNode& settings) {
        XmrNode* view = settings.child("view");
        if (!view) view = settings.addChild("view");

        view->addAttrib("useTimeBasedView", myUseTimeBasedView);
        view->addAttrib("useReverseScroll", myUseReverseScroll);
        view->addAttrib("useChartPreview", myUseChartPreview);
        view->addAttrib("customSnap", (long)myCustomSnap);
        view->addAttrib("zoomLevel", (long)myZoomLevel);
        view->addAttrib("scaleLevel", (long)myScaleLevel);
        view->addAttrib("receptorX", (long)myReceptorX);
        view->addAttrib("receptorY", (long)myReceptorY);
        view->addAttrib("previewOffset", (long)myPreviewOffset);
    }

    // ================================================================================================
    // ViewImpl :: event handling.

    void onMouseScroll(MouseScroll& evt) override {
        if (evt.handled) return;
        if (gTempo->getTweakMode() &&
            (evt.keyflags & (Keyflag::CTRL | Keyflag::ALT)))
            return;

        if (mySnapType > 0) {
            setCursorRow(snapRow(myCursorRow, evt.up ? SNAP_UP : SNAP_DOWN));
        } else {
            double delta = evt.up ? -128.0 : 128.0;
            if (myUseTimeBasedView) {
                setCursorTime(myCursorTime + delta / myPixPerSec);
            } else {
                setCursorRow(myCursorRow + (int)(delta / myPixPerRow));
            }
        }
        evt.handled = true;
    }

    void onMousePress(MousePress& evt) override {
        // Dragging the preview receptors.
        if (evt.button == Mouse::LMB &&
            isMouseOverReceptorsPreview(evt.x, evt.y) && evt.unhandled()) {
            myIsDraggingReceptorsPreview = true;
            evt.setHandled();
        }
        // Dragging the receptors.
        else if (evt.button == Mouse::LMB &&
                 isMouseOverReceptors(evt.x, evt.y) && evt.unhandled()) {
            myIsDraggingReceptors = true;
            evt.setHandled();
        }

        if (evt.unhandled() && evt.button == Mouse::MMB) {
            Vortex::vec2i mouse_pos = gSystem->getMousePos();
            Vortex::ChartOffset ofs = gView->yToOffset(mouse_pos.y);

            setCursorRow(snapRow(offsetToRow(ofs), SnapDir::SNAP_CLOSEST));
            evt.setHandled();
        }
    }

    void onMouseRelease(MouseRelease& evt) override {
        // Finish receptor dragging.
        if (evt.button == Mouse::LMB && myIsDraggingReceptors) {
            myIsDraggingReceptors = false;
        }
        // Finish receptor dragging.
        else if (evt.button == Mouse::LMB && myIsDraggingReceptorsPreview) {
            myIsDraggingReceptorsPreview = false;
        }
    }

    void onKeyPress(KeyPress& evt) override {
        if (evt.handled) return;
        Key::Code kc = evt.key;

        // Switching row snap type.
        if (kc == Key::LEFT || kc == Key::RIGHT) {
            int delta = (kc == Key::RIGHT) ? 1 : -1;
            setSnapType(mySnapType + delta);
            evt.handled = true;
            return;
        }

        // Starting/pausing the song.
        if (kc == Key::SPACE) {
            if (gMusic->isPaused())
                gMusic->play();
            else
                gMusic->pause();
            evt.handled = true;
            return;
        }
    }

    void onKeyRelease(KeyRelease& evt) override {
        if (evt.handled) return;
    }

    // ================================================================================================
    // ViewImpl :: member functions.

    double getZoomLevel() const { return myZoomLevel; }

    double getScaleLevel() const { return myScaleLevel; }

    int getCustomSnap() const { return myCustomSnap; }

    SnapType getSnapType() const { return mySnapType; }

    int getSnapQuant() {
        if (mySnapType == ST_CUSTOM) {
            return myCustomSnap;
        } else {
            return 192 / sRowSnapTypes[mySnapType];
        }
    }

    bool isTimeBased() const { return myUseTimeBasedView; }

    bool hasReverseScroll() const { return myUseReverseScroll; }

    bool hasChartPreview() const { return myUseChartPreview; }

    double getPixPerSec() const { return myPixPerSec; }

    double getPixPerRow() const { return myPixPerRow; }

    double getPixPerOfs() const {
        return myUseTimeBasedView ? myPixPerSec : myPixPerRow;
    }

    int getCursorRow() const { return myCursorRow; }

    double getCursorTime() const { return myCursorTime; }

    double getCursorBeat() const { return myCursorBeat; }

    double getCursorOffset() const {
        return myUseTimeBasedView ? myCursorTime : myCursorRow;
    }

    void onChanges(int changes) {
        if (changes & VCM_TEMPO_CHANGED) {
            myCursorTime = gTempo->rowToTime(myCursorRow);
        }
    }

    void tick() {
        vec2i windowSize = gSystem->getWindowSize();
        rect_ = {0, 0, windowSize.x, windowSize.y};

        handleInputs(gSystem->getEvents());

        // handle receptor dragging.
        if (myIsDraggingReceptors) {
            myReceptorX = gSystem->getMousePos().x - CenterX(rect_);
            myReceptorY = gSystem->getMousePos().y - rect_.y;
        }
        // handle preview receptor dragging.
        else if (myIsDraggingReceptorsPreview) {
            auto mx = gSystem->getMousePos().x - CenterX(rect_) - myReceptorX;
            auto ofs = (mx << 8) / (int)(64 * myScaleLevel);
            myPreviewOffset = ofs;
        }

        // Set cursor to arrows when hovering over/dragging the receptors.
        vec2i mpos = gSystem->getMousePos();
        if (myIsDraggingReceptors || isMouseOverReceptors(mpos.x, mpos.y) ||
            myIsDraggingReceptorsPreview ||
            isMouseOverReceptorsPreview(mpos.x, mpos.y)) {
            gSystem->setCursor(Cursor::SIZE_ALL);
        }

        // Update the cursor time.
        if (!gMusic->isPaused()) {
            int endrow = gSimfile->getEndRow();
            double begintime = gTempo->rowToTime(0);
            double endtime = gTempo->rowToTime(endrow);
            if (myCursorTime > endtime) {
                myCursorTime = endtime;
                myCursorRow = endrow;
                myCursorBeat = endrow * BEATS_PER_ROW;
                gMusic->pause();
            } else {
                double time = gMusic->getPlayTime();
                myCursorTime = clamp(time, begintime, endtime);
                myCursorBeat = gTempo->timeToBeat(myCursorTime);
                myCursorRow = (int)(myCursorBeat * ROWS_PER_BEAT);
            }
        }

        // Clamp the receptor X and Y to the view region.
        int minRecepX = rect_.x - rect_.w / 2;
        int maxRecepX = RightX(rect_) - rect_.w / 2;
        myReceptorY = min(max(myReceptorY, rect_.y), BottomY(rect_));
        myReceptorX = min(max(myReceptorX, minRecepX), maxRecepX);

        // Store the y-position of time zero.
        if (myUseTimeBasedView) {
            myChartTopY =
                floor((double)myReceptorY - myCursorTime * myPixPerSec);
        } else {
            myChartTopY = floor((double)myReceptorY -
                                myCursorBeat * ROWS_PER_BEAT * myPixPerRow);
        }
    }

    void updateScrollValues() {
        myPixPerSec = round(21.077 * pow(1.518, myZoomLevel));
        myPixPerRow = round(11.588 * pow(1.48, myZoomLevel)) * BEATS_PER_ROW;
        if (myUseReverseScroll) {
            myPixPerSec = -myPixPerSec;
            myPixPerRow = -myPixPerRow;
        }
    }

    void updateCustomSnapSteps() {
        double inc = 192.0 / myCustomSnap;
        for (int i = 0; i <= myCustomSnap; ++i) {
            myCustomSnapSteps[i] = static_cast<int>(round(inc * i));
        }
    }

    void toggleReverseScroll() {
        myUseReverseScroll = !myUseReverseScroll;
        myReceptorY = rect_.h - myReceptorY;
        updateScrollValues();
        gMenubar->update(Menubar::USE_REVERSE_SCROLL);
    }

    void toggleChartPreview() {
        myUseChartPreview = !myUseChartPreview;
        gMenubar->update(Menubar::USE_CHART_PREVIEW);
    }

    void setTimeBased(bool enabled) {
        if (myUseTimeBasedView != enabled) {
            myUseTimeBasedView = enabled;
            gMenubar->update(Menubar::VIEW_MODE);
            gEditor->reportChanges(VCM_VIEW_CHANGED);
        }
    }

    void setZoomLevel(double level) {
        level = min(max(level, -2.0), 16.0);
        if (myZoomLevel != level) {
            myZoomLevel = level;
            updateScrollValues();
            gEditor->reportChanges(VCM_ZOOM_CHANGED);
        }
    }

    void setScaleLevel(double level) {
        level = min(max(level, 1.0), 10.0);
        if (myScaleLevel != level) {
            myScaleLevel = level;
            gEditor->reportChanges(VCM_ZOOM_CHANGED);
        }
    }

    void setSnapType(int type) {
        if (type < 0) type = NUM_SNAP_TYPES - 1;
        if (type > NUM_SNAP_TYPES - 1) type = 0;
        if (mySnapType != type) {
            mySnapType = (SnapType)type;
            HudNote("Snap: %s", ToString(mySnapType));
        }
    }

    void setCustomSnap(int size) {
        if (size < 4) size = 4;
        if (size > 192) size = 192;
        // If the custom snap is a non-custom value, set the snap to that value
        // instead
        for (int i = 0; i < ST_CUSTOM; i++) {
            if (192 / sRowSnapTypes[i] == size) {
                setSnapType(i);
                return;
            }
        }
        if (myCustomSnap != size) {
            myCustomSnap = size;
            updateCustomSnapSteps();
            HudNote("Custom Snap: %s", OrdinalSuffix(myCustomSnap));
            setSnapType(ST_CUSTOM);
        }
    }

    void setCursorTime(double time) {
        double begintime = gTempo->rowToTime(0);
        double endtime = gTempo->rowToTime(gSimfile->getEndRow());
        myCursorTime = min(max(begintime, time), endtime);
        myCursorBeat = gTempo->timeToBeat(myCursorTime);
        myCursorRow = (int)(myCursorBeat * ROWS_PER_BEAT);
        gMusic->seek(myCursorTime);
    }

    void setCursorRow(int row) {
        myCursorRow = min(max(row, 0), gSimfile->getEndRow());
        myCursorBeat = myCursorRow * BEATS_PER_ROW;
        myCursorTime = gTempo->rowToTime(myCursorRow);
        gMusic->seek(myCursorTime);
    }

    void setCursorOffset(ChartOffset ofs) {
        if (myUseTimeBasedView) {
            setCursorTime(ofs);
        } else {
            setCursorRow((int)(ofs + 0.5));
        }
    }

    void setCursorToNextInterval(int rows) {
        if (gView->hasReverseScroll()) rows = rows * -1;

        int rowsInInterval = abs(rows);

        if (rows < 0) {
            int delta = myCursorRow % rowsInInterval;
            if (delta == 0 ||
                (delta < rowsInInterval / 2 && !gMusic->isPaused())) {
                delta += rowsInInterval;
            }
            setCursorRow(myCursorRow - delta);
        } else {
            int delta = rowsInInterval - myCursorRow % rowsInInterval;
            setCursorRow(myCursorRow + delta);
        }
    }

    void setCursorToStream(bool top) {
        if (gView->hasReverseScroll()) top = !top;

        auto first = gNotes->begin(), n = first;
        auto last = gNotes->end() - 1;
        if (first >= last) return;

        if (top) {
            // Advance to the first note on/after the cursor row.
            while (n != last && n->row < myCursorRow) ++n;
            if (n != first) {
                // Go back to the first note before it with different interval.
                int interval = n->row - (n - 1)->row;
                while (n != first && (n->row - (n - 1)->row) == interval) --n;
            }
        } else {
            // Advance to the last note before/on the cursor row.
            while (n != last && (n + 1)->row <= myCursorRow) ++n;
            if (n != last) {
                // Advance to the first note after it with a different interval.
                int interval = n->row - (n + 1)->row;
                while (n != last && (n->row - (n + 1)->row) == interval) ++n;
            }
        }

        setCursorRow(n->row);
    }

    void setCursorToSelection(bool top) {
        if (gView->hasReverseScroll()) top = !top;

        auto region = gSelection->getSelectedRegion();
        if (region.beginRow != region.endRow) {
            setCursorRow(top ? region.beginRow : region.endRow);
        } else {
            NoteList notes;
            gSelection->getSelectedNotes(notes);
            if (notes.size())
                setCursorRow(top ? notes.begin()->row : (notes.end() - 1)->row);
        }
    }

    int getRowY(int row) const {
        if (myUseTimeBasedView) {
            return (int)(myChartTopY + gTempo->rowToTime(row) * myPixPerSec);
        } else {
            return (int)(myChartTopY + (double)row * myPixPerRow);
        }
    }

    int getTimeY(double time) const {
        if (myUseTimeBasedView) {
            return (int)(myChartTopY + time * myPixPerSec);
        } else {
            return (int)(myChartTopY + gTempo->timeToBeat(time) *
                                           ROWS_PER_BEAT * myPixPerRow);
        }
    }

    Coords getReceptorCoords() const {
        Coords out;
        auto noteskin = gNoteskin->get();
        out.y = rect_.y + myReceptorY;
        out.xc = rect_.x + rect_.w / 2 + myReceptorX;
        if (noteskin) {
            out.xl = out.xc + applyZoom(noteskin->leftX);
            out.xr = out.xc + applyZoom(noteskin->rightX);
        } else {
            int w = applyZoom(128);
            out.xl = out.xc - w;
            out.xr = out.xc + w;
        }
        return out;
    }

    Coords getNotefieldCoords() const {
        Coords out = getReceptorCoords();
        int ofs = gView->applyZoom(32);
        out.xl -= ofs, out.xr += ofs;
        if (myUseTimeBasedView && gNotefield->hasShowWaveform()) {
            int w = gWaveform->getWidth() / 2;
            out.xl = min(out.xl, out.xc - w - 4);
            out.xr = max(out.xr, out.xc + w + 4);
        }
        return out;
    }

    int columnToX(int col) const {
        auto noteskin = gNoteskin->get();
        int cx = rect_.x + rect_.w / 2 + myReceptorX;
        if (!noteskin) return cx;
        int x = (col < gStyle->getNumCols()) ? noteskin->colX[col] : 0;
        return cx + applyZoom(x);
    }

    int rowToY(int row) const {
        if (myUseTimeBasedView) {
            return (int)(myChartTopY + gTempo->rowToTime(row) * myPixPerSec);
        } else {
            return (int)(myChartTopY + (double)row * myPixPerRow);
        }
    }

    int timeToY(double time) const {
        if (myUseTimeBasedView) {
            return (int)(myChartTopY + time * myPixPerSec);
        } else {
            return (int)(myChartTopY + gTempo->timeToBeat(time) *
                                           ROWS_PER_BEAT * myPixPerRow);
        }
    }

    int offsetToY(ChartOffset ofs) {
        if (myUseTimeBasedView) {
            return (int)(myChartTopY + ofs * myPixPerSec);
        } else {
            return (int)(myChartTopY + ofs * myPixPerRow);
        }
    }

    ChartOffset yToOffset(int viewY) const {
        return ((ChartOffset)viewY - myChartTopY) / getPixPerOfs();
    }

    ChartOffset rowToOffset(int row) const {
        return myUseTimeBasedView ? gTempo->rowToTime(row) : (ChartOffset)row;
    }

    ChartOffset timeToOffset(double time) const {
        return myUseTimeBasedView ? time
                                  : (gTempo->timeToBeat(time) * ROWS_PER_BEAT);
    }

    int offsetToRow(ChartOffset ofs) const {
        return myUseTimeBasedView ? gTempo->timeToRow(ofs) : (int)(ofs + 0.5);
    }

    double offsetToTime(ChartOffset ofs) const {
        return myUseTimeBasedView ? ofs
                                  : gTempo->beatToTime(ofs * BEATS_PER_ROW);
    }

    int getWidth() const { return rect_.w; }

    int getHeight() const { return rect_.h; }

    const recti& getRect() const { return rect_; }

    void adjustForPreview(bool enabled) {
        myReceptorX +=
            applyZoom(enabled ? -myPreviewOffset : myPreviewOffset) / 2;
    }

    int getPreviewOffset() const { return applyZoom(myPreviewOffset); }

    int applyZoom(int v) const { return (v * (int)(64 * myScaleLevel)) >> 8; }

    int getNoteScale() const { return 64 * myScaleLevel; }

    int snapRow(int row, SnapDir dir) {
        if (dir == SNAP_CLOSEST) {
            if (isAlignedToSnap(row)) {
                return row;
            } else {
                int uprow = snapRow(row, SNAP_UP);
                int downrow = snapRow(row, SNAP_DOWN);
                int up = abs(row - uprow), down = abs(row - downrow);
                return (up < down) ? uprow : downrow;
            }
        } else {
            // Up and down are switched when reverse scroll is enabled.
            if (gView->hasReverseScroll()) {
                switch (dir) {
                    case SNAP_UP:
                        dir = SNAP_DOWN;
                        break;
                    case SNAP_DOWN:
                        dir = SNAP_UP;
                        break;
                };
            }

            // Bump the row by one so the snap will jump to the next position if
            // the given row is equal to the snap row.
            row = row + ((dir == SNAP_UP) ? -1 : 1);

            // Custom snaps
            if (mySnapType == ST_CUSTOM) {
                int measure = row / 192, measurerow = row % 192;
                if (dir == SNAP_UP) {
                    for (int i = myCustomSnap; i >= 0; --i) {
                        if (myCustomSnapSteps[i] <= measurerow) {
                            measurerow = myCustomSnapSteps[i];
                            break;
                        }
                    }
                } else {
                    for (int i = 0; i <= myCustomSnap; ++i) {
                        if (myCustomSnapSteps[i] >= measurerow) {
                            measurerow = myCustomSnapSteps[i];
                            break;
                        }
                    }
                }
                row = measure * 192 + measurerow;
            } else  // Regular case, snap is divisible by 192.
            {
                int snap = sRowSnapTypes[mySnapType];
                if (row % snap && dir != SNAP_UP) row += snap;
                row -= row % snap;
            }

            return row;
        }
    }

    bool isAlignedToSnap(int row) {
        int snap = sRowSnapTypes[mySnapType];

        // Special case, custom snapping.
        if (snap == 0) {
            return std::find(myCustomSnapSteps,
                             myCustomSnapSteps + myCustomSnap,
                             row % 192) != myCustomSnapSteps + myCustomSnap;
        }

        return (row % snap == 0);
    }

    bool isMouseOverReceptors(int x, int y) const {
        if (!GuiMain::isCapturingMouse()) {
            auto c = getReceptorCoords();
            int dy = applyZoom(gChart->isClosed() ? 8 : 32);
            return (x >= c.xl && x < c.xr && abs(y - myReceptorY) <= dy);
        }
        return false;
    }

    bool isMouseOverReceptorsPreview(int x, int y) const {
        if (!GuiMain::isCapturingMouse()) {
            auto c = getReceptorCoords();
            int dy = applyZoom(gChart->isClosed() ? 8 : 32);
            return (x >= c.xl + getPreviewOffset() &&
                    x < c.xr + getPreviewOffset() &&
                    abs(y - gNotefieldPreview->getY()) <= dy);
        }
        return false;
    }

};  // ViewImpl

// ================================================================================================
// View API.

View* gView = nullptr;

void View::create(XmrNode& settings) {
    gView = new ViewImpl;
    ((ViewImpl*)gView)->loadSettings(settings);
}

void View::destroy() {
    delete (ViewImpl*)gView;
    gView = nullptr;
}

};  // namespace Vortex
