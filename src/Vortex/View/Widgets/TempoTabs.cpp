#include <Precomp.h>

#include <Vortex/View/Widgets/TempoTabs.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Text.h>
#include <Core/Graphics/Canvas.h>

#include <Core/Interface/UiStyle.h>

#include <Simfile/Simfile.h>
#include <Simfile/Tempo.h>
#include <Simfile/NoteSet.h>
#include <Simfile/Tempo.h>
#include <Simfile/GameMode.h>

#include <Vortex/Editor.h>

#include <Vortex/View/View.h>

#include <Vortex/Edit/Selection.h>

namespace AV {

using namespace std;

// =====================================================================================================================
// Tempo tabs :: button.

/*

class WTempoTabs::ButtonList : public Widget
{
public:
	~ButtonList();
	ButtonList(SegmentType type);

	void updateButtons();

	void onMeasure() override;
	void draw(bool enabled) override;

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	const char* getType() const;

	int getButtonAtPos(Vec2 pos);

private:
	SegmentType myType;
	int myPressedButton;
	int myNumButtons;
};

WTempoTabs::ButtonList::~ButtonList()
{
}

WTempoTabs::ButtonList::ButtonList(SegmentType type)
	: myType(type)
	, myNumButtons(0)
	, myPressedButton(-1)
{
	myWidth = 200;
	myMinWidth = 100;
}

void WTempoTabs::ButtonList::updateButtons()
{
	auto tempo = Editor::currentTempo();
	if (tempo)
	{
		myNumButtons = (tempo->begin() + (size_t)myType)->size();
	}
	else
	{
		myNumButtons = 0;
	}
}

int WTempoTabs::ButtonList::getButtonAtPos(Vec2 pos)
{
	for (int index = 0; index < myNumButtons; ++index)
	{
		if (myRect.top(20).offsetY(index * 16).contains(pos))
			return index;
	}
	return -1;
}

void WTempoTabs::ButtonList::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::Lmb && input.unhandled())
		{
			startCapturingMouse();
			myPressedButton = getButtonAtPos(input.pos);
			auto tempo = Editor::currentTempo();
			if (tempo && myPressedButton >= 0)
			{
				auto segments = (tempo->begin() + (size_t)myType);
				if (segments->size() > myPressedButton)
				{
					auto segment = segments->begin() + myPressedButton;
					View::setCursorRow(segment->row);
				}
			}
		}
		input.setHandled();
	}
}

void WTempoTabs::ButtonList::onMouseRelease(MouseRelease& input)
{
	stopCapturingMouse();
	myPressedButton = -1;
}

void WTempoTabs::ButtonList::onMeasure()
{
	myHeight = myNumButtons * 16;
	myMinHeight = myNumButtons * 16;
}

void WTempoTabs::ButtonList::draw(bool enabled)
{
	auto tempo = Editor::currentTempo();
	if (!tempo) return;

	int selectedItem = myPressedButton;
	if (selectedItem == -1)
		selectedItem = getButtonAtPos(Window::getMousePos());

	int index = 0;
	Rect r = myRect.top(16);

	auto list = tempo->begin() + (size_t)myType;
	auto meta = Segment::getTypeData(myType);
	auto seg = list->begin(), segEnd = list->end();

	int cx = r.cx();
	int topY = -8;
	int bottomY = Window::getSize().h + 8;

	Draw::fill(myRect.center(1), Color(128, 255));

	Text::setStyle(UiText::regular);
	while (seg != segEnd && r.t < topY)
	{
		r.t += 16;
		++index;
		++seg;
	}
	while (seg != segEnd && r.t < bottomY)
	{
		if (selectedItem == index)
			Draw::fill(r, Color(255, isCapturingMouse() ? 20 : 30));

		auto row = String::fromDouble(seg->row * BeatsPerRow, 3, 3);
		
		Text::format(TextAlign::MR, row);
		Text::draw(cx - 6, r.t + 8);

		auto description = meta->getDescription(seg.ptr);
		Text::format(TextAlign::ML, description);
		Text::draw(cx + 6, r.t + 8);

		r.t += 16;
		++index;
		++seg;
	}
}

const char* WTempoTabs::ButtonList::getType() const
{
	return "TempoButtons";
}

*/

// =====================================================================================================================
// DialogTempoBreakdown

WTempoTabs::~WTempoTabs()
{
}

WTempoTabs::WTempoTabs()
{
	// TODO: fix
	/*
	for (size_t type = 0; type < (size_t)SegmentType::COUNT; ++type)
	{
		myButtonLists[type] = make_shared<ButtonList>((SegmentType)type);
		addTab(myButtonLists[type], Segment::getTypeData((SegmentType)type)->plural);
	}
	*/
	updateList();
}

void WTempoTabs::updateList()
{
	auto sim = Editor::currentSimfile();
	auto chart = Editor::currentChart();

	// TODO: fix
	/*
	Tempo* tempo = Editor::currentTempo();
	if (tempo)
	{
		for (size_t type = 0; type < (size_t)SegmentType::COUNT; ++type)
		{
			auto tab = getTab(type);
			auto list = tempo->begin() + type;
			tab->setVisible(list->size() > 0);
			myButtonLists[type]->updateButtons();
		}
	}
	else
	{
		for (size_t type = 0; type < (size_t)SegmentType::COUNT; ++type)
		{
			myButtonLists[type]->updateButtons();
			getTab(type)->setVisible(false);
		}
	}
	*/
}

} // namespace AV
