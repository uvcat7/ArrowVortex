#include <Editor/TextOverlay.h>

#include <Core/Utils.h>
#include <Core/StringUtils.h>
#include <Core/Xmr.h>
#include <Core/Gui.h>
#include <Core/GuiDraw.h>
#include <Core/Draw.h>
#include <Core/Text.h>

#include <System/Debug.h>
#include <System/System.h>
#include <System/File.h>

#include <Editor/Common.h>
#include <Editor/Shortcuts.h>
#include <Editor/Action.h>
#include <string>

#include <algorithm>

namespace Vortex {

namespace {

#define OVERLAY ((TextOverlayImpl*)gTextOverlay)
#define TEXT_Y_START gSystem->applyScaleFactor(32)

struct HudEntry {
    std::string text;
    TextOverlay::MessageType type;
    float timeLeft;
};

struct ProgressMessage {
    int id;
    std::string text;
};

struct Shortcut {
    std::string a, b;
    bool isHeader;
};

static const int NUM_ICONS = 20;

static uint8_t supportLink[] = "https://discord.gg/GCNAyDmjqy";
static uint8_t githubLink[] = "https://github.com/uvcat7/ArrowVortex";

static const char* iconNames[NUM_ICONS] = {
    "up one",      "up half",        "down one",       "down half", "halve",
    "double",      "full selection", "half selection", "undo",      "redo",
    "calculate",   "tweak",          "search",         "copy",      "play",
    "arrow right", "folder",         "trash",          "keyboard",  "mouse"};

};  // anonymous namespace

// ================================================================================================
// TextOverlayImpl :: member data.

struct TextOverlayImpl : public TextOverlay {
    std::vector<HudEntry> hudEntries_;
    std::vector<InfoBox*> infoBoxes_;
    std::vector<std::string> logEntries_;
    std::vector<Shortcut> displayShortcuts_;
    std::string debugLog_;

    int textOverlayScrollPos_, textOverlayScrollEnd_, textOverlayPageSize_;
    Mode textOverlayMode_;

    // ================================================================================================
    // TextOverlayImpl :: constructor and destructor.

    ~TextOverlayImpl() = default;

    TextOverlayImpl() {
        textOverlayMode_ = HUD;

        textOverlayScrollPos_ = 0;
        textOverlayScrollEnd_ = 0;
        textOverlayPageSize_ = 0;

        Texture icons[NUM_ICONS];
        Texture::createTiles("assets/icons text.png", 64, 64, NUM_ICONS, icons,
                             false, Texture::ALPHA);
        int tex_size = gSystem->applyScaleFactor(16);
        for (int i = 0; i < NUM_ICONS; ++i) {
            Text::createGlyph(iconNames[i], icons[i], 0, tex_size / 4 - 1, 0,
                              tex_size, tex_size);
        }
    }

    // ================================================================================================
    // TextOverlayImpl :: shortcuts.

    void addShortcutHeader(const char* name) {
        Shortcut shortcut = {std::string(name), "", true};
        displayShortcuts_.emplace_back(shortcut);
    }

    void addShortcut(const char* name, const char* keys) {
        Shortcut shortcut = {std::string(name), std::string(keys), false};
        displayShortcuts_.emplace_back(shortcut);
    }

    void addShortcut(Action::Type action, const char* name) {
        std::string notation = gShortcuts->getNotation(action, true);
        if (!notation.length())
            addShortcut(name, "{tc:666}-- unassigned --");
        else
            addShortcut(name, notation.c_str());
    }

