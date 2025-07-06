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

namespace Vortex {

namespace {

#define OVERLAY ((TextOverlayImpl*)gTextOverlay) 

struct HudEntry { String text; TextOverlay::MessageType type; float timeLeft; };

struct ProgressMessage { int id; String text; };

struct Shortcut { String a, b; bool isHeader; };

static const int NUM_ICONS = 16;

static uchar supportLink[] = "https://discord.gg/GCNAyDmjqy";

static const char* iconNames[NUM_ICONS] = {
	"up one",
	"up half",
	"down one",
	"down half",
	"halve",
	"double",
	"full selection",
	"half selection",
	"undo",
	"redo",
	"calculate",
	"tweak",
	"search",
	"copy",
	"play",
	"arrow right",
};

}; // anonymous namespace

// ================================================================================================
// TextOverlayImpl :: member data.

struct TextOverlayImpl : public TextOverlay {

Vector<HudEntry> myHud;
Vector<InfoBox*> myInfoBoxes;
Vector<String> myLog;
Vector<Shortcut> myShortcuts;
String myDebugLog;

int myScrollPos, myScrollEnd, myPageSize;
Mode myMode;

// ================================================================================================
// TextOverlayImpl :: constructor and destructor.

~TextOverlayImpl()
{
}

TextOverlayImpl()
{
	myMode = HUD;

	myScrollPos = 0;
	myScrollEnd = 0;
	myPageSize = 0;

	Texture icons[NUM_ICONS];
	Texture::createTiles("assets/icons text.png", 16, 16, NUM_ICONS, icons, false, Texture::ALPHA);
	for(int i = 0; i < NUM_ICONS; ++i)
	{
		Text::createGlyph(iconNames[i], icons[i], 0, 3);
	}
}

// ================================================================================================
// TextOverlayImpl :: shortcuts.

void addShortcutHeader(const char* name)
{
	Shortcut& shortcut = myShortcuts.append();
	shortcut.isHeader = true;
	shortcut.a = name;
}

void addShortcut(const char* name, const char* keys)
{
	Shortcut& shortcut = myShortcuts.append();
	shortcut.isHeader = false;
	shortcut.a = name;
	shortcut.b = keys;
}

void addShortcut(Action::Type action, const char* name)
{
	String notation = gShortcuts->getNotation(action);
	if(notation.len()) addShortcut(name, notation.str());
}

void LoadShortcuts()
{
	if(!myShortcuts.empty()) return;

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
	addShortcut(Action::SELECT_REGION_BEFORE_CURSOR, "Select region before cursor");
	addShortcut(Action::SELECT_REGION_AFTER_CURSOR, "Select region after cursor");

	addShortcutHeader("Visual sync");
	addShortcut(Action::SET_VISUAL_SYNC_CURSOR_ANCHOR, "Set to target nearby row of snap");
	addShortcut(Action::SET_VISUAL_SYNC_RECEPTOR_ANCHOR, "Set to target receptor's row");
	addShortcut(Action::SHIFT_ROW_NONDESTRUCTIVE, "Shift to cursor, non-destructive");
	addShortcut(Action::SHIFT_ROW_DESTRUCTIVE, "Shift to cursor, destructive");
	addShortcut(Action::INJECT_BOUNDING_BPM_CHANGE, "Add bounding BPM change");

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
}

// ================================================================================================
// TextOverlayImpl :: member functions.

void UpdateScrollValues()
{
	vec2i size = gSystem->getWindowSize();

	if(myMode == MESSAGE_LOG)
	{
		myPageSize = max(0, (size.y - 32) / 16);
		myScrollEnd = max(0, myLog.size() - myPageSize);
	}
	else if(myMode == SHORTCUTS)
	{
		myPageSize = size.y;
		myScrollEnd = 32;
		for(const Shortcut& e : myShortcuts)
		{
			myScrollEnd += e.isHeader ? 24 : 18;
		}
		myScrollEnd = max(0, myScrollEnd - myPageSize);
	}
	else
	{
		myPageSize = size.y;
		myScrollEnd = 0;
	}

	myScrollPos = min(max(0, myScrollPos), myScrollEnd);
}

void tick()
{
	UpdateScrollValues();
	switch(myMode)
	{
	case HUD:
		tickHud(); break;
	case MESSAGE_LOG:
		tickMessageLog(); break;
	case DEBUG_LOG:
		tickDebugLog(); break;
	case SHORTCUTS:
		tickShortcuts(); break;
	case ABOUT:
		tickAbout(); break;
	};
}

void onKeyPress(KeyPress& evt) override
{
	if(myMode == MESSAGE_LOG && evt.key == Key::DELETE && !evt.handled)
	{
		myLog.release();
		evt.handled = true;
	}
	if(myMode != HUD && evt.key == Key::ESCAPE && !evt.handled)
	{
		myMode = HUD;
		evt.handled = true;
	}
}

void onMouseScroll(MouseScroll& evt) override
{
	if(myMode != HUD && !evt.handled)
	{
		int delta = evt.up ? -1 : +1;
		myScrollPos += delta * max(1, myPageSize / 10);
		myScrollPos = min(max(0, myScrollPos), myScrollEnd);
		evt.handled = true;
	}
}

void show(Mode mode)
{
	if(myMode == mode)
	{
		myMode = HUD;
	}
	else
	{
		myMode = mode;
	}

	myDebugLog.clear();

	if(myMode == SHORTCUTS)
	{
		LoadShortcuts();
	}
	else if(myMode == DEBUG_LOG)
	{
		bool success;
		myDebugLog = File::getText("ArrowVortex.log", &success);
		if(success)
		{
			Str::erase(myDebugLog, 0, 3); // UTF-8 BOM.
		}
		else
		{
			myDebugLog = "(could not open ArrowVortex.log)";
		}
	}

	UpdateScrollValues();
}

bool isOpen()
{
	return myMode != HUD;
}

void DrawTitleText(const char* left, const char* mid, const char* right)
{
	vec2i size = gSystem->getWindowSize();

	Draw::fill({0, 0, size.x, 28}, COLOR32(0, 0, 0, 191));
	Draw::fill({0, 28, size.x, 1}, Colors::white);

	if(right)
	{
		Text::arrange(Text::TR, right);
		Text::draw(vec2i{size.x - 4, 4});
	}
	if(mid)
	{
		Text::arrange(Text::TC, mid);
		Text::draw(vec2i{size.x / 2, 4});
	}
	if(left)
	{
		Text::arrange(Text::TL, left);
		Text::draw(vec2i{4, 4});
	}
}

void DrawScrollbar()
{
	if(myPageSize > 0 && myScrollEnd > 0)
	{
		vec2i size = gSystem->getWindowSize();
		recti box = {size.x - 16, 32, 12, size.y - 36};

		int barY = box.y + box.h * myScrollPos / (myScrollEnd + myPageSize);
		int barH = box.h * myPageSize / (myScrollEnd + myPageSize);

		Draw::outline(box, COLOR32(255, 255, 255, 128));
		Draw::fill(box, COLOR32(255, 255, 255, 64));
		Draw::fill({box.x, barY, box.w, barH}, COLOR32(255, 255, 255, 128));
	}
}

void draw()
{
	if(myMode != HUD)
	{
		vec2i size = gSystem->getWindowSize();
		Draw::fill({0, 0, size.x, size.y}, COLOR32(0, 0, 0, 191));
	}
	switch(myMode)
	{
	case HUD:
		drawHud(); break;
	case MESSAGE_LOG:
		drawMessageLog(); break;
	case DEBUG_LOG:
		drawDebugLog(); break;
	case SHORTCUTS:
		drawShortcuts(); break;
	case ABOUT:
		drawAbout(); break;
	};
}

void addMessage(const char* str, MessageType type)
{
	String msg = str;
	if(type == NOTE)
	{
		myHud.push_back({msg, type, 0.5f});
	}
	else if(type == INFO)
	{
		myLog.push_back(msg);
		myHud.push_back({msg, type, 3.0f});
	}
	else if(type == WARNING)
	{
		Str::insert(msg, 0, "{tc:FF0}WARNING:{tc} ");
		myLog.push_back(msg);
		myHud.push_back({msg, type, 6.0f});
	}
	else if(type == ERROR)
	{
		Str::insert(msg, 0, "{tc:F44}ERROR:{tc} ");
		myLog.push_back(msg);
		myHud.push_back({msg, type, 6.0f});
	}
}

void addInfoBox(InfoBox* box)
{
	myInfoBoxes.push_back(box);
}

void removeInfoBox(InfoBox* box)
{
	myInfoBoxes.erase_values(box);
}

// ================================================================================================
// TextOverlayImpl :: hud.

void tickHud()
{
	for(int i = myHud.size() - 1; i >= 0; --i)
	{
		float fade = 0.75f * (myHud.size() - 1 - i);
		if(myHud[i].type == NOTE)
		{
			fade += 0.25;
		}
		else
		{
			fade += 0.5f;
		}
		float delta = clamp(deltaTime * fade, 0.0f, 1.0f);

		myHud[i].timeLeft -= delta;
		if(myHud[i].timeLeft <= -0.5f) myHud.erase(i);
	}
}

void drawHud()
{
	TextStyle textStyle;
	textStyle.textFlags = Text::MARKUP;

	vec2i view = gSystem->getWindowSize();
	int x = view.x / 2, y = 8;
	for(auto& box : myInfoBoxes)
	{
		vec2i size = {280, box->height()};
		recti r = {x - size.x / 2, y, size.x, size.y};
		recti r2 = {r.x - 4, r.y - 4, r.w + 8, r.h + 8};

		Draw::fill(r2, COLOR32(0, 0, 0, 128));
		Draw::outline(r2, COLOR32(128, 128, 128, 128));

		box->draw(r);

		y += size.y + 12;
	}

	x = 4, y = 4;
	for(auto& m : myHud)
	{
		int a = clamp((int)(m.timeLeft * 512.0f + 256.0f), 0, 255);

		textStyle.textColor = Color32a(textStyle.textColor, a);
		textStyle.shadowColor = Color32a(textStyle.shadowColor, a);

		Text::arrange(Text::TL, textStyle, m.text.str());
		Text::draw(recti{x, y, view.x - 8, view.y - y});

		y += Text::getSize().y + 2;
	}

	// Speed up the message removal if we have too many.
	if(y > view.y) myHud.erase(0);
}

// ================================================================================================
// TextOverlayImpl :: message log.

void tickMessageLog()
{
}

void drawMessageLog()
{
	vec2i size = gSystem->getWindowSize();

	TextStyle textStyle;
	textStyle.textFlags = Text::MARKUP;

	// Messages.
	int x = 4, y = 32, bottomY = size.y;
	for(int i = myScrollPos, n = myLog.size(); i < n && y < bottomY - 16; ++i)
	{
		auto& m = myLog[i];
		Text::arrange(Text::TL, textStyle, m.str());
		Text::draw(recti{x, y, size.x - 8, size.y});
		y += Text::getSize().y + 2;
	}

	DrawTitleText("MESSAGE LOG", "[ESC/F2] close", "[Delete] clear");
	DrawScrollbar();
}

// ================================================================================================
// TextOverlayImpl :: debug log.

void tickDebugLog()
{
}

void drawDebugLog()
{
	vec2i size = gSystem->getWindowSize();

	TextStyle textStyle;
	textStyle.fontSize = 11;
	Text::arrange(Text::TL, textStyle, myDebugLog.str());
	Text::draw(recti{4, 32, size.x - 8, size.y});

	DrawTitleText("DEBUG LOG", "[ESC] close", "");
	DrawScrollbar();
}

// ================================================================================================
// TextOverlayImpl :: shortcuts.

void tickShortcuts()
{
}

void drawShortcuts()
{
	int x = gSystem->getWindowSize().x / 2 - 250, w = 500;
	int y = 32 - myScrollPos;
	for(const Shortcut& e : myShortcuts)
	{
		if(e.isHeader)
		{
			Text::arrange(Text::TL, e.a.str());
			Text::draw(vec2i{x, y});
			Draw::fill({x, y + 18, w, 1}, Colors::white);
			y += 24;
		}
		else
		{
			Text::arrange(Text::TR, e.a.str());
			Text::draw(vec2i{x + w / 2 - 10, y});
			Text::arrange(Text::TL, e.b.str());
			Text::draw(vec2i{x + w / 2 + 10, y});

			y += 18;
		}
	}

	DrawTitleText("SHORTCUTS", "[ESC/F1] close", nullptr);
	DrawScrollbar();
}

// ================================================================================================
// TextOverlayImpl :: about.

void tickAbout()
{
	vec2i mpos = gSystem->getMousePos();
	if (IsInside(getSupportButtonRect(), mpos.x, mpos.y))
	{
		gSystem->setCursor(Cursor::HAND);
		MousePress* mp = nullptr;
		if (gSystem->getEvents().next(mp))
		{
			if (mp->unhandled())
			{
				mp->setHandled();
				gSystem->openWebpage((const char*)supportLink);
			}
		}
	}
}

recti getSupportButtonRect()
{
	vec2i w = gSystem->getWindowSize();
	return { w.x / 2 - 24, w.y / 2 - 60, 64, 28 };
}

void drawAbout()
{
	vec2i size = gSystem->getWindowSize();

	Text::arrange(Text::BC, "ArrowVortex release 1.0.0 (beta)");
	Text::draw(vec2i{size.x / 2, size.y / 2 - 128});
	String buildDate = "Build date: " + System::getBuildData();
	Text::arrange(Text::TC, buildDate.str());
	Text::draw(vec2i{ size.x / 2, size.y / 2 - 112 });

	Text::arrange(Text::TC, "Join our Discord for support and source code access!");
	Text::draw(vec2i{ size.x / 2, size.y / 2 - 80 });

	recti r = getSupportButtonRect();
	GuiDraw::getButton().base.draw(r);
	Text::arrange(Text::MC, "Discord");
	Text::draw(r);

	Text::arrange(Text::TC, "Current GitHub maintainers:\n"
		"@uvcat/TheUltravioletCatastrophe\n"
		"@sukibaby/Jasmine\n"
		"\n"
		"Source code contributors:\n"
		"@Psycast/Velocity\n"
		"@DeltaEpsilon7787/Delta Epsilon\n"
	    "@DolpinChips/insep\n"
		"\n"
		"Original program and many thanks to : \n"
		"Bram 'Fietsemaker' van de Wetering\n");
	Text::draw(vec2i{ size.x / 2, size.y / 2 - 16 });

	DrawTitleText("ABOUT", "[ESC] close", nullptr);

	auto fps = Str::fmt("%1 FPS").arg(1.0f / max(deltaTime, 0.0001f), 0, 0);
	Text::arrange(Text::TR, fps);
	Text::draw(vec2i{size.x - 4, 4});
}

}; // TextOverlayImpl

// ================================================================================================
// InfoBox.

InfoBox::InfoBox()
{
	if(gTextOverlay)
	{
		OVERLAY->addInfoBox(this);
	}
}

InfoBox::~InfoBox()
{
	if(gTextOverlay)
	{
		OVERLAY->removeInfoBox(this);
	}
}

void InfoBoxWithProgress::draw(recti r)
{
	Text::arrange(Text::TL, r.w, left.str());
	Text::draw(r);

	Text::arrange(Text::TR, r.w, right.str());
	Text::draw(r);
}

void InfoBoxWithProgress::setProgress(double rate)
{
	int progress = clamp((int)(rate * 100.0f + 0.5f), 0, 100);
	right = Str::val(progress) + '%';
}

void InfoBoxWithProgress::setTime(double seconds)
{
	right = Str::formatTime(seconds, false);
}

int InfoBoxWithProgress::height()
{
	return 16;
}

// ================================================================================================
// TextOverlay API. 

TextOverlay* gTextOverlay = nullptr;

void TextOverlay::create()
{
	gTextOverlay = new TextOverlayImpl;
}

void TextOverlay::destroy()
{
	delete OVERLAY;
	gTextOverlay = nullptr;
}

}; // namespace Vortex
