#include <Editor/Statusbar.h>

#include <Core/StringUtils.h>
#include <Core/Utils.h>
#include <Core/Xmr.h>
#include <Core/Gui.h>
#include <Core/Draw.h>
#include <Core/Text.h>

#include <System/System.h>

#include <Editor/Menubar.h>
#include <Editor/Action.h>
#include <Editor/Editor.h>
#include <Editor/Common.h>
#include <Editor/View.h>

#include <Simfile/SegmentGroup.h>

#include <Managers/MetadataMan.h>
#include <Managers/SimfileMan.h>
#include <Managers/ChartMan.h>
#include <Managers/TempoMan.h>

namespace Vortex {

// ================================================================================================
// StatusbarImpl :: member data.

struct StatusbarImpl : public Statusbar {
    bool myShowChart;
    bool myShowSnap;
    bool myShowBpm;
    bool myShowRow;
    bool myShowBeat;
    bool myShowMeasure;
    bool myShowTime;
    bool myShowTimingMode;
    bool myShowScroll;
    bool myShowSpeed;

    // ================================================================================================
    // StatusbarImpl :: constructor / destructor.

    ~StatusbarImpl() = default;

    StatusbarImpl() {
        myShowChart = true;
        myShowSnap = true;
        myShowBpm = true;
        myShowRow = false;
        myShowBeat = false;
        myShowMeasure = true;
        myShowTime = true;
        myShowTimingMode = true;
        myShowScroll = false;
        myShowSpeed = false;
    }

    // ================================================================================================
    // StatusbarImpl :: load / save settings.

    void loadSettings(XmrNode& settings) {
        XmrNode* statusbar = settings.child("statusbar");
        if (statusbar) {
            statusbar->get("showChart", &myShowChart);
            statusbar->get("showSnap", &myShowSnap);
            statusbar->get("showBpm", &myShowBpm);
            statusbar->get("showRow", &myShowRow);
            statusbar->get("showBeat", &myShowBeat);
            statusbar->get("showMeasure", &myShowMeasure);
            statusbar->get("showTime", &myShowTime);
            statusbar->get("showTimingMode", &myShowTimingMode);
            statusbar->get("myShowScroll", &myShowScroll);
            statusbar->get("myShowSpeed", &myShowSpeed);
        }
    }

    void saveSettings(XmrNode& settings) override {
        XmrNode* statusbar = settings.addChild("statusbar");

        statusbar->addAttrib("showChart", myShowChart);
        statusbar->addAttrib("showSnap", myShowSnap);
        statusbar->addAttrib("showBpm", myShowBpm);
        statusbar->addAttrib("showRow", myShowRow);
        statusbar->addAttrib("showBeat", myShowBeat);
        statusbar->addAttrib("showMeasure", myShowMeasure);
        statusbar->addAttrib("showTime", myShowTime);
        statusbar->addAttrib("showTimingMode", myShowTimingMode);
        statusbar->addAttrib("myShowScroll", myShowScroll);
        statusbar->addAttrib("myShowSpeed", myShowSpeed);
    }

    // ================================================================================================
    // StatusbarImpl :: member functions.

