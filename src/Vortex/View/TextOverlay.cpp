#include <Precomp.h>

#include <Vortex/View/TextOverlay.h>

#include <Core/Utils/Xmr.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/vector.h>

#include <Core/System/Debug.h>
#include <Core/System/System.h>
#include <Core/System/File.h>
#include <Core/System/Log.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Color.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>

#include <Core/Interface/UiMan.h>
#include <Core/Interface/UiStyle.h>

#include <Vortex/Common.h>
#include <Vortex/Application.h>

#include <Vortex/Shortcuts.h>

namespace AV {
	
using namespace std;
using namespace Util;

struct ProgressMessage { int id; string text; };

static constexpr int NumIcons = 16;

static uchar donateLink[] =
{
	169, 230, 230, 223, 234, 144, 158, 161, 235, 220,
	239, 111, 226, 211, 232, 231, 183, 219, 160, 215,
	212, 229, 112, 213, 217, 216, 164, 184, 216, 224,
	163, 220, 221, 163, 229, 213, 225, 182, 185, 220,
	214, 177, 196, 235, 110, 234, 213, 219, 224, 185,
	218, 152, 220, 212, 235, 181, 215, 214, 206, 217,
	203, 227, 230, 227, 211, 215, 170, 214, 175, 195,
	199, 154, 176, 171, 173, 179, 205, 152, 197, 181,
	193, 188, 0
};

// =====================================================================================================================
// TextOverlay :: member data.

struct TextOverlayUi : public UiElement
{
	void onKeyPress(KeyPress& input) override;
	void onMouseScroll(MouseScroll& input) override;

	void tick(bool enabled) override;
	void draw(bool enabled) override;
};

namespace TextOverlay
{
	struct State
	{
		vector<string> log;
		string debugLog;
	
		int scrollPos = 0;
		int scrollEnd = 0;
		int pageSize = 0;
	
