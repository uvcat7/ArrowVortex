#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/vector.h>
#include <Core/Utils/Map.h>

#include <Core/System/Debug.h>
#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Texture.h>
#include <Core/Graphics/Renderer.h>
#include <Core/Graphics/FontManager.h>
#include <Core/Graphics/Shader.h>

#include <Core/Interface/UiMan.h>
#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Dialog.h>
#include <Core/Interface/DialogFrame.h>

#include <Core/Interface/Widgets/Seperator.h>
#include <Core/Interface/Widgets/Label.h>
#include <Core/Interface/Widgets/Button.h>
#include <Core/Interface/Widgets/Checkbox.h>
#include <Core/Interface/Widgets/Slider.h>
#include <Core/Interface/Widgets/Scrollbar.h>
#include <Core/Interface/Widgets/SelectList.h>
#include <Core/Interface/Widgets/Droplist.h>
#include <Core/Interface/Widgets/LineEdit.h>
#include <Core/Interface/Widgets/Spinner.h>

namespace AV {

using namespace std;
using namespace Util;

namespace UiMan
{
	struct State
	{
		string tooltipPreviousText;
		string tooltipText;
		Vec2 tooltipPos;
		float tooltipTimer = 0.0;
		
		vector<unique_ptr<UiElement>> elements;
	
		UiElement* mouseOver = nullptr;
		UiElement* mouseCapture = nullptr;
		UiElement* textCapture = nullptr;
	
