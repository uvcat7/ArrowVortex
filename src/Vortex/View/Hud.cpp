#include <Precomp.h>

#include <stdarg.h>

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
#include <Vortex/View/Hud.h>

namespace AV {
	
using namespace std;
using namespace Util;

enum class HudMessageType { Note, Info, Warning, Error };

static constexpr size_t HudBufferSize = 512;

struct HudEntry
{
	HudEntry(string&& text, HudMessageType type, float timeLeft)
		: text(text), type(type), timeLeft(timeLeft), fade(false) {}

	string text;
	HudMessageType type;
	float timeLeft;
	bool fade = false;
};

struct ProgressMessage { int id; string text; };

// =====================================================================================================================
// Hud :: member data.

struct HudUi : public UiElement
{
	void tick(bool enabled) override;
	void draw(bool enabled) override;
};

namespace Hud
{
	struct State
	{
		vector<HudEntry> hud;
		vector<weak_ptr<InfoBox>> infoBoxes;
		vector<string> log;
		string debugLog;
	};
	static State* state = nullptr;
}
using Hud::state;

// =====================================================================================================================
// Hud :: constructor and destructor.

void Hud::initialize(const XmrDoc& settings)
{
	state = new State();
	UiMan::add(make_unique<HudUi>());
}

void Hud::deinitialize()
{
	Util::reset(state);
}

void Hud::show(const shared_ptr<InfoBox>& infoBox)
{
	state->infoBoxes.emplace_back(infoBox);
}

void HudUi::tick(bool enabled)
{
	auto& hud = state->hud;

	if (hud.size() > 100)
		hud.erase(hud.begin(), hud.end() - 100);

	for (auto i = (int)hud.size() - 1; i >= 0; --i)
	{
		float delta = (float)Window::getDeltaTime();

		hud[i].timeLeft -= delta;

		if (hud[i].timeLeft < 0)
			hud.erase(hud.begin() + i);
	}

	erase_if(state->infoBoxes, [](const auto& i) { return i.expired(); });
}

void HudUi::draw(bool enabled)
{
	auto textStyle = UiText::regular;
	textStyle.flags = TextOptions::Markup;

	Size2 viewSize = Window::getSize();
	Rect view(0, 0, viewSize.w, viewSize.h);
	int x = viewSize.w / 2, y = 24;

	for (auto& boxPtr : state->infoBoxes)
	{
		auto box = boxPtr.lock();
		if (!box) continue;

		Size2 size = { 280, box->height() };
		Rect r(Vec2(x, y), size);
		Rect r2 = r.expand(4);

		Draw::fill(r2, Color(0, 0, 0, 128));
		Draw::outline(r2, Color(128, 128, 128, 128));

		box->draw(r);

		y += size.h + 12;
	}

	y = 4;
	for (auto& m : reverseIterate(state->hud))
	{
		auto range = m.fade ? 125 : 250;
		auto alpha = clamp(int(m.timeLeft * 2 * range), 0, range);

		textStyle.textColor = Color(textStyle.textColor, alpha);
		textStyle.shadowColor = Color(textStyle.shadowColor, alpha);

		Text::setStyle(textStyle);
		Text::format(TextAlign::TL, m.text.data());
		Text::draw(4, y);
		y += Text::getHeight() + 2;
	}
}

static void AddHudMessage(HudMessageType type, float timeLeft, const char* fmt, va_list args)
{
	char buffer[HudBufferSize];
	auto n = size_t(vsnprintf(buffer, HudBufferSize - 1, fmt, args));
	if (n > HudBufferSize - 1) n = HudBufferSize - 1;
	buffer[n] = 0;
	string msg;
	switch (type)
	{
	case HudMessageType::Warning:
		msg.assign("{tc:FF0}WARNING:{tc} ");
		msg.append(buffer);
		break;
	case HudMessageType::Error:
		msg.assign("{tc:F44}ERROR:{tc} ");
		msg.append(buffer);
		break;
	default:
		msg.assign(buffer);
		break;
	}
	state->hud.emplace_back(move(msg), type, timeLeft);
}

void Hud::note(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	AddHudMessage(HudMessageType::Note, 1.0f, fmt, args);
	va_end(args);
}

void Hud::info(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	AddHudMessage(HudMessageType::Info, 3.0f, fmt, args);
	va_end(args);
}

void Hud::warning(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	AddHudMessage(HudMessageType::Warning, 6.0f, fmt, args);
	va_end(args);
}

void Hud::error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	AddHudMessage(HudMessageType::Error, 6.0f, fmt, args);
	va_end(args);
}

// =====================================================================================================================
// InfoBox.

InfoBox::~InfoBox()
{
}

void InfoBoxWithProgress::draw(Rect r)
{
	Text::setStyle(UiText::regular);
	Text::format(TextAlign::TL, r.w(), left.data());
	Text::draw(r);

	Text::format(TextAlign::TR, r.w(), right.data());
	Text::draw(r);
}

void InfoBoxWithProgress::setProgress(double rate)
{
	int progress = clamp(int(rate * 100.0f + 0.5f), 0, 100);
	right = String::fromInt(progress) + '%';
}

void InfoBoxWithProgress::setTime(double seconds)
{
	right = String::formatTime(seconds, false);
}

int InfoBoxWithProgress::height()
{
	return 16;
}

} // namespace AV
