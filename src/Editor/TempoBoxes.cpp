#include <Editor/TempoBoxes.h>

#include <Core/Utils.h>
#include <Core/Vector.h>
#include <Core/StringUtils.h>
#include <Core/QuadBatch.h>
#include <Core/Texture.h>
#include <Core/Gui.h>
#include <Core/Text.h>
#include <Core/Draw.h>
#include <Core/Xmr.h>

#include <Simfile/TimingData.h>
#include <Simfile/SegmentGroup.h>

#include <Managers/TempoMan.h>
#include <Managers/ChartMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Common.h>
#include <Editor/View.h>
#include <Editor/Selection.h>
#include <Editor/Menubar.h>

#include <System/System.h>

#include <algorithm>

namespace Vortex {

// ================================================================================================
// TempoBoxesImpl :: member data.

struct TempoBoxesImpl : public TempoBoxes {
    Vector<TempoBox> myBoxes;
    int myMouseOverBox;
    TileBar myBoxBar;
    TileBar myBoxHl;

    bool myShowBoxes;
    bool myShowHelp;

    // ================================================================================================
    // TempoBoxesImpl :: constructor and destructor.

    ~TempoBoxesImpl() {}

    TempoBoxesImpl() {
        myBoxBar.texture = myBoxHl.texture =
            Texture("assets/icons tempo.png", false);
        myBoxBar.border = myBoxHl.border = 8;
        myBoxBar.uvs = {0, 0, 1, 0.5f};
        myBoxHl.uvs = {0, 0.5f, 1, 1};

        myShowBoxes = true;
        myShowHelp = true;
    }

    // ================================================================================================
    // TempoBoxesImpl :: load / save settings.

    void loadSettings(XmrNode& settings) {
        XmrNode* view = settings.child("view");
        if (view) {
            view->get("showTempoHelp", &myShowHelp);
        }
    }

    void saveSettings(XmrNode& settings) {
        XmrNode* view = settings.child("view");
        if (!view) view = settings.addChild("view");

        view->addAttrib("showTempoHelp", myShowHelp);
    }

    // ================================================================================================
    // TempoBoxesImpl :: update.

    void update() {
        myBoxes.clear();

        if (gSimfile->isClosed()) return;

        // Create a box for every segment.
        auto segments = gTempo->getSegments();
        for (auto it = segments->begin(), end = segments->end(); it != end;
             ++it) {
            auto type = it->type();
            auto meta = Segment::meta[type];
            for (auto seg = it->begin(), segEnd = it->end(); seg != segEnd;
                 ++seg) {
                std::string desc = meta->getDescription(seg.ptr);
                myBoxes.push_back(TempoBox{desc, seg->row, type, 0, 0, 0});
            }
        }

        // Sort the list of boxes by row.
        std::stable_sort(myBoxes.begin(), myBoxes.end(),
                         [](const TempoBox& a, const TempoBox& b) {
                             return (a.row < b.row);
                         });

        // Precalculate the x-position and width of each box.
        TextStyle textStyle;
        int previousRow = -1;
        int stacks[2] = {0, 0};
        for (TempoBox& box : myBoxes) {
            Text::arrange(Text::MC, textStyle, box.str.c_str());
            int width = max(32, Text::getWidth() + 24);
            box.width = width;

            int side = Segment::meta[box.type]->side;
            int offset = width * (side * 2 - 1);

            box.x = (side - 1) * width;
            if (box.row == previousRow) {
                box.x += stacks[side];
                stacks[side] += offset;
            } else {
                stacks[side] = offset;
                stacks[1 - side] = 0;
                previousRow = box.row;
            }
        }
    }

    void onChanges(int changes) {
        if (changes & VCM_TEMPO_CHANGED) {
            update();
        }
    }

    // ================================================================================================
    // TempoBoxesImpl :: toggle visuals.

    void toggleShowBoxes() {
        myShowBoxes = !myShowBoxes;
        gMenubar->update(Menubar::SHOW_TEMPO_BOXES);
    }

    void toggleShowHelp() {
        myShowHelp = !myShowHelp;
        gMenubar->update(Menubar::SHOW_TEMPO_HELP);
    }

    bool hasShowBoxes() { return myShowBoxes; }

    bool hasShowHelp() { return myShowHelp; }

    // ================================================================================================
    // TempoBoxesImpl :: selection.

