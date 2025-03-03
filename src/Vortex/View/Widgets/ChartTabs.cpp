#include <Precomp.h>

#include <Vortex/View/Widgets/ChartTabs.h>

#include <Core/Utils/Util.h>
#include <Core/Utils/String.h>
#include <Core/Utils/vector.h>

#include <Core/System/System.h>
#include <Core/System/Window.h>

#include <Core/Graphics/Text.h>
#include <Core/Graphics/Canvas.h>

#include <Core/Interface/UiStyle.h>

#include <Simfile/Simfile.h>
#include <Simfile/Chart.h>
#include <Simfile/NoteSet.h>
#include <Simfile/Tempo.h>
#include <Simfile/GameMode.h>

#include <Vortex/Editor.h>

#include <Vortex/View/View.h>

#include <Vortex/Edit/Selection.h>

namespace AV {

using namespace std;
using namespace Util;

// =====================================================================================================================
// Chart tabs :: button.

enum class ButtonState
{
	Idle,
	Hover,
	Pressed,
};

class WChartTabs::ButtonList : public Widget
{
public:
	~ButtonList();
	ButtonList(const Graphics* graphics);

	void updateButtons(const GameMode* mode);

	void onMeasure() override;
	void draw(bool enabled) override;

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	const char* getType() const;