    void LoadShortcuts() {
        if (!displayShortcuts_.empty()) return;

        addShortcutHeader("Basic");
        addShortcut("Play/pause", "Space");

        addShortcut(Action::SNAP_NEXT, "Increase snap");
        addShortcut(Action::SNAP_PREVIOUS, "Decrease snap");
        addShortcut(Action::CURSOR_UP, "Move cursor up");
        addShortcut(Action::CURSOR_DOWN, "Move cursor down");
        addShortcut(Action::ZOOM_IN, "Zoom in");
        addShortcut(Action::ZOOM_OUT, "Zoom out");
        addShortcut(Action::SCALE_INCREASE, "Scale Increase");
        addShortcut(Action::SCALE_DECREASE, "Scale Decrease");

        addShortcutHeader("File");
        addShortcut(Action::FILE_OPEN, "Open");
        addShortcut(Action::FILE_SAVE, "Save");
        addShortcut(Action::FILE_SAVE_AS, "Save as");
        addShortcut(Action::FILE_CLOSE, "Close");

        addShortcutHeader("Dialogs");
        addShortcut(Action::OPEN_DIALOG_SONG_PROPERTIES, "Simfile properties");
        addShortcut(Action::OPEN_DIALOG_CHART_PROPERTIES, "Chart properties");
        addShortcut(Action::OPEN_DIALOG_CHART_LIST, "Chart list");
        addShortcut(Action::OPEN_DIALOG_NEW_CHART, "New chart");
        addShortcut(Action::OPEN_DIALOG_ADJUST_SYNC, "Adjust sync");
        addShortcut(Action::OPEN_DIALOG_ADJUST_TEMPO, "Adjust tempo");
        addShortcut(Action::OPEN_DIALOG_DANCING_BOT, "Dancing bot");
        addShortcut(Action::OPEN_DIALOG_GENERATE_NOTES, "Generate notes");
        addShortcut(Action::OPEN_DIALOG_TEMPO_BREAKDOWN, "Tempo breakdown");

        addShortcutHeader("Selection");
        addShortcut(Action::SELECT_REGION, "Select region");
        addShortcut(Action::SELECT_REGION_BEFORE_CURSOR,
                    "Select region before cursor");
        addShortcut(Action::SELECT_REGION_AFTER_CURSOR,
                    "Select region after cursor");

        addShortcutHeader("Visual sync");
        addShortcut(Action::SET_VISUAL_SYNC_CURSOR_ANCHOR,
                    "Set to target nearby row of snap");
        addShortcut(Action::SET_VISUAL_SYNC_RECEPTOR_ANCHOR,
                    "Set to target receptor's row");
        addShortcut("Non-destructive row shift", "B");
        addShortcut("Destructive row shift", "Alt-B");
        addShortcut(Action::INJECT_BOUNDING_BPM_CHANGE,
                    "Add bounding BPM change");

        addShortcutHeader("Chart");
        addShortcut(Action::CHART_PREVIOUS, "Previous chart");
        addShortcut(Action::CHART_NEXT, "Next chart");
        addShortcut(Action::CHART_DELETE, "Delete chart");

        addShortcutHeader("Music");
        addShortcut(Action::VOLUME_INCREASE, "Increase volume");
        addShortcut(Action::VOLUME_DECREASE, "Decrease volume");
        addShortcut(Action::SPEED_INCREASE, "Increase speed");
        addShortcut(Action::SPEED_DECREASE, "Decrease speed");

        addShortcutHeader("Navigation");
        addShortcut(Action::CURSOR_PREVIOUS_BEAT, "Previous beat");
        addShortcut(Action::CURSOR_NEXT_BEAT, "Next beat");
        addShortcut(Action::CURSOR_PREVIOUS_MEASURE, "Previous measure");
        addShortcut(Action::CURSOR_NEXT_MEASURE, "Next measure");
        addShortcut(Action::CURSOR_STREAM_START, "Start of stream");
        addShortcut(Action::CURSOR_STREAM_END, "End of stream");
        addShortcut(Action::CURSOR_SELECTION_START, "Start of selection");
        addShortcut(Action::CURSOR_SELECTION_END, "End of selection");
        addShortcut(Action::CURSOR_CHART_START, "Start of chart");
        addShortcut(Action::CURSOR_CHART_END, "End of chart");

        addShortcutHeader("Quick Editing");
        addShortcut(Action::EDIT_TEMPO_BPM, "Edit BPM");
        addShortcut(Action::EDIT_TEMPO_STOP, "Edit Stop");
        addShortcut(Action::EDIT_TEMPO_DELAY, "Edit Delay");
        addShortcut(Action::EDIT_TEMPO_WARP, "Edit Warp");
        addShortcut(Action::EDIT_TEMPO_TIME_SIG, "Edit Time Signature");
        addShortcut(Action::EDIT_TEMPO_TICK_COUNT, "Edit Tick Count");
        addShortcut(Action::EDIT_TEMPO_COMBO, "Edit Combo");
        addShortcut(Action::EDIT_TEMPO_SPEED, "Edit Speed");
        addShortcut(Action::EDIT_TEMPO_SCROLL, "Edit Scroll");
        addShortcut(Action::EDIT_TEMPO_FAKE, "Edit Fake");
        addShortcut(Action::EDIT_TEMPO_LABEL, "Edit Label");
    }

