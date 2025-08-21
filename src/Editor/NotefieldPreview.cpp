#include <Editor/NotefieldPreview.h>

#include <math.h>
#include <stdint.h>

#include <System/Debug.h>

#include <Core/Draw.h>
#include <Core/Gui.h>
#include <Core/Utils.h>
#include <Core/ImageLoader.h>
#include <Core/QuadBatch.h>
#include <Core/Xmr.h>

#include <Simfile/TimingData.h>

#include <Managers/TempoMan.h>
#include <Managers/NoteskinMan.h>
#include <Managers/StyleMan.h>
#include <Managers/ChartMan.h>
#include <Managers/NoteMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Menubar.h>
#include <Editor/Music.h>
#include <Editor/View.h>

namespace Vortex {

namespace {

typedef View::Coords Coords;

struct DrawPosHelper {
    typedef NotefieldPreview::DrawMode DrawMode;

    explicit DrawPosHelper(DrawMode mode, bool reverse = false)
        : tracker(gTempo->getTimingData()) {
        int dir = reverse ? -1 : 1;
        if (mode == DrawMode::CMOD) {
            deltaY = -gView->getPixPerSec() * dir;
            baseY = -floor(gView->getCursorTime() * deltaY);
            advanceFunc = TimeBasedAdvance;
            getFunc = TimeBasedGet;
        } else if (mode == DrawMode::XMOD) {
            deltaY = -gView->getPixPerRow() * dir;
            baseY = -floor(gView->getCursorBeat() * ROWS_PER_BEAT * deltaY);
            advanceFunc = RowBasedAdvance;
            getFunc = RowBasedGet;
        } else if (mode == DrawMode::XMOD_ALL) {
            deltaY = -gView->getPixPerRow() * dir;
            baseY =
                -floor(gTempo->beatToScroll(gView->getCursorBeat()) * deltaY);
            advanceFunc = RowBasedAlteredAdvance;
            getFunc = RowBasedAlteredGet;
        }
    }

    inline int advance(int row) { return advanceFunc(this, row); }
    inline int get(int row, double time) const {
        return getFunc(this, row, time);
    }
    static int RowBasedAdvance(DrawPosHelper* dp, int row) {
        return static_cast<int>(dp->baseY + dp->deltaY * row);
    }
    static int RowBasedGet(const DrawPosHelper* dp, int row, double time) {
        return static_cast<int>(dp->baseY + dp->deltaY * row);
    }
    static int RowBasedAlteredAdvance(DrawPosHelper* dp, int row) {
        return static_cast<int>(dp->baseY +
                                dp->deltaY * gTempo->rowToScroll(row));
    }
    static int RowBasedAlteredGet(const DrawPosHelper* dp, int row,
                                  double time) {
        return static_cast<int>(dp->baseY +
                                dp->deltaY * gTempo->rowToScroll(row));
    }
    static int TimeBasedAdvance(DrawPosHelper* dp, int row) {
        return static_cast<int>(dp->baseY +
                                dp->deltaY * dp->tracker.advance(row));
    }
    static int TimeBasedGet(const DrawPosHelper* dp, int row, double time) {
        return static_cast<int>(dp->baseY + dp->deltaY * time);
    }

    typedef int (*AdvanceFunc)(DrawPosHelper*, int);
    typedef int (*GetFunc)(const DrawPosHelper*, int, double);

    double baseY, deltaY;
    TempoTimeTracker tracker;
    AdvanceFunc advanceFunc;
    GetFunc getFunc;
};

};  // anonymous namespace

// ================================================================================================
// NotefieldPreviewImpl :: member data.

struct NotefieldPreviewImpl : public NotefieldPreview {
    Texture myNoteLabelsTex;
    BatchSprite myNoteLabels[2];

    int myColX[SIM_MAX_COLUMNS], myX, myY, myW, maxY;

    int scale, cols;
    double speed, currentRow;

    bool myEnabled;
    bool myShowBeatLines;
    bool myUseReverseScroll;
    DrawMode myDrawMode = XMOD_ALL;

    // ================================================================================================
    // NotefieldPreviewImpl :: constructor and destructor.

    ~NotefieldPreviewImpl() = default;