    void deselectAll() {
        for (auto& box : myBoxes) {
            box.isSelected = 0;
        }
        if (gSelection->getType() == Selection::TEMPO) {
            gSelection->setType(Selection::NONE);
        }
    }

    int selectAll() {
        for (auto& box : myBoxes) {
            box.isSelected = 1;
        }
        if (myBoxes.size()) {
            gSelection->setType(Selection::TEMPO);
        }
        return myBoxes.size();
    }

    int selectType(Segment::Type type) {
        for (auto& box : myBoxes) {
            box.isSelected = (box.type == type ? 1 : 0);
        }
        if (myBoxes.size()) {
            gSelection->setType(Selection::TEMPO);
        }
        return myBoxes.size();
    }

    int selectSegments(const Tempo* tempo) {
        int numSelected = 0;
        auto boxEnd = myBoxes.end();
        auto segments = gTempo->getSegments();
        for (auto list = segments->begin(), listEnd = segments->end();
             list != listEnd; ++list) {
            auto type = list->type();
            auto box = myBoxes.begin();
            for (auto seg = list->begin(), segEnd = list->end(); seg != segEnd;
                 ++seg) {
                while (box != boxEnd &&
                       (box->type != type || box->row < seg->row)) {
                    ++box;
                }
                bool select = (box != boxEnd && box->row == seg->row);
                box->isSelected = select;
                numSelected += select;
            }
        }
        return numSelected;
    }

    template <typename Predicate>
    int performSelection(SelectModifier mod, Predicate pred) {
        int numSelected = 0;
        auto box = myBoxes.begin();
        auto end = myBoxes.end();
        if (mod == SELECT_SET) {
            for (; box != end; ++box) {
                uint32_t set = pred(box);
                numSelected += set;
                box->isSelected = set;
            }
        } else if (mod == SELECT_ADD) {
            for (; box != end; ++box) {
                uint32_t set = pred(box);
                numSelected += set & (box->isSelected ^ 1);
                box->isSelected |= set;
            }
        } else if (mod == SELECT_SUB) {
            for (; box != end; ++box) {
                uint32_t set = pred(box);
                numSelected += set & box->isSelected;
                box->isSelected &= set ^ 1;
            }
        }
        gSelection->setType(Selection::TEMPO);
        return numSelected;
    }

    int selectRows(SelectModifier mod, int begin, int end, int xl, int xr) {
        auto coords = gView->getNotefieldCoords();
        const int baseX[2] = {coords.xl, coords.xr};
        return performSelection(mod, [&](const TempoBox* box) {
            int side = Segment::meta[box->type]->side;
            int x1 = baseX[side] + box->x + 8;
            int x2 = x1 + box->width - 16, row = box->row;
            return (x2 >= xl && x1 <= xr && row >= begin && row <= end);
        });
    }

    int selectTime(SelectModifier mod, double begin, double end, int xl,
                   int xr) {
        auto coords = gView->getNotefieldCoords();
        const int baseX[2] = {coords.xl, coords.xr};
        TempoTimeTracker tracker;
        return performSelection(mod, [&](const TempoBox* box) {
            int side = Segment::meta[box->type]->side;
            int x1 = baseX[side] + box->x + 8;
            int x2 = x1 + box->width - 16;
            double time = tracker.advance(box->row);
            return (x2 >= xl && x1 <= xr && time >= begin && time <= end);
        });
    }

    bool noneSelected() const {
        for (auto& box : myBoxes) {
            if (box.isSelected) return false;
        }
        return true;
    }

    // ================================================================================================
    // TempoBoxesImpl :: tick.

    void tick() {
        myMouseOverBox = -1;
        if (!GuiMain::isCapturingMouse()) {
            bool timeBased = gView->isTimeBased();
            double oy = gView->offsetToY(0.0);
            double dy = gView->getPixPerOfs();

            TempoTimeTracker tracker;
            auto coords = gView->getNotefieldCoords();
            const int baseX[2] = {coords.xl, coords.xr};
            vec2i mpos = gSystem->getMousePos();
            for (int i = 0; i < myBoxes.size(); ++i) {
                auto& box = myBoxes[i];
                int y = (int)(oy + dy * (timeBased ? tracker.advance(box.row)
                                                   : (double)box.row));
                int side = Segment::meta[box.type]->side;
                int x = baseX[side] + box.x;
                if (IsInside(recti{x, y - 16, (int)box.width, 32}, mpos.x,
                             mpos.y)) {
                    myMouseOverBox = i;
                    break;
                }
            }
        }
    }