    // ================================================================================================
    // TextOverlayImpl :: member functions.

    void UpdateScrollValues() {
        vec2i size = gSystem->getWindowSize();
        int height = gSystem->applyScaleFactor(18);
        int height_header = gSystem->applyScaleFactor(24);
        if (textOverlayMode_ == MESSAGE_LOG) {
            textOverlayPageSize_ = std::max(0, (size.y - TEXT_Y_START) / 16);
            textOverlayScrollEnd_ = std::max(
                0, static_cast<int>(logEntries_.size() - textOverlayPageSize_));
        } else if (textOverlayMode_ == SHORTCUTS) {
            textOverlayPageSize_ = size.y;
            textOverlayScrollEnd_ = TEXT_Y_START;
            for (const Shortcut& e : displayShortcuts_) {
                textOverlayScrollEnd_ += e.isHeader ? height_header : height;
            }
            textOverlayScrollEnd_ =
                std::max(0, textOverlayScrollEnd_ - textOverlayPageSize_);
        } else {
            textOverlayPageSize_ = size.y;
            textOverlayScrollEnd_ = 0;
        }

        textOverlayScrollPos_ =
            std::clamp(textOverlayScrollPos_, 0, textOverlayScrollEnd_);
    }

    void tick() override {
        UpdateScrollValues();
        switch (textOverlayMode_) {
            case HUD:
                tickHud();
                break;
            case MESSAGE_LOG:
                tickMessageLog();
                break;
            case DEBUG_LOG:
                tickDebugLog();
                break;
            case SHORTCUTS:
                tickShortcuts();
                break;
            case ABOUT:
                tickAbout();
                break;
        };
    }

    void onKeyPress(KeyPress& evt) override {
        if (textOverlayMode_ == MESSAGE_LOG && evt.key == Key::DELETE &&
            !evt.handled) {
            logEntries_.clear();
            evt.handled = true;
        }
        if (textOverlayMode_ != HUD && evt.key == Key::ESCAPE && !evt.handled) {
            textOverlayMode_ = HUD;
            evt.handled = true;
        }
    }

    void onMouseScroll(MouseScroll& evt) override {
        if (textOverlayMode_ != HUD && !evt.handled) {
            int delta = evt.up ? -1 : +1;
            textOverlayScrollPos_ +=
                delta * std::max(1, textOverlayPageSize_ / 10);
            textOverlayScrollPos_ =
                std::clamp(textOverlayScrollPos_, 0, textOverlayScrollEnd_);
            evt.handled = true;
        }
    }

    void show(Mode mode) override {
        if (textOverlayMode_ == mode) {
            textOverlayMode_ = HUD;
        } else {
            textOverlayMode_ = mode;
        }

        debugLog_.clear();

        if (textOverlayMode_ == SHORTCUTS) {
            LoadShortcuts();
        } else if (textOverlayMode_ == DEBUG_LOG) {
            bool success;
            debugLog_ = File::getText(fs::path("ArrowVortex.log"), &success);
            if (success) {
                Str::erase(debugLog_, 0, 3);  // UTF-8 BOM.
            } else {
                debugLog_ = "(could not open ArrowVortex.log)";
            }
        }

        UpdateScrollValues();
    }

    bool isOpen() override { return textOverlayMode_ != HUD; }

    void DrawTitleText(const char* left, const char* mid, const char* right) {
        vec2i size = gSystem->getWindowSize();

        int height = gSystem->applyScaleFactor(28);
        Draw::fill({0, 0, size.x, height}, RGBAtoColor32(0, 0, 0, 191));
        Draw::fill({0, height, size.x, 1}, Colors::white);

        if (right) {
            Text::arrange(Text::TR, right);
            Text::draw(vec2i{size.x - 4, 4});
        }
        if (mid) {
            Text::arrange(Text::TC, mid);
            Text::draw(vec2i{size.x / 2, 4});
        }
        if (left) {
            Text::arrange(Text::TL, left);
            Text::draw(vec2i{4, 4});
        }
    }