		TextOverlay::Mode mode = TextOverlay::Mode::None;
	};
	static State* state = nullptr;
}
using TextOverlay::state;

void TextOverlay::initialize(const XmrDoc& settings)
{
	state = new State();
	UiMan::add(make_unique<TextOverlayUi>());
}

void TextOverlay::deinitialize()
{
	Util::reset(state);
}

using namespace TextOverlay;

// =====================================================================================================================
// TextOverlay :: drawing.

static void DrawTitleText(const char* left, const char* mid, const char* right)
{
	Size2 size = Window::getSize();

	Draw::fill(Rect(0, 0, size.w, 28), Color(0, 0, 0, 191));
	Draw::fill(Rect(0, 28, size.w, 29), Color::White);

	Text::setStyle(UiText::regular);
	if (right)
	{
		Text::format(TextAlign::TR, right);
		Text::draw(size.w - 4, 4);
	}
	if (mid)
	{
		Text::format(TextAlign::TC, mid);
		Text::draw(size.w / 2, 4);
	}
	if (left)
	{
		Text::format(TextAlign::TL, left);
		Text::draw(4, 4);
	}
}

void DrawScrollbar()
{
	if (state->pageSize > 0 && state->scrollEnd > 0)
	{
		Size2 size = Window::getSize();
		Rect box = { size.w - 16, 32, size.w - 4, size.h - 4 };

		int barY = box.t + box.h() * state->scrollPos / (state->scrollEnd + state->pageSize);
		int barH = box.h() * state->pageSize / (state->scrollEnd + state->pageSize);

		Draw::outline(box, Color(255, 255, 255, 128));
		Draw::fill(box, Color(255, 255, 255, 64));
		Draw::fill(Rect(box.l, barY, box.r, barY + barH), Color(255, 255, 255, 128));
	}
}

// =====================================================================================================================
// TextOverlay :: message log.

static void DrawMessageLog()
{
	Size2 size = Window::getSize();

	auto textStyle = UiText::regular;
	textStyle.flags = TextOptions::Markup;

	// Messages.
	int x = 4, y = 32, bottomY = size.h;
	for (int i = state->scrollPos, n = (int)state->log.size(); i < n && y < bottomY - 16; ++i)
	{
		auto& m = state->log[i];
		Text::setStyle(textStyle);
		Text::format(TextAlign::TL, m.data());
		Text::draw(x, y);
		y += Text::getHeight() + 2;
	}

	DrawTitleText("MESSAGE LOG", "[ESC/F2] close", "[Delete] clear");
	DrawScrollbar();
}

// =====================================================================================================================
// TextOverlay :: debug log.

static void TickDebugLog()
{
}

static void DrawDebugLog()
{
	Size2 size = Window::getSize();

	Text::setStyle(UiText::regular);
	Text::setFontSize(11);
	Text::format(TextAlign::TL, state->debugLog.data());
	Text::draw(4, 32);

	DrawTitleText("DEBUG LOG", "[ESC] close", "");
	DrawScrollbar();
}

// =====================================================================================================================
// TextOverlay :: shortcuts.

static void TickShortcuts()
{
}

static void DrawShortcuts()
{
	Text::setStyle(UiText::regular);
	int x = Window::getSize().w / 2 - 250, w = 500;
	int y = 32 - state->scrollPos;
	for (auto& group : Shortcuts::getShortcuts())
	{
		Text::format(TextAlign::TL, group.name.data());
		Text::draw(x, y);
		Draw::fill(Rect(x, y + 18, x + w, y + 19), Color::White);
		y += 24;

		for (auto& shortcut : group.shortcuts)
		{
			Text::format(TextAlign::TR, shortcut.name.data());
			Text::draw(x + w / 2 - 10, y);

			auto notation = shortcut.key.notation();
			Text::format(TextAlign::TL, notation);
			Text::draw(x + w / 2 + 10, y);

			y += 18;
		}
	}

	DrawTitleText("SHORTCUTS", "[ESC/F1] close", nullptr);
	DrawScrollbar();
}

// =====================================================================================================================
// TextOverlay :: about.

static void TickAbout()
{
}

static void DrawAbout()
{
	Size2 size = Window::getSize();

	auto textStyle = UiText::regular;

	Text::setStyle(textStyle);
	Text::format(TextAlign::BC, "ArrowVortex (beta)");
	Text::draw(size.w / 2, size.h / 2 - 2);

	Text::format(TextAlign::TC, "(C) Bram 'Fietsemaker' van de Wetering");
	Text::draw(size.w / 2, size.h / 2 + 2);

	string buildDate = "Build :: " + System::getBuildDate();
	Text::format(TextAlign::TC, buildDate.data());
	Text::draw(size.w / 2, size.h / 2 + 64);

	DrawTitleText("ABOUT", "[ESC] close", nullptr);

	auto fps = format("{:.0f} FPS", 1.0f / max((float)Window::getDeltaTime(), 0.0001f));
	Text::format(TextAlign::TR, fps.data());
	Text::draw(size.w - 4, 4);
}

// =====================================================================================================================
// TextOverlay :: event handling.

static void UpdateScrollValues()
{
	Size2 size = Window::getSize();

	if (state->mode == TextOverlay::Mode::MessageLog)
	{
		state->pageSize = max(0, (size.h - 32) / 16);
		state->scrollEnd = max(0, (int)state->log.size() - state->pageSize);
	}
	else if (state->mode == TextOverlay::Mode::Shortcuts)
	{
		state->pageSize = size.h;
		state->scrollEnd = 64;
		for (auto& group : Shortcuts::getShortcuts())
			state->scrollEnd += 24 + (int)group.shortcuts.size() * 18;
		state->scrollEnd = max(0, state->scrollEnd - state->pageSize);
	}
	else
	{
		state->pageSize = size.h;
		state->scrollEnd = 0;
	}

	state->scrollPos = min(max(0, state->scrollPos), state->scrollEnd);
}

void TextOverlayUi::onKeyPress(KeyPress& input)
{
	if (input.handled()) return;

	if (input.key.code == KeyCode::Delete && state->mode == TextOverlay::Mode::MessageLog)
	{
		state->log.clear();
		input.setHandled();
	}
	if (input.key.code == KeyCode::Escape && state->mode != TextOverlay::Mode::None)
	{
		state->mode = TextOverlay::Mode::None;
		input.setHandled();
	}
}

void TextOverlayUi::onMouseScroll(MouseScroll& input)
{
	if (state->mode != TextOverlay::Mode::None && input.unhandled())
	{
		int delta = input.isUp ? -1 : +1;
		state->scrollPos += delta * max(1, state->pageSize / 10);
		state->scrollPos = min(max(0, state->scrollPos), state->scrollEnd);
		input.setHandled();
	}
}

// =====================================================================================================================
// TextOverlay :: constructor and destructor.

void TextOverlayUi::tick(bool enabled)
{
	UpdateScrollValues();
	switch (state->mode)
	{
	case TextOverlay::Mode::None:
		break;
	case TextOverlay::Mode::DebugLog:
		TickDebugLog(); break;
	case TextOverlay::Mode::Shortcuts:
		TickShortcuts(); break;
	case TextOverlay::Mode::About:
		TickAbout(); break;
	};
}

// =====================================================================================================================
// TextOverlay :: member functions.

void TextOverlay::show(Mode mode)
{
	if (state->mode == mode)
	{
		state->mode = Mode::None;
	}
	else
	{
		state->mode = mode;
	}

	state->debugLog.clear();

	if (state->mode == Mode::DebugLog)
	{
		if (FileSystem::readText("ArrowVortex.log", state->debugLog))
		{
			state->debugLog.erase(0, 3); // UTF-8 BOM.
		}
		else
		{
			state->debugLog = "(could not open ArrowVortex.log)";
		}
	}

	UpdateScrollValues();
}

bool TextOverlay::isOpen()
{
	return state->mode != Mode::None;
}

void TextOverlayUi::draw(bool enabled)
{
	if (state->mode != TextOverlay::Mode::None)
	{
		Size2 size = Window::getSize();
		Draw::fill(Rect(0, 0, size.w, size.h), Color(0, 0, 0, 191));
	}
	switch(state->mode)
	{
	case TextOverlay::Mode::None:
		break;
	case TextOverlay::Mode::MessageLog:
		DrawMessageLog(); break;
	case TextOverlay::Mode::DebugLog:
		DrawDebugLog(); break;
	case TextOverlay::Mode::Shortcuts:
		DrawShortcuts(); break;
	case TextOverlay::Mode::About:
		DrawAbout(); break;
	};
}

} // namespace AV