    NotefieldPreviewImpl() {
        myEnabled = false;
        myShowBeatLines = true;
        myUseReverseScroll = false;
        myNoteLabelsTex = Texture("assets/note labels.png", false);

        BatchSprite::init(myNoteLabels, 2, 2, 1, 32, 32);
    }

    // ================================================================================================
    // NotefieldPreviewImpl :: load / save settings.

    static const char* ToString(DrawMode mode) {
        if (mode == XMOD_ALL) return "all";
        if (mode == XMOD) return "xmod";
        return "cmod";
    }

    static DrawMode ToMode(const std::string& str) {
        if (str == "all") return XMOD_ALL;
        if (str == "xmod") return XMOD;
        return CMOD;
    }

    void loadSettings(XmrNode& settings) {
        XmrNode* preview = settings.child("preview");
        if (preview) {
            preview->get("enabled", &myEnabled);
            preview->get("beatlines", &myShowBeatLines);
            preview->get("reverse", &myUseReverseScroll);

            const char* mode = preview->get("mode");
            if (mode) myDrawMode = ToMode(mode);
        }
    }

    void saveSettings(XmrNode& settings) override {
        XmrNode* preview = settings.child("preview");
        if (!preview) preview = settings.addChild("preview");

        preview->addAttrib("enabled", myEnabled);
        preview->addAttrib("beatlines", myShowBeatLines);
        preview->addAttrib("reverse", myUseReverseScroll);
        preview->addAttrib("mode", ToString(myDrawMode));
    }

    // ================================================================================================
    // NotefieldPreviewImpl :: drawing routines.
    void updateNotefieldSize() {
        Coords coords = gView->getReceptorCoords();
        int ofs = gView->applyZoom(32);
        coords.xl -= ofs, coords.xr += ofs;

        myW = coords.xr - coords.xl;
        myX = coords.xl + gView->getPreviewOffset();
        myY = coords.y;

        maxY = gView->getHeight() + 32;

        if (myUseReverseScroll) {
            myY = maxY - 32 - myY;
        }
    }

    void draw() override {
        if (!myEnabled || gChart->isClosed()) return;

        // Update common variables.
        auto beat = gView->getCursorBeat(), time = gView->getCursorTime();

        speed =
            (myDrawMode == XMOD_ALL ? gTempo->positionToSpeed(beat, time) : 1);
        cols = gStyle->getNumCols();
        scale = gView->getNoteScale();
        currentRow = beat * ROWS_PER_BEAT;
        updateNotefieldSize();

        BatchSprite::setScale(scale);

        // Calculate the x-position of each note column.
        auto noteskin = gNoteskin->get();
        if (noteskin) {
            for (int c = 0; c < cols; ++c) {
                myColX[c] = gView->columnToX(c) + gView->getPreviewOffset();
            }
        }

        recti view = gView->getRect();
        Draw::fill({myX, view.y, myW, view.h}, RGBAtoColor32(0, 0, 0, 128));

        if (myShowBeatLines) drawBeatLines();
        drawReceptors();
        drawNotes();
        if (!gMusic->isPaused()) drawReceptorGlow();
    }

    // ================================================================================================
    // NotefieldPreviewImpl :: segments.