    void DrawScrollbar() {
        if (textOverlayPageSize_ > 0 && textOverlayScrollEnd_ > 0) {
            vec2i size = gSystem->getWindowSize();
            int width = gSystem->applyScaleFactor(12);
            int height = gSystem->applyScaleFactor(28);
            recti box = {size.x - width - 4, height + 4, width,
                         size.y - height - 8};

            int barY =
                box.y + box.h * textOverlayScrollPos_ /
                            (textOverlayScrollEnd_ + textOverlayPageSize_);
            int barH = box.h * textOverlayPageSize_ /
                       (textOverlayScrollEnd_ + textOverlayPageSize_);

            Draw::outline(box, RGBAtoColor32(255, 255, 255, 128));
            Draw::fill(box, RGBAtoColor32(255, 255, 255, 64));
            Draw::fill({box.x, barY, box.w, barH},
                       RGBAtoColor32(255, 255, 255, 128));
        }
    }

    void draw() override {
        if (textOverlayMode_ != HUD) {
            vec2i size = gSystem->getWindowSize();
            Draw::fill({0, 0, size.x, size.y}, RGBAtoColor32(0, 0, 0, 191));
        }
        switch (textOverlayMode_) {
            case HUD:
                drawHud();
                break;
            case MESSAGE_LOG:
                drawMessageLog();
                break;
            case DEBUG_LOG:
                drawDebugLog();
                break;
            case SHORTCUTS:
                drawShortcuts();
                break;
            case ABOUT:
                drawAbout();
                break;
        };
    }

    void addMessage(const char* str, MessageType type) override {
        std::string msg = str;
        if (type == NOTE) {
            hudEntries_.emplace_back(msg, type, 0.5f);
        } else if (type == INFO) {
            logEntries_.emplace_back(msg);
            hudEntries_.emplace_back(msg, type, 3.0f);
        } else if (type == WARNING) {
            Str::insert(msg, 0, "{tc:FF0}WARNING:{tc} ");
            logEntries_.emplace_back(msg);
            hudEntries_.emplace_back(msg, type, 6.0f);
        } else if (type == ERROR) {
            Str::insert(msg, 0, "{tc:F44}ERROR:{tc} ");
            logEntries_.emplace_back(msg);
            hudEntries_.emplace_back(msg, type, 6.0f);
        }
    }

    void addInfoBox(InfoBox* box) { infoBoxes_.emplace_back(box); }

    void removeInfoBox(InfoBox* box) { std::erase(infoBoxes_, box); }

    // ================================================================================================
    // TextOverlayImpl :: hud.

    void tickHud() {
        for (int i = hudEntries_.size() - 1; i >= 0; --i) {
            float fade = 0.75f * (hudEntries_.size() - 1 - i);
            if (hudEntries_[i].type == NOTE) {
                fade += 0.25;
            } else {
                fade += 0.5f;
            }
            float delta = std::clamp(deltaTime.count() * fade, 0.0, 1.0);

            hudEntries_[i].timeLeft -= delta;
            if (hudEntries_[i].timeLeft <= -0.5f)
                hudEntries_.erase(hudEntries_.begin() + i);
        }
    }