    // ================================================================================================
    // TempoBoxesImpl :: draw.

    void draw() {
        if (myShowBoxes == false || myBoxes.empty() ||
            gView->getScaleLevel() < 2)
            return;

        auto coords = gView->getNotefieldCoords();
        const int baseX[2] = {coords.xl, coords.xr};

        bool timeBased = gView->isTimeBased();
        double oy = gView->offsetToY(0.0);
        double dy = gView->getPixPerOfs();
        int viewTop = gView->getRect().y;
        int viewBtm = viewTop + gView->getHeight();

        Renderer::resetColor();
        Renderer::bindTexture(myBoxHl.texture.handle());
        Renderer::bindShader(Renderer::SH_TEXTURE);

        // First pass, draw the box sprites.
        int previousRow = 0;
        TempoTimeTracker tracker;
        auto batch = Renderer::batchTC();
        for (const TempoBox& box : myBoxes) {
            int y = (int)(oy + dy * (timeBased ? tracker.advance(box.row)
                                               : (double)box.row));
            if (y < viewTop - 16 || y > viewBtm + 16) continue;

            int side = Segment::meta[box.type]->side;
            int x = baseX[side] + box.x;

            int flags = side * TileBar::FLIP_H;
            recti r = {x, y - 16, (int)box.width, 32};

            uint32_t color = Segment::meta[box.type]->color;
            myBoxBar.draw(&batch, r, color, flags);
            if (box.isSelected) myBoxHl.draw(&batch, r, Colors::white, flags);

            previousRow = box.row;
        }
        batch.flush();

        // Second pass, draw the text labels.
        tracker = TempoTimeTracker();
        TextStyle textStyle;
        for (const TempoBox& box : myBoxes) {
            int y = (int)(oy + dy * (timeBased ? tracker.advance(box.row)
                                               : (double)box.row));
            if (y < viewTop - 16 || y > viewBtm + 16) continue;

            int side = Segment::meta[box.type]->side;
            int x = baseX[side] + box.x + side * 4 - 2;

            Text::arrange(Text::MC, textStyle, box.str.c_str());
            Text::draw(recti{x, y - 17, (int)box.width, 32});
        }

        // Display detailed info of the mouse over box.
        if (myMouseOverBox >= 0 && myShowHelp &&
            !gSystem->isMouseDown(Mouse::LMB)) {
            drawBoxHelp(myBoxes[myMouseOverBox]);
        }
    }

    void drawBoxHelp(const TempoBox& box) {
        auto meta = Segment::meta[box.type];

        auto coords = gView->getNotefieldCoords();
        int x = (meta->side ? coords.xr : coords.xl) + box.x + box.width / 2;
        int y = gView->rowToY(box.row) + 24;

        TextStyle style;

        style.fontSize = 12;
        Text::arrange(Text::TC, style, meta->singular);
        vec2i nameSize = Text::getSize();

        style.fontSize = 10;
        style.textColor = RGBAtoColor32(192, 192, 192, 255);
        Text::arrange(Text::TC, style, meta->help);
        vec2i helpSize = Text::getSize();

        int w = max(nameSize.x, helpSize.x) + 12;
        int h = nameSize.y + helpSize.y + 8;
        recti r = recti{x - w / 2, y, w, h};

        Draw::roundedBox(r, RGBAtoColor32(128, 128, 128, 255));
        r = Shrink(r, 1);
        Draw::roundedBox(r, RGBAtoColor32(26, 26, 26, 255));

        Text::draw(vec2i{x, y + nameSize.y + 4});

        style.fontSize = 12;
        style.textColor = Colors::white;
        Text::arrange(Text::MC, style, meta->singular);
        Text::draw(vec2i{x, y + 10});
    }

    const Vector<TempoBox>& getBoxes() { return myBoxes; }

};  // TempoBoxesImpl

// ================================================================================================
// TempoBoxes API.

TempoBoxes* gTempoBoxes = nullptr;

void TempoBoxes::create(XmrNode& settings) {
    gTempoBoxes = new TempoBoxesImpl;
    ((TempoBoxesImpl*)gTempoBoxes)->loadSettings(settings);
}

void TempoBoxes::destroy() {
    delete (TempoBoxesImpl*)gTempoBoxes;
    gTempoBoxes = nullptr;
}

};  // namespace Vortex