	int getButtonAtPos(Vec2 pos);

private:
	const Graphics* myGraphics;
	const GameMode* myGameMode;
	int myNumButtons;
	int myPressedButton;
};

WChartTabs::ButtonList::~ButtonList()
{
}

WChartTabs::ButtonList::ButtonList(const Graphics* graphics)
	: myGameMode(nullptr)
	, myPressedButton(-1)
	, myNumButtons(0)
	, myGraphics(graphics)
{
	myWidth = 300;
	myMinWidth = 100;
}

void WChartTabs::ButtonList::updateButtons(const GameMode* mode)
{
	myNumButtons = 0;
	myGameMode = mode;

	auto sim = Editor::currentSimfile();
	if (sim)
	{
		for (auto chart : sim->charts)
		{
			myNumButtons += (chart->gameMode == myGameMode);
		}
	}
}

int WChartTabs::ButtonList::getButtonAtPos(Vec2 pos)
{
	for (int index = 0; index < myNumButtons; ++index)
	{
		if (myRect.sliceT(20).offsetY(index * 21).contains(pos))
			return index;
	}
	return -1;
}

void WChartTabs::ButtonList::onMousePress(MousePress& input)
{
	if (isMouseOver())
	{
		if (isEnabled() && input.button == MouseButton::LMB && input.unhandled())
		{
			startCapturingMouse();
			myPressedButton = getButtonAtPos(input.pos);
			auto sim = Editor::currentSimfile();
			if (sim && myPressedButton >= 0)
			{
				int counter = myPressedButton;
				for (auto chart : sim->charts)
				{
					if (chart->gameMode == myGameMode)
					{
						if (counter == 0)
						{
							Editor::openChart(chart.get());
							break;
						}
						else
						{
							--counter;
						}
					}
				}
			}
		}
		input.setHandled();
	}
}

void WChartTabs::ButtonList::onMouseRelease(MouseRelease& input)
{
	stopCapturingMouse();
	myPressedButton = -1;
}

static void DrawButton(Rect r, Chart* chart, const WChartTabs::Graphics* graphics, ButtonState state)
{
	Text::setStyle(UiText::regular, TextOptions::Ellipses);

	// Draw the button graphic.
	graphics->flat.draw(r);

	// Draw the first segment with difficulty and meter.
	auto firstSegment = (r.w() > 100) ? r.sliceL(100) : r;
	Color color = GetDifficultyColor(chart->difficulty.get());
	graphics->color.draw(firstSegment, color);
	
	auto diff = GetDifficultyName(chart->difficulty.get());
	Text::format(TextAlign::ML, diff);
	Text::draw(firstSegment.centerL().offsetX(4));

	auto meter = String::fromInt(chart->meter.get());
	Text::format(TextAlign::MR, meter);
	Text::draw(firstSegment.centerR().offsetX(-4));

	// Draw the second segment with author and step count.
	if (r.r > firstSegment.r)
	{
		auto secondSegment = r.shrinkL(firstSegment.w());

		auto stepCount = String::fromInt(chart->getStepCount());
		Text::format(TextAlign::MR, secondSegment.w() - 8, stepCount);
		Text::draw(secondSegment.centerR().offsetX(-4));

		auto description = chart->description.get();
		if (description.empty())
			description = chart->author.get();

		Text::format(TextAlign::ML, secondSegment.w() - Text::getWidth() - 16, description);
		Text::draw(secondSegment.centerL().offsetX(4));
	}

	// Draw the hover highlight overlay.
	if (state == ButtonState::Hover)
	{
		graphics->hover.draw(r);
	}
}

void WChartTabs::ButtonList::onMeasure()
{
	myHeight = myNumButtons * 21;
	myMinHeight = myNumButtons * 21;
}

void WChartTabs::ButtonList::draw(bool enabled)
{
	auto sim = Editor::currentSimfile();
	if (!sim) return;

	int selectedItem = myPressedButton;
	if (selectedItem == -1)
	{
		auto mpos = Window::getMousePos();
		selectedItem = getButtonAtPos(mpos);
	}

	int index = 0;
	Rect itemRect = myRect.sliceT(20);
	for (const auto& chart : sim->charts)
	{
		if (chart->gameMode == myGameMode)
		{
			ButtonState state;
			if (selectedItem == index)
			{
				state = isCapturingMouse() ? ButtonState::Pressed : ButtonState::Hover;
			}
			else
			{
				state = ButtonState::Idle;
			}
			DrawButton(itemRect.offsetY(index * 21), chart.get(), myGraphics, state);
			++index;
		}
	}
}

const char* WChartTabs::ButtonList::getType() const
{
	return "ChartButtons";
}

// =====================================================================================================================
// DialogChartList

WChartTabs::~WChartTabs()
{
}

static unique_ptr<Texture> CreateHoverGraphic()
{
	Canvas c(16, 16, 1.0f);
	c.setColor(Colorf(1.0f, 0.05f));
	c.box(2, 2, 14, 14, 1.2f);
	c.setFill(false);
	c.setOutline(1);
	c.setColor(Colorf(1.0f, 0.1f));
	c.box(1, 1, 15, 15, 1.0f);
	return c.createTexture("CLhover", false);
}

static unique_ptr<Texture> CreateButtonGraphic()
{
	Canvas c(16, 16, 0.15f);
	c.setColor(Colorf(0.15f, 1.0f));
	c.box(0, 0, 16, 16, 0.8f);
	c.setColor(Colorf(0.3f, 1.0f));
	c.box(1, 1, 15, 15, 1.0f);
	c.setColor(Colorf(0.25f, 1.0f));
	c.box(2, 2, 14, 14, 1.2f);
	return c.createTexture("CLbutton", false);
}

static unique_ptr<Texture> CreateColorGraphic()
{
	Canvas c(16, 16, 0.9f);
	c.setColor(Colorf(0.75f, 1.0f));
	c.box(1, 1, 15, 15, 1.0f);
	c.setColor(Colorf(0.65f, 1.0f));
	c.box(2, 2, 14, 14, 1.2f);
	return c.createTexture("CLcolor", false);
}

WChartTabs::WChartTabs()
{
	myGraphics.flat.texture = CreateButtonGraphic();
	myGraphics.color.texture = CreateColorGraphic();
	myGraphics.hover.texture = CreateHoverGraphic();

	myGraphics.flat.border = 4;
	myGraphics.color.border = 4;
	myGraphics.hover.border = 4;

	updateList();
}

void WChartTabs::updateList()
{
	int chartIndex = 0;
	int numTabs = 0;

	auto sim = Editor::currentSimfile();
	if (sim)
	{
		int numCharts = (int)sim->charts.size();
		if (numCharts > 0)
		{
			const GameMode* currentGameMode = nullptr;
			WVerticalTabs::Tab* currentTab = nullptr;
			shared_ptr<WChartTabs::ButtonList> currentList;

			for (int i = 0; i < numCharts; ++i)
			{
				auto gameMode = sim->charts[i]->gameMode;
				if (gameMode != currentGameMode)
				{
					if (numTabs >= (int)myButtonLists.size())
					{
						currentList = make_shared<WChartTabs::ButtonList>(&myGraphics);
						myButtonLists.push_back(currentList);
					}
					else
					{
						currentList = myButtonLists[numTabs];
					}

					if (numTabs >= getNumTabs())
					{
						currentTab = addTab(currentList);
						// TODO: fix currentList, because it should be owned by the tab,
						// but right now it is not.
					}
					else
					{
						currentTab = getTab(numTabs);
					}

					currentList->updateButtons(gameMode);

					currentTab->setDescription(gameMode->game + " " + gameMode->mode);
					currentTab->setVisible(true);

					currentGameMode = gameMode;

					++numTabs;
				}
			}
		}
	}

	while ((int)myButtonLists.size() > numTabs)
	{
		myButtonLists.pop_back();
	}
	for (int i = numTabs; i < getNumTabs(); ++i)
	{
		getTab(i)->setVisible(false);
	}
}

} // namespace AV