    void drawHud() {
        TextStyle textStyle;
        textStyle.textFlags = Text::MARKUP;

        vec2i view = gSystem->getWindowSize();
        int x = view.x / 2, y = 8;
        for (auto& box : infoBoxes_) {
            vec2i size = {gSystem->applyScaleFactor(280), box->height()};
            recti r = {x - size.x / 2, y, size.x, size.y};
            recti r2 = {r.x - 4, r.y - 4, r.w + 8, r.h + 8};

            Draw::fill(r2, RGBAtoColor32(0, 0, 0, 128));
            Draw::outline(r2, RGBAtoColor32(128, 128, 128, 128));

            box->draw(r);

            y += size.y + gSystem->applyScaleFactor(280);
        }

        x = 4, y = 4;
        for (auto& m : hudEntries_) {
            int a = std::clamp(static_cast<int>(m.timeLeft * 512.0f + 256.0f),
                               0, 255);

            textStyle.textColor = Color32a(textStyle.textColor, a);
            textStyle.shadowColor = Color32a(textStyle.shadowColor, a);

            Text::arrange(Text::TL, textStyle, m.text.c_str());
            Text::draw(recti{x, y, view.x - 8, view.y - y});

            y += Text::getSize().y + 2;
        }

        // Speed up the message removal if we have too many.
        if (y > view.y) hudEntries_.erase(hudEntries_.begin());
    }

    // ================================================================================================
    // TextOverlayImpl :: message log.

    void tickMessageLog() {}

    void drawMessageLog() {
        vec2i size = gSystem->getWindowSize();

        TextStyle textStyle;
        textStyle.textFlags = Text::MARKUP;

        // Messages.
        int x = 4, y = TEXT_Y_START, bottomY = size.y;
        for (int i = textOverlayScrollPos_, n = logEntries_.size();
             i < n && y < bottomY - 16; ++i) {
            auto& m = logEntries_[i];
            Text::arrange(Text::TL, textStyle, m.c_str());
            Text::draw(recti{x, y, size.x - 8, size.y});
            y += Text::getSize().y + 2;
        }

        DrawTitleText("MESSAGE LOG", "[ESC/F2] close", "[Delete] clear");
        DrawScrollbar();
    }

    // ================================================================================================
    // TextOverlayImpl :: debug log.

    void tickDebugLog() {}

    void drawDebugLog() {
        vec2i size = gSystem->getWindowSize();

        TextStyle textStyle;
        textStyle.fontSize = 11;
        Text::arrange(Text::TL, textStyle, debugLog_.c_str());
        Text::draw(recti{4, TEXT_Y_START, size.x - 8, size.y});

        DrawTitleText("DEBUG LOG", "[ESC] close", "");
        DrawScrollbar();
    }

    // ================================================================================================
    // TextOverlayImpl :: shortcuts.

    void tickShortcuts() {}

    void drawShortcuts() {
        int x = gSystem->getWindowSize().x / 2 - 325, w = 650;
        int y = TEXT_Y_START - textOverlayScrollPos_;
        int y_off = gSystem->applyScaleFactor(18);
        TextStyle textStyle;
        textStyle.textFlags = Text::MARKUP;
        for (const Shortcut& e : displayShortcuts_) {
            if (e.isHeader) {
                Text::arrange(Text::TL, e.a.c_str());
                Text::draw(vec2i{x, y});
                Draw::fill({x, y + y_off, w, 1}, Colors::white);
                y += gSystem->applyScaleFactor(24);
            } else {
                Text::arrange(Text::TR, textStyle, e.a.c_str());
                Text::draw(vec2i{x + w / 2 - 10, y});
                Text::arrange(Text::TL, textStyle, e.b.c_str());
                Text::draw(vec2i{x + w / 2 + 10, y});

                y += gSystem->applyScaleFactor(18);
            }
        }

        DrawTitleText("SHORTCUTS", "[ESC/F1] close", nullptr);
        DrawScrollbar();
    }

    // ================================================================================================
    // TextOverlayImpl :: about.

    void tickAbout() {
        vec2i mpos = gSystem->getMousePos();
        if (IsInside(getSupportButtonRect(), mpos.x, mpos.y)) {
            gSystem->setCursor(Cursor::HAND);
            MousePress* mp = nullptr;
            if (gSystem->getEvents().next(mp)) {
                if (!mp->handled) {
                    mp->handled = true;
                    gSystem->openWebpage(
                        reinterpret_cast<const char*>(supportLink));
                }
            }
        }

        if (IsInside(getGithubButtonRect(), mpos.x, mpos.y)) {
            gSystem->setCursor(Cursor::HAND);
            MousePress* mp = nullptr;
            if (gSystem->getEvents().next(mp)) {
                if (!mp->handled) {
                    mp->handled = true;
                    gSystem->openWebpage(
                        reinterpret_cast<const char*>(githubLink));
                }
            }
        }
    }