    void draw() override {
        Vector<std::string> info;

        TextStyle textStyle;
        textStyle.textFlags = Text::MARKUP;

        if (myShowChart) {
            info.push_back(gChart->getDescription());
        }

        if (myShowSnap) {
            const char* snap = ToString(gView->getSnapType());
            info.push_back(Str::fmt("{tc:888}Snap:{tc} %1").arg(snap));
        }

        if (myShowBpm && gSimfile->isOpen()) {
            double bpm = gTempo->getBpm(gView->getCursorRow());
            info.push_back(Str::fmt("{tc:888}BPM:{tc} %1").arg(bpm, 3, 3));
        }

        if (myShowRow) {
            int row = gView->getCursorRow();
            info.push_back(Str::fmt("{tc:888}Row:{tc} %1").arg(row));
        }

        if (myShowBeat) {
            double beat = gView->getCursorBeat();
            info.push_back(Str::fmt("{tc:888}Beat:{tc} %1").arg(beat, 3, 3));
        }

        if (myShowMeasure) {
            double measure = gTempo->beatToMeasure(gView->getCursorBeat());
            info.push_back(
                Str::fmt("{tc:888}Measure:{tc} %1").arg(measure, 2, 2));
        }

        if (myShowTime) {
            std::string time = Str::formatTime(gView->getCursorTime());
            info.push_back(Str::fmt("{tc:888}Time:{tc} %1").arg(time));
        }
        if (myShowTimingMode) {
            switch (gTempo->getTimingMode()) {
                case TempoMan::TIMING_UNIFIED:
                    break;
                case TempoMan::TIMING_SONG:
                    info.push_back("{tc:888}Timing:{tc} song");
                    break;
                case TempoMan::TIMING_STEPS:
                    info.push_back("{tc:888}Timing:{tc} steps");
                    break;
            }
        }

        if (myShowScroll) {
            int row = gView->getCursorRow();
            double ratio = gTempo->getSegments()->getRecent<Scroll>(row).ratio;
            info.push_back(Str::fmt("{tc:888}Scroll:{tc} %1").arg(ratio, 2, 2));
        }
        if (myShowSpeed) {
            double beat = gView->getCursorBeat();
            double time = gView->getCursorTime();
            double speed = gTempo->positionToSpeed(beat, time);
            info.push_back(Str::fmt("{tc:888}Speed:{tc} %1").arg(speed, 3, 3));
        }

        if (info.size()) {
            std::string str = Str::join(info, "  ");
            Text::arrange(Text::MC, textStyle, str.c_str());

            recti view = gView->getRect();
            view = {view.x + 128, view.y + view.h - 32, view.w - 256 - 32, 24};

            int w = Text::getSize().x + 12;
            int x = view.x + view.w / 2 - w / 2;
            Draw::fill({x, view.y, w, view.h}, Color32(0, 128));

            Text::draw(view);
        }
    }

    // ================================================================================================
    // StatusbarImpl :: toggle/check functions.

    void toggleChart() override {
        myShowChart = !myShowChart;
        gMenubar->update(Menubar::STATUSBAR_CHART);
    }

    void toggleSnap() override {
        myShowSnap = !myShowSnap;
        gMenubar->update(Menubar::STATUSBAR_SNAP);
    }

    void toggleBpm() override {
        myShowBpm = !myShowBpm;
        gMenubar->update(Menubar::STATUSBAR_BPM);
    }

    void toggleRow() override {
        myShowRow = !myShowRow;
        gMenubar->update(Menubar::STATUSBAR_ROW);
    }

    void toggleBeat() override {
        myShowBeat = !myShowBeat;
        gMenubar->update(Menubar::STATUSBAR_BEAT);
    }

    void toggleMeasure() override {
        myShowMeasure = !myShowMeasure;
        gMenubar->update(Menubar::STATUSBAR_MEASURE);
    }

    void toggleTime() override {
        myShowTime = !myShowTime;
        gMenubar->update(Menubar::STATUSBAR_TIME);
    }

    void toggleTimingMode() override {
        myShowTimingMode = !myShowTimingMode;
        gMenubar->update(Menubar::STATUSBAR_TIMING_MODE);
    }

    void toggleScroll() override {
        myShowScroll = !myShowScroll;
        gMenubar->update(Menubar::STATUSBAR_SCROLL);
    }

    void toggleSpeed() override {
        myShowSpeed = !myShowSpeed;
        gMenubar->update(Menubar::STATUSBAR_SPEED);
    }

    bool hasChart() override { return myShowChart; }

    bool hasSnap() override { return myShowSnap; }

    bool hasBpm() override { return myShowBpm; }

    bool hasRow() override { return myShowRow; }

    bool hasBeat() override { return myShowBeat; }

    bool hasMeasure() override { return myShowMeasure; }

    bool hasTime() override { return myShowTime; }

    bool hasTimingMode() override { return myShowTimingMode; }

    bool hasScroll() override { return myShowScroll; }

    bool hasSpeed() override { return myShowSpeed; }

};  // StatusbarImpl

// ================================================================================================
// Statusbar :: create / destroy.

Statusbar* gStatusbar = nullptr;

void Statusbar::create(XmrNode& settings) {
    gStatusbar = new StatusbarImpl();
    static_cast<StatusbarImpl*>(gStatusbar)->loadSettings(settings);
}

void Statusbar::destroy() {
    delete static_cast<StatusbarImpl*>(gStatusbar);
    gStatusbar = nullptr;
}

};  // namespace Vortex