    void drawBeatLines() {
        Renderer::resetColor();
        Renderer::bindShader(Renderer::SH_COLOR);

        bool zoomedIn = (gView->getZoomLevel() >= 4);

        // Determine the first row and last row that should show beat lines.
        int drawBeginRow =
            max(0, static_cast<int>(currentRow) - ROWS_PER_BEAT * 4);
        int drawEndRow = min(static_cast<int>(currentRow) + ROWS_PER_BEAT * 20,
                             gSimfile->getEndRow()) +
                         1;

        auto& sigs = gTempo->getTimingData().sigs;
        auto it = sigs.begin(), end = sigs.end();

        // Skip over time signatures that are not drawn.
        auto next = it + 1;
        while (next != end && next->row <= drawBeginRow) {
            it = next, ++next;
        }

        // Skip over measures at the start of the time signature that are not
        // drawn.
        int measure = it->measure, row = it->row;
        if (drawBeginRow > row + it->rowsPerMeasure) {
            int skip = (drawBeginRow - it->row - it->rowsPerMeasure) /
                       it->rowsPerMeasure;
            measure += skip;
            row += skip * it->rowsPerMeasure;
        }

        // Start drawing measures and beat lines.
        auto batch = Renderer::batchC();
        uint32_t halfColor = ToColor32({1, 1, 1, 0.4f});
        uint32_t fullColor = ToColor32({1, 1, 1, 0.7f});
        DrawPosHelper drawPos = DrawPosHelper(myDrawMode, myUseReverseScroll);
        while (it != end && row < drawEndRow) {
            int endRow = drawEndRow;
            if (next != end) endRow = min(endRow, next->row);
            while (row < endRow) {
                // Measure line and measure label.
                int y = myY - drawPos.advance(row);

                // Modify y to account for Tempo Speed
                y = myY + ((y - myY) * speed);

                // Don't show beatlines off the screen
                if (y > -32 && y < maxY)
                    Draw::fill(&batch, {myX, y, myW, 1}, fullColor);

                // Beat lines.
                if (zoomedIn) {
                    int beatRow = row + ROWS_PER_BEAT;
                    int measureEnd = row + it->rowsPerMeasure;
                    while (beatRow < measureEnd) {
                        if (beatRow > drawEndRow) break;

                        int y = myY - drawPos.advance(beatRow);

                        // Modify y to account for Tempo Speed
                        y = myY + ((y - myY) * speed);

                        // Don't show beatlines off the screen
                        if (y > -32 && y < maxY)
                            Draw::fill(&batch, {myX, y, myW, 1}, halfColor);

                        beatRow += ROWS_PER_BEAT;
                    }
                }

                ++measure;
                row += it->rowsPerMeasure;
            }
            it = next, ++next;
        }
        batch.flush();
    }

    void drawReceptors() {
        auto noteskin = gNoteskin->get();

        Renderer::resetColor();
        Renderer::bindShader(Renderer::SH_TEXTURE);
        Renderer::bindTexture(noteskin->recepTex.handle());

        // Calculate the beat pulse value for the receptors.
        double beat = gTempo->timeToBeat(gView->getCursorTime());
        float beatfrac = static_cast<float>(beat - floor(beat));
        uint8_t beatpulse = static_cast<uint8_t>(
            min(max(static_cast<int>((2 - beatfrac * 4) * 255), 0), 255));

        // Draw the receptors.
        auto batch = Renderer::batchTC();
        for (int c = 0; c < cols; ++c) {
            noteskin->recepOff[c].draw(&batch, myColX[c], myY,
                                       static_cast<uint8_t>(255));
            noteskin->recepOn[c].draw(&batch, myColX[c], myY, beatpulse);
        }
        batch.flush();
    }

    void drawReceptorGlow() {
        auto noteskin = gNoteskin->get();
        double time = gView->getCursorTime();
        auto prevNotes = gNotes->getNotesBeforeTime(time);

        Renderer::resetColor();
        Renderer::bindShader(Renderer::SH_TEXTURE);
        Renderer::bindTexture(noteskin->glowTex.handle());

        // Draw the receptor glow based on the time elapsed since the previous
        // note.
        auto batch = Renderer::batchTC();
        for (int c = 0; c < cols; ++c) {
            auto note = prevNotes[c];
            if (!note) continue;
            double lum = 1.5 - (time - note->endtime) * 6.0;
            uint8_t alpha = static_cast<uint8_t>(
                clamp(static_cast<int>(lum * 255.0), 0, 255));
            if (alpha > 0) {
                noteskin->recepGlow[c].draw(&batch, myColX[c], myY, alpha);
            }
        }
        batch.flush();
    }

