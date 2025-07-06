#include <Dialogs/LabelBreakdown.h>

#include <System/System.h>

#include <Core/StringUtils.h>
#include <Core/Canvas.h>

#include <Managers/TempoMan.h>
#include <Managers/SimfileMan.h>

#include <Editor/Common.h>
#include <Editor/View.h>

#include <Simfile/SegmentGroup.h>

#define Dlg DialogLabelBreakdown

namespace Vortex {

// ================================================================================================
// LabelButton
enum DisplayType {
	TIMESTAMP,
	BEAT,
	ROW
};

struct Dlg::LabelButton : public GuiWidget {

LabelButton(GuiContext* gui, TileRect2* bar, int row, String time, String text)
	: GuiWidget(gui)
	, myBar(bar)
	, myRow(row)
	, myDisplayTime(time)
	, myDisplayText(text)
{
}

void onMousePress(MousePress& evt) override
{
	if(isMouseOver())
	{
		if(isEnabled() && evt.button == Mouse::LMB && evt.unhandled())
		{
			startCapturingMouse();
			gView->setCursorRow(myRow);
		}
		evt.setHandled();
	}
}

void onMouseRelease(MouseRelease& evt) override
{
	if(isCapturingMouse() && evt.button == Mouse::LMB)
	{
		stopCapturingMouse();
	}
}

void onTick() override
{
	GuiWidget::onTick();
	if(isMouseOver())
	{
		GuiMain::setTooltip("Jump to label");
	}
}

void onDraw() override
{
	recti r = myRect;

	TextStyle textStyle;
	textStyle.textFlags = Text::ELLIPSES;

	// Draw the button graphic.
	auto& button = GuiDraw::getButton();
	button.base.draw(r, 0);

	// Draw the button text.
	recti left = { myRect.x, myRect.y, 74, 20 };

	color32 color = COLOR32(139, 148, 148, 255);
	myBar->draw(left, 0, color);

	Text::arrange(Text::MR, myDisplayTime.str());
	Text::draw(vec2i{ left.x + left.w - 6, left.y + 10 });

	Text::arrange(Text::ML, textStyle, myDisplayText.str());
	Text::draw(vec2i{ left.x + left.w + 6, left.y + 10 });

	// Interaction effects.
	if(isCapturingMouse())
	{
		button.pressed.draw(r, 0);
	}
	else if(isMouseOver())
	{
		button.hover.draw(r, 0);
	}
}

int myRow;
String myDisplayTime;
String myDisplayText;
TileRect2* myBar;
};

// ================================================================================================
// LabelList

struct Dlg::LabelList : public WgScrollRegion {

Vector<LabelButton*> myButtons;
TileRect2 myButtonTex;
int myDisplayType;

~LabelList()
{
	for(auto button : myButtons)
	{
		delete button;
	}
}

LabelList(GuiContext* gui)
	: WgScrollRegion(gui)
{
	setScrollType(SCROLL_NEVER, SCROLL_WHEN_NEEDED);

	Canvas c(32, 16);

	c.setColor(ToColor(0.8f, 0.8f));
	c.box(1, 1, 15, 15, 0.0f);
	c.box(17, 1, 31, 15, 3.5f);
	c.setColor(ToColor(0.6f, 0.8f));
	c.box(2, 2, 14, 14, 0.0f);
	c.box(18, 2, 30, 14, 2.5f);

	myButtonTex.texture = c.createTexture();
	myButtonTex.border = 4;

	updateButtons();
}

void onUpdateSize() override
{
	myScrollH = max(24, myButtons.size() * 21);
	myClampScrollValues();
}

void onTick() override
{
	myPreTick();

	int viewW = getViewWidth() - 2 * myScrollbarActiveV;

	// Update the properties of each button.
	int y = myRect.y - myScrollY;
	for(int i = 0; i < myButtons.size(); ++i)
	{
		auto button = myButtons[i];
		button->arrange({myRect.x, y, viewW, 20});
		button->tick();
		y += 21;
	}

	myPostTick();
}

void onDraw() override
{
	if (gSimfile->isClosed()) return;

	TextStyle textStyle;
	int w = getViewWidth() - 2 * myScrollbarActiveV;
	int h = getViewHeight();
	int x = myRect.x;
	int y = myRect.y - myScrollY;
	const Style* style = nullptr;
	int chartIndex = 0;

	Renderer::pushScissorRect({myRect.x, myRect.y, w, h});
	if(myButtons.empty())
	{
		Text::arrange(Text::MC, textStyle, "- no labels -");
		Text::draw(vec2i{x + w / 2, y + 16});
	}
	else for(auto button : myButtons)
	{
		button->draw();
		y += 21;
	}
	Renderer::popScissorRect();

	WgScrollRegion::onDraw();
}

void updateButtons()
{
	if (gSimfile->isClosed()) return;

	while (myButtons.size() > 0)
	{
		delete myButtons.back();
		myButtons.pop_back();
	}

	auto segs = gTempo->getSegments();
	auto& list = segs->getList<Label>();
	auto seg = list.begin(), segEnd = list.end();
	while (seg != segEnd)
	{
		String time = displayTime(seg->row);
		String text = segs->getRow<Label>(seg->row).str;
		myButtons.push_back(new LabelButton(getGui(), &myButtonTex, seg->row, time, text));
		++seg;
	}
}

void setDisplayType(int type)
{
	myDisplayType = type;
	updateButtons();
}

String displayTime(int row) const
{
	if (myDisplayType == BEAT) {
		return Str::val((double)row / ROWS_PER_BEAT, 3, 3);
	}
	else if (myDisplayType == ROW) {
		return Str::val(row);
	}
	else {
		double time = gTempo->rowToTime(row);
		return Str::formatTime(time);
	}
}

void copyLabels()
{
	String out;
	for (auto button : myButtons)
	{
		out += button->myDisplayTime;
		out += ": ";
		out += button->myDisplayText;
		out += "\n";
	}
	if(out.empty()) {
		HudInfo("%s", "There is no labels to copy...");
	}
	else {
		gSystem->setClipboardText(out);
		HudInfo("%s", "Label list copied to clipboard.");
	}
}

};

// ================================================================================================
// DialogLabelList

Dlg::~Dlg()
{
}

Dlg::Dlg()
{
	setTitle("LABELS");

	setWidth(298);
	setMinimumWidth(298);
	setMinimumHeight(152);
	setMaximumWidth(1024);
	setMaximumHeight(1024);
	setResizeable(true, true);

	myDisplayType = TIMESTAMP;
	myCreateWidgets();
	onChanges(VCM_ALL_CHANGES);
	clear();
}

void Dlg::clear()
{
	myLabelText.clear();
}

void Dlg::myCreateWidgets()
{
	myLayout.row().col(44).col(234, true);

	WgLineEdit* text = myLayout.add<WgLineEdit>("Label");
	text->text.bind(&myLabelText);
	text->setMaxLength(1000);
	text->onChange.bind(this, &Dlg::onChange);
	text->setTooltip("Label text");

	myLayout.row().col(284, true);
	myList = myLayout.add<LabelList>();

	myLayout.row().col(74).col(210, true);
	myDisplayTypeList = myLayout.add<WgCycleButton>();
	myDisplayTypeList->addItem("Time");
	myDisplayTypeList->addItem("Beat");
	myDisplayTypeList->addItem("Row");
	myDisplayTypeList->value.bind(&myDisplayType);
	myDisplayTypeList->onChange.bind(this, &Dlg::mySetDisplayType);
	myDisplayTypeList->setTooltip("Change the unit used for timestamps.");

	WgButton* copy = myLayout.add<WgButton>();
	copy->text.set("{g:copy}");
	copy->setTooltip("Copy labels to clipboard");
	copy->onPress.bind(this, &Dlg::myCopyLabels);
}

void Dlg::onChanges(int changes)
{
	if (changes & VCM_FILE_CHANGED)
	{
		clear();
		if (gSimfile->isOpen())
		{
			for (auto w : myLayout) w->setEnabled(true);
		}
		else
		{
			for (auto w : myLayout) w->setEnabled(false);
		}
	}
	if(changes & VCM_TEMPO_CHANGED)
	{
		myList->updateButtons();
	}
}

void Dlg::onUpdateSize()
{
	myLayout.onUpdateSize();
}

void Dlg::onTick()
{
	recti bounds = getInnerRect();
	myList->setHeight(bounds.h - 58);

	if (gSimfile->isOpen())
	{
		int row = gView->getCursorRow();
		auto segs = gTempo->getSegments();

		myLabelText = segs->getRow<Label>(row).str;
	}

	EditorDialog::onTick();
}

void Dlg::onChange()
{
	if (gSimfile->isClosed()) return;

	int row = gView->getCursorRow();
	if (strpbrk(myLabelText.str(), ";,=") != nullptr) {
		HudWarning("A Label cannot contain commas, semicolons, or equal signs; they will be replaced with underscores.");
		Str::replace(myLabelText, ",", "_");
		Str::replace(myLabelText, ";", "_");
		Str::replace(myLabelText, "=", "_");
	}
	gTempo->addSegment(Label(row, myLabelText));
}

void Dlg::mySetDisplayType()
{
	myList->setDisplayType(myDisplayType);
}


void Dlg::myCopyLabels()
{
	myList->copyLabels();
}
}; // namespace Vortex