		unordered_map<const UiElement*, string> tooltipMap;
	};
	static State* state = nullptr;
}
using UiMan::state;

static bool ContainsElement(const vector<UiElement*>& vec, const UiElement* elem)
{
	for (auto it : vec)
	{
		if (it == elem)
			return true;
	}
	return false;
}

static void EraseElement(vector<UiElement*>& vec, UiElement* elem)
{
	auto it = vec.begin();
	while (it != vec.end())
	{
		if (*it == elem)
			it = vec.erase(it);
		else
			++it;
	}
}

// =====================================================================================================================
// Initialization.

void UiMan::initialize()
{
	UiStyle::create();

	state = new State();

	state->tooltipPos = { 0, 0 };
	state->tooltipTimer = 0.0f;

	state->mouseOver = nullptr;
	state->mouseCapture = nullptr;
	state->textCapture = nullptr;
}

void UiMan::deinitialize()
{
	UiStyle::destroy();

	state->elements.clear();

	Util::reset(state);
}

// =====================================================================================================================
// Updating UI elements.

static void UpdateTooltip();

void UiMan::add(unique_ptr<UiElement>&& element)
{
	state->elements.emplace_back(move(element));
}

void UiMan::send(UiEvent& uiEvent)
{
	for (auto& elem : state->elements)
		elem->handleEvent(uiEvent);
}

static UiElement* FindMouseOverElement()
{
	if (state->mouseCapture)
		return state->mouseCapture;

	auto mpos = Window::getMousePos();
	for (auto& elem : state->elements)
	{
		if (auto mouseOver = elem->findMouseOver(mpos.x, mpos.y))
			return mouseOver;
	}
	return nullptr;
};

void UiMan::tick()
{
	state->tooltipText.clear();
	state->mouseOver = FindMouseOverElement();

	for (auto& elem : reverseIterate(state->elements))
		elem->tick(true);

	UpdateTooltip();
}

static void DrawTooltip();

void UiMan::draw()
{
	for (auto& elem : state->elements)
		elem->draw(true);

	DrawTooltip();
}

// =====================================================================================================================
// Gui :: widgets.

void UiMan::registerElement(UiElement* elem)
{
}

void UiMan::unregisterElement(UiElement* elem)
{
	if (state->mouseOver == elem)
		state->mouseOver = nullptr;

	if (state->mouseCapture == elem)
		state->mouseCapture = nullptr;

	if (state->textCapture == elem)
		state->textCapture = nullptr;

	state->tooltipMap.erase(elem);
}

// =====================================================================================================================
// Gui :: tooltip.

void UiMan::setTooltip(string text)
{
	state->tooltipText = text;
}

string UiMan::getTooltip()
{
	return state->tooltipText;
}

static void UpdateTooltip()
{
	float deltaTime = (float)Window::getDeltaTime();
	Vec2 mousePos = Window::getMousePos();
	Size2 viewSize = Window::getSize();

	if (state->tooltipPreviousText == state->tooltipText)
	{
		state->tooltipTimer += deltaTime;
	}
	else
	{
		state->tooltipPreviousText = state->tooltipText;
		state->tooltipPos = { mousePos.x + 2, 0 };
		if (mousePos.y > viewSize.h - 24)
		{
			state->tooltipPos.y = mousePos.y - 18;
		}
		else
		{
			state->tooltipPos.y = mousePos.y + 24;
		}
		state->tooltipTimer = 0.0f;
	}
}

static void DrawTooltip()
{
	if (state->tooltipText.length() && state->tooltipTimer > 1.0f)
	{
		Size2 viewSize = Window::getSize();

		int alpha = clamp(int(state->tooltipTimer * 1000 - 1000), 0, 255);

		TextStyle textStyle = UiText::regular;
		textStyle.textColor = Color(0, alpha);
		textStyle.shadowColor = Color::Blank;
		textStyle.fontSize = 11;

		Text::setStyle(textStyle);
		Text::format(TextAlign::TL, 256, state->tooltipText.data());

		int width = Text::getWidth();
		int height = Text::getHeight();

		Vec2 pos = state->tooltipPos;
		pos.x = clamp(pos.x, 4, viewSize.w - width - 4);
		pos.y = clamp(pos.y, 4, viewSize.h - height - 4);

		Rect textBox(pos.x, pos.y, pos.x + width, pos.y + height);
		Rect box = textBox.expand(3);

		Draw::fill(box, Color(191, alpha));
		Draw::outline(box, Color(230, alpha));

		Text::draw(textBox);
	}
}

void UiMan::setTooltip(const UiElement* elem, stringref text)
{
	state->tooltipMap[elem] = text;
}

string UiMan::getTooltip(const UiElement* elem)
{
	auto it = state->tooltipMap.find(elem);
	return it != state->tooltipMap.end() ? it->second : string();
}

// =====================================================================================================================
// Gui :: inputs.

bool UiMan::isCapturingMouse()
{
	return bool(state->mouseCapture) || bool(state->mouseOver);
}

bool UiMan::isCapturingText()
{
	return bool(state->textCapture);
}

void UiMan::startCapturingMouse(UiElement* elem)
{
	VortexAssert(elem != nullptr);

	if (state->mouseCapture == elem)
		return;

	if (state->textCapture && state->textCapture != elem)
	{
		auto previous = state->textCapture;
		state->textCapture = nullptr;
		previous->onTextCaptureLost();
	}

	auto previous = state->mouseCapture;
	state->mouseCapture = elem;
	if (previous)
		previous->onMouseCaptureLost();
}

void UiMan::captureText(UiElement* elem)
{
	VortexAssert(elem != nullptr);

	if (state->textCapture == elem)
		return;

	auto previous = state->textCapture;
	state->textCapture = elem;
	if (previous)
		previous->onTextCaptureLost();
}

void UiMan::stopCapturingMouse(UiElement* elem)
{
	VortexAssert(elem != nullptr);
	if (state->mouseCapture == elem)
	{
		state->mouseCapture = nullptr;
		elem->onMouseCaptureLost();
	}
}

void UiMan::stopCapturingText(UiElement* elem)
{
	VortexAssert(elem != nullptr);
	if (state->textCapture == elem)
	{
		state->textCapture = nullptr;
		elem->onTextCaptureLost();
	}
}

UiElement* UiMan::getMouseOver()
{
	return state->mouseOver;
}

UiElement* UiMan::getMouseCapture()
{
	return state->mouseCapture;
}

UiElement* UiMan::getTextCapture()
{
	return state->textCapture;
}

} // namespace AV