    void drawNotes() {
        auto noteskin = gNoteskin->get();

        Renderer::resetColor();
        Renderer::bindShader(Renderer::SH_TEXTURE);
        Renderer::bindTexture(noteskin->noteTex.handle());

        // Render arrows/holds/mines interleaved, so the z-order is correct.
        auto batch = Renderer::batchTC();
        DrawPosHelper drawPos = DrawPosHelper(myDrawMode, myUseReverseScroll);
        for (auto& note : *gNotes) {
            // Determine the y-position of the note.
            int y = myY - drawPos.get(note.row, note.time),
                by = myY - (note.row == note.endrow
                                ? y
                                : drawPos.get(note.endrow, note.endtime));

            // Simulate chart preview. We want to not show arrows that go past
            // the targets (mines go past the targets in Stepmania, so we keep
            // those.) Notes 20 beats into the future are also not rendered in
            // game.
            if (!(note.isMine | note.isFake) && note.endrow < currentRow ||
                note.row > currentRow + 20 * ROWS_PER_BEAT) {
                continue;
            }

            // Modify y to account for Tempo Speed
            y = myY + ((y - myY) * speed);
            by = myY + ((by - myY) * speed);

            // Don't show notes off the screen
            if (max(y, by) < -32 || min(y, by) > maxY) continue;

            int rowtype = ToRowType(note.row);
            int col = note.col, x = myColX[col];

            // Body and tail for holds.
            if (note.endrow > note.row) {
                int signedScale = by < y ? -scale : scale;
                int index = note.isRoll * cols + col;

                auto& body = noteskin->holdBody[index];
                auto& tail = noteskin->holdTail[index];

                int bodyY = noteskin->holdY[index] * signedScale / 256;
                int tailH = tail.height * signedScale / 512;

                // Only show the part of the tail past the targets, and don't
                // show the arrow.
                if (currentRow > note.row && currentRow <= note.endrow) {
                    body.draw(&batch, x, myY + bodyY, by + bodyY);
                    tail.draw(&batch, x, by - tailH, by + tailH);
                    continue;
                } else {
                    body.draw(&batch, x, y + bodyY, by + bodyY);
                    tail.draw(&batch, x, by - tailH, by + tailH);
                }
            }

            // Note sprite.
            switch (note.type) {
                case NOTE_STEP_OR_HOLD:
                case NOTE_ROLL:
                case NOTE_LIFT:
                case NOTE_FAKE: {
                    int index =
                        (note.player * cols + note.col) * NUM_ROW_TYPES +
                        rowtype;
                    uint8_t alpha = note.isFake ? 128 : 255;
                    noteskin->note[index].draw(&batch, x, y, alpha);
                    break;
                }
                case NOTE_MINE: {
                    int index = note.player * cols + note.col;
                    noteskin->mine[index].draw(&batch, x, y);
                    break;
                }
            }
        }
        batch.flush();

        // Draw indicator sprites for fake notes and lift notes.
        Renderer::bindTexture(myNoteLabelsTex.handle());
        batch = Renderer::batchTC();
        for (auto& note : *gNotes) {
            if (note.type == NOTE_LIFT) {
                int y = myY - drawPos.get(note.row, note.time);
                y = myY + ((y - myY) * speed);
                if (y < -32 || y > maxY ||
                    note.row > currentRow + 20 * ROWS_PER_BEAT)
                    continue;
                int col = note.col, x = myColX[col];
                myNoteLabels[0].draw(&batch, x, y);
            }
        }
        batch.flush();
    }

    // ================================================================================================
    // NotefieldPreviewImpl :: toggle/check functions.

    int getY() override { return myY; }

    void setMode(DrawMode mode) override {
        myDrawMode = mode;
        gMenubar->update(Menubar::PREVIEW_VIEW_MODE);
    }

    DrawMode getMode() override { return myDrawMode; }

    void toggleEnabled() override {
        myEnabled = !myEnabled;
        gView->adjustForPreview(myEnabled);
        gMenubar->update(Menubar::PREVIEW_ENABLED);
    }

    void toggleShowBeatLines() override {
        myShowBeatLines = !myShowBeatLines;
        gMenubar->update(Menubar::PREVIEW_SHOW_BEATLINES);
    }

    void toggleReverseScroll() override {
        myUseReverseScroll = !myUseReverseScroll;
        gMenubar->update(Menubar::PREVIEW_SHOW_REVERSE_SCROLL);
    }

    bool hasEnabled() override { return myEnabled; }

    bool hasReverseScroll() override { return myUseReverseScroll; }

    bool hasShowBeatLines() override { return myShowBeatLines; }
};

// ================================================================================================
// NotefieldPreview API.

NotefieldPreview* gNotefieldPreview = nullptr;

void NotefieldPreview::create(XmrNode& settings) {
    gNotefieldPreview = new NotefieldPreviewImpl;
    static_cast<NotefieldPreviewImpl*>(gNotefieldPreview)
        ->loadSettings(settings);
}

void NotefieldPreview::destroy() {
    delete static_cast<NotefieldPreviewImpl*>(gNotefieldPreview);
    gNotefieldPreview = nullptr;
}

};  // namespace Vortex