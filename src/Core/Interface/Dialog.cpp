#include <Precomp.h>

#include <Core/System/Debug.h>
#include <Core/System/Window.h>

#include <Core/Utils/Flag.h>

#include <Core/Graphics/Draw.h>
#include <Core/Graphics/Text.h>

#include <Core/Interface/UiMan.h>
#include <Core/Interface/Dialog.h>
#include <Core/Interface/DialogFrame.h>
#include <Core/Interface/UiStyle.h>

namespace AV {

using namespace std;

typedef DialogFrame::ActionType         ActionType;
typedef DialogFrame::BaseAction         BaseAction;
typedef DialogFrame::DragAction         DragAction;
typedef DialogFrame::ResizeAction       ResizeAction;
typedef DialogFrame::ScrollDragAction   ScrollDragAction;
typedef DialogFrame::ScrollButtonAction ScrollButtonAction;

typedef ResizeAction::HorizontalAction  ResizeActionH;
typedef ResizeAction::VerticalAction    ResizeActionV;

typedef Dialog::ResizeBehavior ResizeBehavior;

static constexpr int MinimumSizeW = 100;
static constexpr int MinimumSizeH = 40;

static constexpr int MaximumSizeW = 2048;
static constexpr int MaximumSizeH = 2048;

static constexpr int TitlebarH = 24;
static constexpr int TitlebarButtonW = 16;

static constexpr int ResizeMargin = 6;

static constexpr int ScrollbarRegionSize = 13;
static constexpr int ScrollbarBarSize = 5;
static constexpr int ScrollbarArrowSize = 13;

static constexpr int ScrollbarMarginEnd = 1;
static constexpr int ScrollbarMarginSide = 3;

static constexpr float ScrollbarBumpSpeed = 300.0f;

// =====================================================================================================================
// General helper functions.

static int ClampX(const DialogFrame* frame, int x, int viewWidth)
{
	return clamp(x, MinimumSizeW / 2 - frame->width, viewWidth - MinimumSizeW / 2);
}

static int ClampY(const DialogFrame* frame, int y, int viewHeight)
{
	return clamp(y, TitlebarH, viewHeight);
}

static int GetMaximumWidth(const DialogFrame* frame)
{
	return frame->resizeBehaviourX == (uint)ResizeBehavior::Bound
		? max(MinimumSizeW, frame->prefSize.w + frame->margin * 2)
		: MaximumSizeW;
}

static int GetMaximumHeight(const DialogFrame* frame)
{
	return frame->resizeBehaviourY == (uint)ResizeBehavior::Bound
		? max(MinimumSizeH, frame->prefSize.h + frame->margin * 2)
		: MaximumSizeH;
}

static int ClampWidth(const DialogFrame* frame, int width)
{
	return frame->resizeBehaviourX == (uint)ResizeBehavior::Fixed
		? frame->prefSize.w + frame->margin * 2
		: clamp(width, MinimumSizeW, GetMaximumWidth(frame));
}

static int ClampHeight(const DialogFrame* frame, int height)
{
	return frame->resizeBehaviourY == (uint)ResizeBehavior::Fixed
		? frame->prefSize.h + frame->margin * 2
		: clamp(height, MinimumSizeH, GetMaximumHeight(frame));
}

static Rect GetFullRect(const DialogFrame* frame)
{
	int rightScrollbarW = frame->showScrollbarY * ScrollbarRegionSize;
	int bottomScrollbarH = frame->showScrollbarX * ScrollbarRegionSize;
	if (frame->isExpanded)
	{
		return Rect(
			frame->x,
			frame->y - TitlebarH,
			frame->x + frame->width + rightScrollbarW,
			frame->y + frame->height + bottomScrollbarH);
	}
	else
	{
		return Rect(
			frame->x,
			frame->y - TitlebarH,
			frame->x + frame->width + rightScrollbarW,
			frame->y);
	}
}

static Rect GetViewRect(const DialogFrame* frame, int shrink)
{
	return Rect(
		frame->x + shrink,
		frame->y + shrink,
		frame->x + frame->width - shrink,
		frame->y + frame->height - shrink);
}

static Rect GetContentRect(const DialogFrame* frame)
{
	int x = frame->x + frame->margin - frame->scrollOffsetX;
	int y = frame->y + frame->margin - frame->scrollOffsetY;
	int w = max(frame->minSize.w, frame->width - frame->margin * 2);
	int h = max(frame->minSize.h, frame->height - frame->margin * 2);
	return Rect(x, y, x + w, y + h);
}

// =====================================================================================================================
// Dialog dragging.

static DragAction* StartDragAction(DialogFrame* frame, Vec2 mousePos)
{
	auto result = new DragAction;
	result->type = ActionType::Drag;
	result->grabOffset = Vec2(mousePos.x - frame->x, mousePos.y - frame->y);
	return result;
}

static void HandleDragAction(BaseAction* currentAction, DialogFrame* frame)
{
	if (currentAction->type != ActionType::Drag)
		return;

	auto action = (DragAction*)currentAction;
	auto viewSize = Window::getSize();
	auto mousePos = Window::getMousePos();

	frame->x = ClampX(frame, mousePos.x - action->grabOffset.x, viewSize.w);
	frame->y = ClampY(frame, mousePos.y - action->grabOffset.y, viewSize.h);
}

// =====================================================================================================================
// Dialog resizing.

static ResizeAction* StartResizeAction(DialogFrame* frame, ActionType action, Vec2 mousePos)
{
	auto result = new ResizeAction;
	result->type = action;

	auto rect = GetFullRect(frame);
	switch (action)
	{
	case ActionType::ResizeTL:
	case ActionType::ResizeL:
	case ActionType::ResizeBL:
		result->horizontalAction = ResizeActionH::left;
		result->anchor.x = frame->x + frame->width;
		result->grabOffset.x = mousePos.x - frame->x;
		frame->isAutoSizingX = 0;
		break;

	case ActionType::ResizeTR:
	case ActionType::ResizeR:
	case ActionType::ResizeBR:
		result->horizontalAction = ResizeActionH::right;
		result->anchor.x = frame->x;
		result->grabOffset.x = mousePos.x - (frame->x + frame->width);
		frame->isAutoSizingX = 0;
		break;
	}

	switch (action)
	{
	case ActionType::ResizeTL:
	case ActionType::ResizeT:
	case ActionType::ResizeTR:
		result->verticalAction = ResizeActionV::up;
		result->anchor.y = frame->y + frame->height;
		result->grabOffset.y = mousePos.y - frame->y;
		frame->isAutoSizingY = 0;
		break;

	case ActionType::ResizeBL:
	case ActionType::ResizeB:
	case ActionType::ResizeBR:
		result->verticalAction = ResizeActionV::down;
		result->anchor.y = frame->y;
		result->grabOffset.y = mousePos.y - (frame->y + frame->height);
		frame->isAutoSizingY = 0;
		break;
	}

	return result;
}

static void HandleResizeAction(BaseAction* currentAction, DialogFrame* frame)
{
	if (currentAction->type < ActionType::ResizeBegin || currentAction->type >= ActionType::ResizeEnd)
		return;

	auto action = (ResizeAction*)currentAction;
	auto targetPos = Window::getMousePos() - action->grabOffset;

	switch (action->horizontalAction)
	{
	case ResizeActionH::left:
		frame->width = ClampWidth(frame, action->anchor.x - targetPos.x);
		frame->x = action->anchor.x - frame->width;
		break;

	case ResizeActionH::right:
		frame->width = ClampWidth(frame, targetPos.x - action->anchor.x);
		break;
	}

	switch (action->verticalAction)
	{
	case ResizeActionV::up:
		frame->height = ClampHeight(frame, action->anchor.y - targetPos.y);
		frame->y = action->anchor.y - frame->height;
		break;

	case ResizeActionV::down:
		frame->height = ClampHeight(frame, targetPos.y - action->anchor.y);
		break;
	}
}

// =====================================================================================================================
// Dialog scrolling.

struct ScrollBarDimensions
{
	int pos;
	int length;
	int pageSize;
	int contentSize;
	int scrollOffset;
	bool horizontal;
	Rect rect;
};

struct ScrollButtonDimensions
{
	int pos;
	int length;
};

static ScrollButtonDimensions GetScrollButtonDimensions(const ScrollBarDimensions& bar, int offset)
{
	auto button = ScrollButtonDimensions { 0, bar.length };
	if (bar.contentSize > bar.pageSize)
	{
		double buttonLength = 0.5 + bar.length * (double)bar.pageSize / (double)bar.contentSize;
		button.length = clamp((int)buttonLength, 16, bar.length);

		int scrollableRange = bar.contentSize - bar.pageSize;
		int emptyBarArea = bar.length - button.length;
		double buttonPos = 0.5 + offset * (double)emptyBarArea / (double)scrollableRange;
		button.pos = bar.pos + clamp((int)buttonPos, 0, bar.length - button.length);
	}
	return button;
}

static ScrollBarDimensions GetHorizontalScrollbarDimensions(const DialogFrame* frame)
{
	int x = frame->x + 2;
	int y = frame->y + frame->height;
	Rect rect(x, y, x + frame->width - 3, y + ScrollbarRegionSize - 2);
	return ScrollBarDimensions {
		.pos = rect.l + ScrollbarArrowSize,
		.length = rect.w() - ScrollbarArrowSize * 2,
		.pageSize = frame->width,
		.contentSize = frame->minSize.w + frame->margin * 2,
		.horizontal = true,
		.rect = rect,
	};
}

static ScrollBarDimensions GetVerticalScrollbarDimensions(const DialogFrame* frame)
{
	int x = frame->x + frame->width;
	int y = frame->y + 1;
	Rect rect(x, y, x + ScrollbarRegionSize - 2, y + frame->height - 2);
	return ScrollBarDimensions {
		.pos = rect.t + ScrollbarArrowSize,
		.length = rect.h() - ScrollbarArrowSize * 2,
		.pageSize = frame->height,
		.contentSize = frame->minSize.h + frame->margin * 2,
		.horizontal = false,
		.rect = rect,
	};
}

static ActionType GetScrollActionAtPosition(const ScrollBarDimensions& bar, Vec2 mousePos, int offset)
{
	if (!bar.rect.contains(mousePos))
		return ActionType::None;

	int pos = bar.horizontal ? mousePos.x : mousePos.y;
	auto button = GetScrollButtonDimensions(bar, offset);

	if (pos < button.pos)
		return bar.horizontal ? ActionType::ScrollButtonL : ActionType::ScrollButtonU;

	if (pos >= button.pos + button.length)
		return bar.horizontal ? ActionType::ScrollButtonR : ActionType::ScrollButtonD;

	return bar.horizontal ? ActionType::ScrollDragX : ActionType::ScrollDragY;
}

static ScrollButtonAction* StartScrollButtonAction(ActionType action)
{
	auto result = new ScrollButtonAction;
	result->type = action;
	result->accumulatedDelta = 0;
	return result;
}

static void HandleScrollButtonAction(BaseAction* currentAction, DialogFrame* frame)
{
	if (currentAction->type < ActionType::ScrollButtonBegin || currentAction->type >= ActionType::ScrollButtonEnd)
		return;

	auto action = (ScrollButtonAction*)currentAction;
	action->accumulatedDelta += Window::getDeltaTime() * 500;

	if (action->accumulatedDelta >= 1.0)
	{
		double intPart;
		action->accumulatedDelta = modf(action->accumulatedDelta, &intPart);
		int delta = lround(intPart);
		switch (action->type)
		{
		case ActionType::ScrollButtonU:
			frame->scrollOffsetY -= delta;
			break;
		case ActionType::ScrollButtonD:
			frame->scrollOffsetY += delta;
			break;
		case ActionType::ScrollButtonL:
			frame->scrollOffsetX -= delta;
			break;
		case ActionType::ScrollButtonR:
			frame->scrollOffsetX += delta;
			break;
		}
	}
}

static ScrollDragAction* StartScrollDragAction(const ScrollBarDimensions& bar, int mousePos, int offset)
{
	auto result = new ScrollDragAction;
	auto button = GetScrollButtonDimensions(bar, offset);
	result->type = bar.horizontal ? ActionType::ScrollDragX : ActionType::ScrollDragY;
	result->grabOffset = mousePos - button.pos;
	return result;
}

static void HandleScrollDragAction(BaseAction* currentAction, DialogFrame* frame)
{
	if (currentAction->type < ActionType::ScrollDragBegin || currentAction->type >= ActionType::ScrollDragEnd)
		return;

	auto UpdateScrollOffset = [](const ScrollBarDimensions& bar, int& targetOffset, int pos)
	{
		int relativePos = pos - bar.pos;
		double offset = (double)bar.contentSize * (double)relativePos / (double)bar.length;
		targetOffset = (int)offset;
	};

	auto action = (ScrollDragAction*)currentAction;
	if (action->type == ActionType::ScrollDragX)
	{
		auto bar = GetHorizontalScrollbarDimensions(frame);
		auto targetPos = Window::getMousePos().x - action->grabOffset;
		UpdateScrollOffset(bar, frame->scrollOffsetX, targetPos);
	}
	else
	{
		auto bar = GetVerticalScrollbarDimensions(frame);
		auto targetPos = Window::getMousePos().y - action->grabOffset;
		UpdateScrollOffset(bar, frame->scrollOffsetY, targetPos);
	}
}

// =====================================================================================================================
// Update frame position/size/scroll offset based on view constaints.

static void HandleFrameConstraints(DialogFrame* frame)
{
	Size2 view = Window::getSize();

	// Apply autosizing, if enabled.
	if (frame->isAutoSizingX)
		frame->width = frame->prefSize.w + frame->margin * 2;

	if (frame->isAutoSizingY)
		frame->height = frame->prefSize.h + frame->margin * 2;

	// Enforce the minimum and maximum size.
	frame->width = ClampWidth(frame, frame->width);
	frame->height = ClampHeight(frame, frame->height);

	// Enable scrollbars if necessary.
	frame->showScrollbarX = frame->minSize.w + frame->margin * 2 > frame->width;
	frame->showScrollbarY = frame->minSize.h + frame->margin * 2 > frame->height;

	// Enable auto-sizing.
	if (!frame->currentAction)
	{
		if (frame->width == GetMaximumWidth(frame))
			frame->isAutoSizingX |= 1;
		if (frame->height == GetMaximumHeight(frame))
			frame->isAutoSizingY |= 1;
	}

	// Set the dialog to the pinned position when pinned.
	if (frame->isPinned)
	{
		frame->x = ClampX(frame, frame->pinnedPos.x, view.w);
		frame->y = ClampY(frame, frame->pinnedPos.y, view.h);
	}
	else
	{
		frame->pinnedPos.x = frame->x = ClampX(frame, frame->x, view.w);
		frame->pinnedPos.y = frame->y = ClampY(frame, frame->y, view.h);
	}
}

static void HandleScrollConstraints(DialogFrame* frame)
{
	if (frame->scrollOffsetX != 0)
	{
		auto bar = GetHorizontalScrollbarDimensions(frame);
		frame->scrollOffsetX = max(0, min(frame->scrollOffsetX, bar.contentSize - bar.pageSize));
	}
	if (frame->scrollOffsetY != 0)
	{
		auto bar = GetVerticalScrollbarDimensions(frame);
		frame->scrollOffsetY = max(0, min(frame->scrollOffsetY, bar.contentSize - bar.pageSize));
	}
}

// =====================================================================================================================
// Dialog actions.

static ActionType GetAction(const Dialog* dialog, const DialogFrame* frame, Vec2 pos)
{
	auto action = frame->currentAction.get();

	if (action)
		return action->type;

	Rect rect = GetFullRect(frame);
	if (rect.contains(pos))
	{
		// Titlebar buttons.
		int dx = pos.x - rect.r + TitlebarButtonW;
		int dy = pos.y - rect.t;
		if (dy < TitlebarH)
		{
			if (!frame->isPinned)
			{
				if (frame->isCloseable)
				{
					if (dx >= 0) return ActionType::Close;
					dx += TitlebarButtonW;
				}
				if (frame->isMinimizable)
				{
					if (dx >= 0) return ActionType::Minimize;
					dx += TitlebarButtonW;
				}
			}
			if (frame->isPinnable)
			{
				if (dx >= 0) return ActionType::Pin;
				dx += TitlebarButtonW;
			}
			return frame->isPinned ? ActionType::None : ActionType::Drag;
		}

		// Scroll bars.
		if (frame->showScrollbarX)
		{
			auto bar = GetHorizontalScrollbarDimensions(frame);
			auto action = GetScrollActionAtPosition(bar, pos, frame->scrollOffsetX);
			if (action != ActionType::None)
				return action;
		}
		if (frame->showScrollbarY)
		{
			auto bar = GetVerticalScrollbarDimensions(frame);
			auto action = GetScrollActionAtPosition(bar, pos, frame->scrollOffsetY);
			if (action != ActionType::None)
				return action;
		}

		// Resize indicator in the bottom right corner.
		if (!frame->isPinned && frame->showScrollbarX && frame->showScrollbarY)
		{
			if (pos.x > frame->x + frame->width && pos.y > frame->y + frame->height)
				return ActionType::ResizeBR;
		}
	}
	else
	{
		// Horizontal and vertical resizing.
		rect = rect.expand(ResizeMargin);
		if (rect.contains(pos) && !frame->isPinned)
		{
			int resizeH = 0;
			int resizeV = 0;
			int dx = pos.x - rect.l;
			int dy = pos.y - rect.t;
			int border = ResizeMargin * 2;

			if (frame->resizeBehaviourX != (uint)ResizeBehavior::Fixed)
				resizeH = (dx > rect.w() - border) - (dx < border);

			if (frame->resizeBehaviourY != (uint)ResizeBehavior::Fixed)
				resizeV = (dy > rect.h() - border) - (dy < border);

			if (resizeH != 0 || resizeV != 0)
			{
				int index = resizeV * 3 + resizeH + 4;
				ActionType actions[] =
				{
					ActionType::ResizeTL, ActionType::ResizeT, ActionType::ResizeTR,
					ActionType::ResizeL,  ActionType::None,    ActionType::ResizeR,
					ActionType::ResizeBL, ActionType::ResizeB, ActionType::ResizeBR,
				};
				return actions[index];
			}
		}
	}

	return ActionType::None;
}

// =====================================================================================================================
// Dialog drawing.

static const TileBar& GetScrollbarButtonStyle(const DialogFrame* frame, ActionType currentAction, ActionType target)
{
	auto& dlg = UiStyle::getDialog();

	if (currentAction != target)
		return dlg.scrollButtonBase;

	if (frame->currentAction)
		return dlg.scrollButtonPress;

	return dlg.scrollButtonHover;
}

static void DrawHorizontalScrollbar(const DialogFrame* frame, ActionType currentAction)
{
	auto& dlg = UiStyle::getDialog();

	auto bar = GetHorizontalScrollbarDimensions(frame);
	auto button = GetScrollButtonDimensions(bar, frame->scrollOffsetX);

	auto decAlpha = currentAction == ActionType::ScrollButtonL ? (frame->currentAction ? 255 : 200) : 100;
	auto incAlpha = currentAction == ActionType::ScrollButtonR ? (frame->currentAction ? 255 : 200) : 100;
	
	auto arrowY = bar.rect.t + ScrollbarArrowSize / 2 - 1;
	auto arrowX1 = bar.rect.l + ScrollbarArrowSize / 2;
	auto arrowX2 = bar.rect.r - ScrollbarArrowSize / 2;

	dlg.iconArrowR.draw(Vec2(arrowX1, arrowY), Color(255, decAlpha), Sprite::Orientation::Rot180);
	dlg.iconArrowR.draw(Vec2(arrowX2, arrowY), Color(255, incAlpha), Sprite::Orientation::Normal);

	auto buttonRect = bar.rect.withXandWidth(button.pos, button.length);
	auto& buttonStyle = GetScrollbarButtonStyle(frame, currentAction, ActionType::ScrollDragX);
	buttonStyle.draw(buttonRect);
}

static void DrawVerticalScrollbar(const DialogFrame* frame, ActionType currentAction)
{
	auto& dlg = UiStyle::getDialog();

	auto bar = GetVerticalScrollbarDimensions(frame);
	auto button = GetScrollButtonDimensions(bar, frame->scrollOffsetY);

	auto decAlpha = currentAction == ActionType::ScrollButtonU ? (frame->currentAction ? 255 : 200) : 100;
	auto incAlpha = currentAction == ActionType::ScrollButtonD ? (frame->currentAction ? 255 : 200) : 100;

	auto arrowX = bar.rect.l + ScrollbarArrowSize / 2 - 1;
	auto arrowY1 = bar.rect.t + ScrollbarArrowSize / 2;
	auto arrowY2 = bar.rect.b - ScrollbarArrowSize / 2;

	dlg.iconArrowR.draw(Vec2(arrowX, arrowY1), Color(255, decAlpha), Sprite::Orientation::Rot270);
	dlg.iconArrowR.draw(Vec2(arrowX, arrowY2), Color(255, incAlpha), Sprite::Orientation::Rot90);

	auto buttonRect = bar.rect.withYandHeight(button.pos, button.length);
	auto& buttonStyle = GetScrollbarButtonStyle(frame, currentAction, ActionType::ScrollDragY);
	buttonStyle.draw(buttonRect, Color::White, TileBar::Flags::Vertical);
}

static void DrawFrame(Dialog* dialog, DialogFrame* frame)
{
	auto& style = UiStyle::getDialog();

	auto action = ActionType::None;
	if (frame->currentAction)
	{
		action = frame->currentAction->type;
	}
	else if (dialog->isMouseOver())
	{
		Vec2 mpos = Window::getMousePos();
		action = GetAction(dialog, frame, mpos);
	}

	// Outer frame.
	auto outer = GetFullRect(frame);
	style.outerFrame.draw(outer);

	// Inner frame.
	if (frame->isExpanded)
	{
		auto viewRect = GetViewRect(frame, 1);
		style.innerFrame.draw(viewRect);

		// Horizontal scrollbar.
		if (frame->showScrollbarX)
			DrawHorizontalScrollbar(frame, action);

		// Vertical scrollbar.
		if (frame->showScrollbarY)
			DrawVerticalScrollbar(frame, action);

		// Resize indicator.
		if (frame->showScrollbarX && frame->showScrollbarY)
		{
			Vec2 pos(viewRect.r + 6, viewRect.b + 6);
			style.iconResize.draw(pos, Color(80, 255));
		}
	}

	// Title bar buttons.
	int titleTextW = outer.w();
	int buttonX = outer.r - TitlebarButtonW / 2 - 4;
	if (!frame->isPinned)
	{
		if (frame->isCloseable)
		{
			Color col = Color((action == ActionType::Close) ? 200 : 100, 255);
			style.iconClose.draw(Vec2(buttonX, outer.t + TitlebarH / 2), col);
			buttonX -= TitlebarButtonW;
			titleTextW -= TitlebarButtonW;
		}
		if (frame->isMinimizable)
		{
			auto& spr = frame->isExpanded ? style.iconCollapse : style.iconExpand;
			Color col = Color((action == ActionType::Minimize) ? 200 : 100, 255);
			spr.draw(Vec2(buttonX, outer.t + TitlebarH / 2), col);
			buttonX -= TitlebarButtonW;
			titleTextW -= TitlebarButtonW;
		}
	}
	if (frame->isPinnable)
	{
		auto& spr = frame->isPinned ? style.iconUnpin : style.iconPin;
		Color col = Color((action == ActionType::Pin) ? 200 : 100, 255);
		spr.draw(Vec2(buttonX, outer.t + TitlebarH / 2), col);
		buttonX -= TitlebarButtonW;
		titleTextW -= TitlebarButtonW;
	}

	// Title bar text.
	Text::setStyle(UiText::regular, TextOptions::Markup | TextOptions::Ellipses);
	Text::format(TextAlign::ML, titleTextW - 12, frame->title.data());
	Text::draw(Rect(outer.l + 6, outer.t, outer.l + titleTextW, outer.t + TitlebarH));
}

// =====================================================================================================================
// Event handling.

void Dialog::onMousePress(MousePress& press)
{
	myFrame->currentAction.reset();
	stopCapturingMouse();
	if (isMouseOver())
	{
		auto rect = GetFullRect(myFrame);
		if (press.button == MouseButton::LMB && press.unhandled())
		{
			auto action = GetAction(this, myFrame, press.pos);
			if (action != ActionType::None || rect.contains(press.pos))
			{
				myFrame->requestMoveToTop = 1;
			}
			if (action == ActionType::Drag)
			{
				startCapturingMouse();
				myFrame->currentAction.reset(StartDragAction(myFrame, press.pos));
			}
			else if (action >= ActionType::ResizeBegin && action < ActionType::ResizeEnd)
			{
				startCapturingMouse();
				myFrame->currentAction.reset(StartResizeAction(myFrame, action, press.pos));
			}
			else if (action >= ActionType::ScrollButtonBegin && action < ActionType::ScrollButtonEnd)
			{
				startCapturingMouse();
				myFrame->currentAction.reset(StartScrollButtonAction(action));
			}
			else if (action == ActionType::ScrollDragX)
			{
				startCapturingMouse();
				auto bar = GetHorizontalScrollbarDimensions(myFrame);
				myFrame->currentAction.reset(StartScrollDragAction(bar, press.pos.x, myFrame->scrollOffsetX));
			}
			else if (action == ActionType::ScrollDragY)
			{
				startCapturingMouse();
				auto bar = GetVerticalScrollbarDimensions(myFrame);
				myFrame->currentAction.reset(StartScrollDragAction(bar, press.pos.y, myFrame->scrollOffsetY));
			}
			else if (action == ActionType::Close)
			{
				myFrame->requestClose = 1;
			}
			else if (action == ActionType::Minimize)
			{
				myFrame->requestMinimize = 1;
			}
			else if (action == ActionType::Pin)
			{
				myFrame->requestPin = 1;
			}
		}
		press.setHandled();
	}
}

void Dialog::onMouseRelease(MouseRelease& release)
{
	myFrame->currentAction.reset();
	stopCapturingMouse();
}

static void UpdateMouseCursor(Dialog* dialog, DialogFrame* frame)
{
	Vec2 mpos = Window::getMousePos();
	auto action = GetAction(dialog, frame, mpos);
	switch (action)
	{
	case ActionType::ResizeL:
	case ActionType::ResizeR:
		Window::setCursor(Window::CursorIcon::SizeWE);
		break;

	case ActionType::ResizeB:
	case ActionType::ResizeT:
		Window::setCursor(Window::CursorIcon::SizeNS);
		break;

	case ActionType::ResizeTL:
	case ActionType::ResizeBR:
		Window::setCursor(Window::CursorIcon::SizeNWSE);
		break;

	case ActionType::ResizeTR:
	case ActionType::ResizeBL:
		Window::setCursor(Window::CursorIcon::SizeNESW);
		break;

	default:
		break;
	};
}

// =====================================================================================================================
// Dialog :: implementation.

Dialog::Dialog()
	: myFrame(new DialogFrame)
{
}

Dialog::~Dialog()
{
	delete myFrame;
}

void Dialog::onUpdateSize()
{
}

UiElement* Dialog::findMouseOver(int x, int y)
{
	Rect captureRegion = GetFullRect(myFrame).expand(ResizeMargin);
	if (captureRegion.contains(x, y))
	{
		if (GetViewRect(myFrame, 2).contains(x, y))
		{
			if (auto result = findContentMouseOver(x, y))
				return result;
		}
		return this;
	}
	return nullptr;
}

void Dialog::arrange()
{
	if (myFrame->requestMinimize)
	{
		myFrame->requestMinimize = 0;
		myFrame->isExpanded = ~myFrame->isExpanded;
		if (!myFrame->isExpanded) myFrame->isPinned = 0;
	}

	if (myFrame->requestPin)
	{
		myFrame->requestPin = 0;
		myFrame->isPinned = ~myFrame->isPinned;

		if (myFrame->isPinned)
			myFrame->isExpanded = 1;
	}

	onUpdateSize();

	auto action = myFrame->currentAction.get();
	if (action)
	{
		HandleDragAction(action, myFrame);
		HandleResizeAction(action, myFrame);
	}
	HandleFrameConstraints(myFrame);

	if (action)
	{
		HandleScrollButtonAction(action, myFrame);
		HandleScrollDragAction(action, myFrame);
	}
	HandleScrollConstraints(myFrame);
}

void Dialog::tick(bool enabled)
{
	// Dialog frame.
	Vec2 mpos = Window::getMousePos();

	if (isMouseOver())
		UpdateMouseCursor(this, myFrame);

	// Update the tooltip.
	auto action = GetAction(this, myFrame, mpos);
	switch (action)
	{
	case ActionType::Close:
		setTooltip("Close the dialog");
		break;
	case ActionType::Minimize:
		setTooltip(myFrame->isExpanded ? "Hide the dialog contents" : "Show the dialog contents");
		break;
	case ActionType::Pin:
		setTooltip(myFrame->isPinned ? "Unpin the dialog" : "Pin the dialog");
		break;
	}

	// Dialog contents.
	if (myFrame->isExpanded)
		tickContent(enabled);
}

void Dialog::draw(bool enabled)
{
	// Dialog contents.
	DrawFrame(this, myFrame);
	if (myFrame->isExpanded)
	{
		auto clipRect = GetViewRect(myFrame, 2);
		if (myFrame->showScrollbarX || myFrame->showScrollbarY)
		{
			Renderer::pushScissorRect(clipRect);
			drawContent(enabled);
			Renderer::popScissorRect();
		}
		else
		{
			drawContent(enabled);
		}
	}
}

void Dialog::requestClose()
{
	myFrame->requestClose = 1;
}

void Dialog::requestPin()
{
	myFrame->requestPin = 1;
}

void Dialog::setPosition(int x, int y)
{
	myFrame->x = x;
	myFrame->y = y;
}

void Dialog::setTitle(stringref str)
{
	myFrame->title = str;
}

void Dialog::setMargin(int margin)
{
	myFrame->margin = margin;
}

void Dialog::setWidth(int width)
{
	myFrame->width = width;
}

void Dialog::setHeight(int height)
{
	myFrame->height = height;
}

void Dialog::setMinimumSize(int w, int h)
{
	myFrame->minSize = { w, h };
}

void Dialog::setPreferredSize(int w, int h)
{
	myFrame->prefSize = { w, h };
}

void Dialog::setResizeBehaviorH(ResizeBehavior behaviour)
{
	myFrame->resizeBehaviourX = (uint)behaviour;
}

void Dialog::setResizeBehaviorV(ResizeBehavior behaviour)
{
	myFrame->resizeBehaviourY = (uint)behaviour;
}

void Dialog::setCloseable(bool enable)
{
	myFrame->isCloseable = 1;
}

void Dialog::setMinimizable(bool enable)
{
	myFrame->isMinimizable = 1;
}

Size2 Dialog::getMinimumSize() const
{
	return myFrame->minSize;
}

Size2 Dialog::getPreferredSize() const
{
	return myFrame->prefSize;
}

Rect Dialog::getContentRect() const
{
	return GetContentRect(myFrame);
}

bool Dialog::isPinned() const
{
	return myFrame->isPinned;
}

void Dialog::resetCloseRequest() const
{
	myFrame->requestClose = 0;
}

bool Dialog::hasCloseRequest() const
{
	return myFrame->requestClose;
}

} // namespace AV
