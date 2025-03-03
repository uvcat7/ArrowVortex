#include <Precomp.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/Flag.h>

#include <Core/System/Debug.h>
#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Text.h>

#include <Core/Interface/UiStyle.h>
#include <Core/Interface/Widgets/VerticalTabs.h>

namespace AV {

using namespace std;
using namespace Util;

static constexpr int TitleH = 16;
static constexpr int FMax = 1024;
static constexpr int ButtonW = 12;
static constexpr int SeperatorH = 2;

enum class TabFlag
{
	None          = 0,
	IsVisible     = 1 << 0,
	IsExpanded    = 1 << 1,
	IsStretching  = 1 << 2,
	IsCollapsable = 1 << 3,
};

template <>
constexpr bool IsFlags<TabFlag> = true;

// =====================================================================================================================
// Tab :: Implementation.

#define SELF ((TabImpl*)this)

struct WVerticalTabs::TabImpl : public WVerticalTabs::Tab
{
	int offset;
	int height;
	int minHeight;
	int stretch;
	TabFlag flags = TabFlag::None;
	string description;
	shared_ptr<Widget> widget;
};

void WVerticalTabs::Tab::setVisible(bool state)
{
	Flag::set(SELF->flags, TabFlag::IsVisible, state);
}

void WVerticalTabs::Tab::setExpanded(bool state)
{
	Flag::set(SELF->flags, TabFlag::IsExpanded, state);
}

void WVerticalTabs::Tab::setStretching(bool state)
{
	Flag::set(SELF->flags, TabFlag::IsStretching, state);
}

void WVerticalTabs::Tab::setCollapsable(bool state)
{
	Flag::set(SELF->flags, TabFlag::IsCollapsable, state);
}

void WVerticalTabs::Tab::setDescription(string text)
{
	SELF->description = text;
	String::toUpper(SELF->description);
}

typedef WVerticalTabs::Tab Tab;
typedef WVerticalTabs::TabImpl TabImpl;

#undef SELF

// =====================================================================================================================
// WVerticalTabs.

static bool IsExpandedAndVisible(const TabImpl* tab)
{
	return Flag::all(tab->flags, TabFlag::IsExpanded | TabFlag::IsVisible);
}

static bool IsVisible(const TabImpl* tab)
{
	return tab->flags & TabFlag::IsVisible;
}

static bool IsExpanded(const TabImpl* tab)
{
	return tab->flags & TabFlag::IsExpanded;
}

static bool HasTitle(TabImpl* tab)
{
	return tab->description.length() > 0;
}

static Rect GetButtonRect(Rect rect, TabImpl* tab)
{
	return Rect(rect.l, rect.t + tab->offset, rect.r, rect.t + tab->offset + TitleH);
}

static Vec2 GetButtonPos(Rect rect, TabImpl* tab)
{
	return Vec2(rect.l + ButtonW / 2 + 2, rect.t + tab->offset + TitleH / 2);
}

static int ApplyFactor(int value, int factor)
{
	return (value * factor) >> 10;
}

WVerticalTabs::~WVerticalTabs()
{
	for (auto tab : myTabs)
		delete tab;
}

WVerticalTabs::WVerticalTabs()
	: myMargin(4)
{
}

void WVerticalTabs::setMargin(int margin)
{
	myMargin = margin;
}

Tab* WVerticalTabs::addTab(shared_ptr<Widget> widget, const char* description, bool expanded)
{
	auto tab = new TabImpl;

	tab->height = 0;
	tab->offset = 0;
	tab->flags = TabFlag::None;

	tab->flags |= TabFlag::IsVisible;
	Flag::set(tab->flags, TabFlag::IsExpanded, expanded);

	if (description)
		tab->setDescription(description);

	tab->widget = widget;
	myTabs.push_back(tab);

	return tab;
}

Tab* WVerticalTabs::getTab(int index)
{
	return myTabs[index];
}

int WVerticalTabs::getNumTabs() const
{
	return (int)myTabs.size();
}

void WVerticalTabs::onMousePress(MousePress& input)
{
	if (input.button != MouseButton::LMB || !isMouseOver())
		return;

	for (int tabIndex = 0; tabIndex < (int)myTabs.size(); ++tabIndex)
	{
		auto* tab = myTabs[tabIndex];
		if (Flag::all(tab->flags, TabFlag::IsVisible))
		{
			if (GetButtonRect(myRect, tab).contains(input.pos))
			{
				tab->flags ^= TabFlag::IsExpanded;
			}
		}
	}
}

void WVerticalTabs::onMeasure()
{
	int margins = myMargin * 2;

	myWidth = 0;
	myMinWidth = 0;
	myHeight = 0;
	myMinHeight = 0;

	int tabIndex = 0;
	for (auto tab : myTabs)
	{
		auto flags = tab->flags;
		tab->stretch = 0;
		tab->height = 0;
		tab->minHeight = 0;
		if (IsVisible(tab))
		{
			if (HasTitle(tab))
			{
				tab->height += TitleH;
				tab->minHeight += TitleH;
			}

			if (auto w = tab->widget)
			{
				w->measure();
				myWidth = max(myWidth, w->getWidth());
				myMinWidth = max(myMinWidth, w->getMinWidth());

				if (IsExpanded(tab))
				{
					tab->height += w->getHeight() + margins;
					tab->minHeight += w->getMinHeight() + margins;
					tab->stretch = (flags & TabFlag::IsStretching) ? 10000 : 0;
				}
			}			
		}
		if (tabIndex > 0)
		{
			myHeight += SeperatorH;
			myMinHeight += SeperatorH;
		}
		myHeight += tab->height;
		myMinHeight += tab->minHeight;
		++tabIndex;
	}
	myWidth += margins;
	myMinWidth += margins;
}

void WVerticalTabs::onArrange(Rect r)
{
	myRect = r;

	int topMargin = TitleH;

	// Calculate combined height, minimum height and stretch.
	int combinedHeight = 0;
	int combinedMinHeight = 0;
	int combinedStretch = 0;
	for (auto tab : myTabs)
	{
		combinedHeight += tab->height;
		combinedMinHeight += tab->minHeight;
		combinedStretch += tab->stretch;
	}

	// Check if the combined height requirement can be met.
	if (r.h() >= combinedHeight)
	{
		int leftoverHeight = r.h() - combinedHeight;
		double scalar = (double)leftoverHeight / (double)max(1, combinedStretch);
		int offset = 0;
		for (auto tab : myTabs)
		{
			tab->offset = offset;
			tab->height += int(tab->stretch * scalar);
			offset += tab->height + SeperatorH;
		}
	}
	// If not, check if the available height is between height and minimum height.
	else if (r.h() >= combinedMinHeight)
	{
		int heightDiff = max(1, combinedHeight - combinedMinHeight);
		int leftoverHeight = r.h() - combinedMinHeight;
		double t = (double)leftoverHeight / (double)heightDiff;
		int offset = 0;
		for (auto tab : myTabs)
		{
			tab->offset = offset;
			tab->height = (int)lerp<double>(tab->minHeight, tab->height, t);
			offset += tab->height + SeperatorH;
		}
	}
	else // If not, assign the minimum required space.
	{
		int offset = 0;
		for (auto tab : myTabs)
		{
			tab->offset = offset;
			tab->height = tab->minHeight;
			offset += tab->height + SeperatorH;
		}
	}

	// Arrange the widgets inside the tabs.
	auto innerRect = r.shrink(myMargin);
	for (auto tab : myTabs)
	{
		if (IsExpandedAndVisible(tab))
		{
			auto tabRect = innerRect.sliceT(tab->height).offsetY(tab->offset);
			
			if (HasTitle(tab))
				tabRect.t += TitleH;

			tab->widget->arrange(tabRect);
		}
	}
}

UiElement* WVerticalTabs::findMouseOver(int x, int y)
{
	for (auto tab : myTabs)
	{
		if (IsExpandedAndVisible(tab))
		{
			if (auto mouseOver = tab->widget->findMouseOver(x, y))
				return mouseOver;
		}
	}
	return Widget::findMouseOver(x, y);
}

void WVerticalTabs::handleEvent(UiEvent& uiEvent)
{
	for (auto tab : myTabs)
		tab->widget->handleEvent(uiEvent);

	Widget::handleEvent(uiEvent);
}

void WVerticalTabs::tick(bool enabled)
{
	// Tick widgets and update the animation flags.
	for (int tabIndex = 0; tabIndex < (int)myTabs.size(); ++tabIndex)
	{
		auto* tab = myTabs[tabIndex];
		if (IsExpandedAndVisible(tab))
			tab->widget->tick(enabled);
	}

	// Update the tooltip.
	if (isMouseOver())
	{
		Vec2 mpos = Window::getMousePos();
		for (int tabIndex = 0; tabIndex < (int)myTabs.size(); ++tabIndex)
		{
			auto* tab = myTabs[tabIndex];
			if (GetButtonRect(myRect, tab).contains(mpos))
			{
				setTooltip((tab->flags & TabFlag::IsExpanded) ? "Collapse tab" : "Expand tab");
			}
		}
	}
}

void WVerticalTabs::draw(bool enabled)
{
	Vec2 mousePos = Window::getMousePos();
	
	auto titleStyle = UiText::regular;
	titleStyle.fontSize -= 2;
	titleStyle.textColor = Color(140, 255);
	titleStyle.shadowColor = Color::Blank;

	auto sepT = Color(20, 255);
	auto sepB = Color(70, 255);

	// Tabs.
	int tabIndex = 0;
	for (auto tab : myTabs)
	{
		if (IsVisible(tab))
		{
			Rect r = myRect.shrinkX(myMargin).sliceT(tab->height).offsetY(tab->offset);

			// Seperator.
			if (tabIndex > 0)
			{
				auto separatorRect = r.sliceT(2).offsetY(-SeperatorH / 2 - 1);
				Draw::fill(separatorRect, sepT, sepT, sepB, sepB, false);
			}

			if (HasTitle(tab))
			{
				// Collapse button.
				auto& misc = UiStyle::getMisc();
				Rect buttonRect = GetButtonRect(myRect, tab);
				Vec2 buttonPos = GetButtonPos(myRect, tab);

				bool isMouseOverButton = isMouseOver() && buttonRect.contains(mousePos.x, mousePos.y);
				Color color(isMouseOverButton ? 120 : 70, 255);

				if (IsExpanded(tab))
				{
					misc.arrow.draw(buttonPos, color, Sprite::Orientation::Rot90);
				}
				else
				{
					misc.arrow.draw(buttonPos, color);
				}

				// Title text.
				auto rtext = r.sliceT(TitleH).shrink(ButtonW, 1, 0, 1);
				Text::setStyle(titleStyle);
				Text::format(TextAlign::TL, tab->description.data());
				Text::draw(rtext);
			}

			// Tab contents.
			if (IsExpanded(tab))
				tab->widget->draw(enabled);
		}
		++tabIndex;
	}
}

} // namespace AV