    recti getGithubButtonRect() {
        vec2i win = gSystem->getWindowSize();
        int h = gSystem->applyScaleFactor(28);
        int w = gSystem->applyScaleFactor(64);
        int y_off = 128 - 5 * gSystem->applyScaleFactor(16);
        return {win.x / 2 - 3 * w / 2, win.y / 2 - y_off, w, h};
    }

    recti getSupportButtonRect() {
        vec2i win = gSystem->getWindowSize();
        int h = gSystem->applyScaleFactor(28);
        int w = gSystem->applyScaleFactor(64);
        int y_off = 128 - 5 * gSystem->applyScaleFactor(16);
        return {win.x / 2 + w / 2, win.y / 2 - y_off, w, h};
    }

    void drawAbout() {
        vec2i size = gSystem->getWindowSize();

        Text::arrange(Text::BC, "ArrowVortex release v1.0.1");
        int h = gSystem->applyScaleFactor(16);
        Text::draw(vec2i{size.x / 2, size.y / 2 - 128});
        std::string buildDate = "Build date: " + System::getBuildData();
        Text::arrange(Text::TC, buildDate.c_str());
        Text::draw(vec2i{size.x / 2, size.y / 2 - 128 + h});

        Text::arrange(Text::TC,
                      "Join our Discord for support and our GitHub for source "
                      "code access!");
        Text::draw(vec2i{size.x / 2, size.y / 2 - 128 + 3 * h});

        recti r = getSupportButtonRect();
        GuiDraw::getButton().base.draw(r);
        Text::arrange(Text::MC, "Discord");
        Text::draw(r);

        r = getGithubButtonRect();
        GuiDraw::getButton().base.draw(r);
        Text::arrange(Text::MC, "GitHub");
        Text::draw(r);

        Text::arrange(Text::TC,
                      "Current GitHub maintainers:\n"
                      "@uvcat7/TheUltravioletCatastrophe\n"
                      "@sukibaby/Jasmine\n"
                      "\n"
                      "Source code contributors:\n"
                      "@Psycast/Velocity\n"
                      "@DeltaEpsilon7787/Delta Epsilon\n"
                      "@DolpinChips/insep\n"
                      "@ScottBrenner/bren\n"
                      "@LetalexAlex\n"
                      "@SilentMystification\n"
                      "\n"
                      "Original program and many thanks to : \n"
                      "Bram 'Fietsemaker' van de Wetering\n");
        Text::draw(vec2i{size.x / 2, size.y / 2 - 128 + 7 * h});

        DrawTitleText("ABOUT", "[ESC] close", nullptr);

        auto fps = Str::fmt("%1 FPS").arg(
            1.0f / std::max(deltaTime.count(), 0.0001), 0, 0);
        Text::arrange(Text::TR, static_cast<const char*>(fps));
        Text::draw(vec2i{size.x - 4, 4});
    }

};  // TextOverlayImpl

// ================================================================================================
// InfoBox.

InfoBox::InfoBox() {
    if (gTextOverlay) {
        OVERLAY->addInfoBox(this);
    }
}

InfoBox::~InfoBox() {
    if (gTextOverlay) {
        OVERLAY->removeInfoBox(this);
    }
}

void InfoBoxWithProgress::draw(recti r) {
    Text::arrange(Text::TL, r.w, left.c_str());
    Text::draw(r);

    Text::arrange(Text::TR, r.w, right.c_str());
    Text::draw(r);
}

void InfoBoxWithProgress::setProgress(double rate) {
    int progress = std::clamp(static_cast<int>(rate * 100.0f + 0.5f), 0, 100);
    right = Str::val(progress) + '%';
}

void InfoBoxWithProgress::setTime(double seconds) {
    right = Str::formatTime(seconds, false);
}

int InfoBoxWithProgress::height() { return 16; }

// ================================================================================================
// TextOverlay API.

TextOverlay* gTextOverlay = nullptr;

void TextOverlay::create() { gTextOverlay = new TextOverlayImpl; }

void TextOverlay::destroy() {
    delete OVERLAY;
    gTextOverlay = nullptr;
}

};  // namespace Vortex
