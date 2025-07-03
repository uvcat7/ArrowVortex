#include <Core/GuiDialog.h>
#include <Core/GuiContext.h>
#include <Core/Utils.h>
#include <Core/GuiDraw.h>
#include <Core/GuiContext.h>
#include <Core/GuiWidget.h>

namespace Vortex {

#define MY_GUI ((GuiContextImpl*)gui_)

static const int FRAME_TITLEBAR_H = 24;
static const int FRAME_PADDING = 4;
static const int FRAME_RESIZE_BORDER = 5;
static const int FRAME_BUTTON_W = 16;

// ================================================================================================
// Dialog Frame implementation.

DialogData::~DialogData()
{
	delete dialogPtr_;
	delete currentAction_;
}

DialogData::DialogData(GuiContext* gui, GuiDialog* dialog)
	: GuiWidget(gui)
	, dialogPtr_(dialog)
	, gui_(gui)
	, IsPinnable_(1)
	, IsCloseable_(1)
	, isMinimizable_(1)
	, isHorizontallyResizable_(0)
	, isVerticallyResizable_(0)
	, requestClose_(0)
	, requestPin_(0)
	, requestMinimize_(0)
	, requestMoveToTop_(0)
	, pinnedState_(0)
	, minimizedState_(0)
	, minSize_({0, 0})
	, maxSize_({INT_MAX, INT_MAX})
	, pinnedPosition_({0, 0})
	, currentAction_(nullptr)
{
	rect_ = {16, 16, 256, 256};
	MY_GUI->addDialog(this);
}

// ================================================================================================
// DialogData :: frame dragging.

static DialogData::DragAction* StartDrag(recti r, int mx, int my)
{
	auto out = new DialogData::DragAction;
	
	out->type = DialogData::ACT_DRAG;
	out->offset.x = r.x - mx;
	out->offset.y = r.y - my;

	return out;
}

void DialogData::HandleDrag_()
{
	if(currentAction_ && currentAction_->type == ACT_DRAG)
	{
		auto action = (DragAction*)currentAction_;
		vec2i mpos = gui_->getMousePos();

		rect_.x = mpos.x + action->offset.x;
		rect_.y = mpos.y + action->offset.y;
	}
}

// ================================================================================================
// DialogData :: frame resizing.

static DialogData::ResizeAction* StartResize(DialogData::ActionType type, recti r, int mx, int my)
{
	auto out = new DialogData::ResizeAction;
	out->type = type;

	static const int dirH[] = {0, -1, 0, 1, 0, -1, 1, -1, 1};
	static const int dirV[] = {0, 0, -1, 0, 1, 1, 1, -1, -1};

	out->dirH = dirH[type - DialogData::ACT_RESIZE];
	out->dirV = dirV[type - DialogData::ACT_RESIZE];

	if(out->dirH < 0)
	{
		out->anchor.x = r.x + r.w;
		out->offset.x = mx - r.x;
	}
	else
	{
		out->anchor.x = r.x;
		out->offset.x = out->anchor.x + r.w - mx;
	}

	if(out->dirV < 0)
	{
		out->anchor.y = r.y + r.h;
		out->offset.y = my - r.y;
	}
	else
	{
		out->anchor.y = r.y;
		out->offset.y = out->anchor.y + r.h - my;
	}

	return out;
}

void DialogData::HandleResize_()
{
	if(currentAction_ && currentAction_->type >= ACT_RESIZE)
	{
		auto action = (ResizeAction*)currentAction_;
		vec2i mpos = gui_->getMousePos();
		vec2i anchor = action->anchor;

		if(action->dirH < 0)
		{
			rect_.w = anchor.x - mpos.x + action->offset.x;
			rect_.x = anchor.x - rect_.w;

		}
		else if(action->dirH > 0)
		{
			rect_.x = anchor.x;
			rect_.w = mpos.x - anchor.x + action->offset.x;
		}

		if(action->dirV < 0)
		{
			rect_.h = anchor.y - mpos.y + action->offset.y;
			rect_.y = anchor.y - rect_.h;

		}
		else if(action->dirV > 0)
		{
			rect_.y = anchor.y;
			rect_.h = mpos.y - anchor.y + action->offset.y;
		}
	}
}

// ================================================================================================
// DialogData :: event handling.

void DialogData::onMousePress(MousePress& evt)
{
	myFinishActions();
	stopCapturingMouse();
	if(isMouseOver())
	{
		if(evt.button == Mouse::LMB && evt.unhandled())
		{
			auto actionType = myGetAction(evt.x, evt.y);

			if(actionType != ACT_NONE || IsInside(rect_, evt.x, evt.y))
			{
				requestMoveToTop_ = 1;
			}
			if(actionType == ACT_DRAG)
			{
				startCapturingMouse();
				currentAction_ = StartDrag(rect_, evt.x, evt.y);
			}
			else if(actionType >= ACT_RESIZE)
			{
				startCapturingMouse();
				currentAction_ = StartResize(actionType, rect_, evt.x, evt.y);
			}
			else if(actionType == ACT_CLOSE)
			{
				requestClose_ = 1;
			}
			else if(actionType == ACT_MINIMIZE)
			{
				requestMinimize_ = 1;
			}
			else if(actionType == ACT_PIN)
			{
				requestPin_ = 1;
			}
		}
		evt.setHandled();
	}
}

void DialogData::onMouseRelease(MouseRelease& evt)
{
	myFinishActions();
	stopCapturingMouse();
}

// ================================================================================================
// DialogData :: update functions.

void DialogData::ClampRect_()
{
	recti bounds = Shrink(gui_->getView(),
		FRAME_PADDING, FRAME_PADDING + FRAME_TITLEBAR_H, FRAME_PADDING, FRAME_PADDING);

	if(pinnedState_)
	{
		rect_.x = pinnedPosition_.x;
		rect_.y = pinnedPosition_.y;
	}

	if(currentAction_ && currentAction_->type >= ACT_RESIZE)
	{
		auto a = (ResizeAction*)currentAction_;
		if(a->dirH < 0) rect_.w = min(rect_.w, a->anchor.x - bounds.x);
		if(a->dirH > 0) rect_.w = min(rect_.w, bounds.x + bounds.w - a->anchor.x);
		if(a->dirV < 0) rect_.h = min(rect_.h, a->anchor.y - bounds.y);
		if(a->dirV > 0) rect_.h = min(rect_.h, bounds.y + bounds.h - a->anchor.y);
	}

	rect_.w = max(minSize_.x, min(maxSize_.x, min(bounds.w, rect_.w)));
	rect_.h = max(minSize_.y, min(maxSize_.y, min(bounds.h, rect_.h)));

	if(currentAction_ && currentAction_->type >= ACT_RESIZE)
	{
		auto a = (ResizeAction*)currentAction_;
		if(a->dirH < 0) rect_.x = a->anchor.x - rect_.w;
		if(a->dirV < 0) rect_.y = a->anchor.y - rect_.h;
	}

	int marginH = minimizedState_ ? (FRAME_PADDING * -2) : rect_.h;
	rect_.x = max(min(rect_.x, bounds.x + bounds.w - rect_.w), bounds.x);
	rect_.y = max(min(rect_.y, bounds.y + bounds.h - marginH), bounds.y);
}

void DialogData::arrange()
{
	if(requestMinimize_)
	{
		requestMinimize_ = 0;
		minimizedState_ = !minimizedState_;
		if(minimizedState_) pinnedState_ = 0;
	}
	if(requestPin_)
	{
		requestPin_ = 0;
		pinnedState_ = !pinnedState_;
		if(pinnedState_)
		{
			minimizedState_ = 0;
			pinnedPosition_ = Pos(rect_);
		}
	}

	dialogPtr_->onUpdateSize();
	HandleResize_();
	HandleDrag_();
	ClampRect_();
}

void DialogData::UpdateMouseCursor_()
{
	if(!isMouseOver()) return;

	vec2i mpos = gui_->getMousePos();
	Cursor::Icon icon = Cursor::ARROW;
	switch(myGetAction(mpos.x, mpos.y))
	{
	case ACT_RESIZE_L:
	case ACT_RESIZE_R:
		icon = Cursor::SIZE_WE; break;
	case ACT_RESIZE_B:
	case ACT_RESIZE_T:
		icon = Cursor::SIZE_NS; break;
	case ACT_RESIZE_TL:
	case ACT_RESIZE_BR:
		icon = Cursor::SIZE_NWSE; break;
	case ACT_RESIZE_TR:
	case ACT_RESIZE_BL:
		icon = Cursor::SIZE_NESW; break;
	default: break;
	};

	GuiMain::setCursorIcon(icon);
}

void DialogData::tick()
{
	if(!minimizedState_)
	{
		dialogPtr_->onTick();
	}

	vec2i mpos = gui_->getMousePos();
	recti rect = dialogPtr_->getOuterRect();
	rect = Expand(rect, FRAME_RESIZE_BORDER);

	if(IsInside(rect, mpos.x, mpos.y))
	{
		captureMouseOver();
	}

	handleInputs(gui_->getEvents());

	UpdateMouseCursor_();
}

void DialogData::draw()
{
	auto& dlg = GuiDraw::getDialog();

	vec2i mpos = gui_->getMousePos();
	auto action = myGetAction(mpos.x, mpos.y);
	recti r = dialogPtr_->getOuterRect();

	// Draw the frame.
	if(pinnedState_)
	{
		dlg.frame.draw({r.x, r.y, r.w, r.h}, 0);
		recti tb = Shrink(r, 2);
		tb.h = FRAME_TITLEBAR_H - 2;
		Draw::fill(tb, COLOR32(0, 0, 0, 48));
	}
	else
	{
		if(!minimizedState_)
		{
			dlg.titlebar.draw({r.x, r.y, r.w, FRAME_TITLEBAR_H + 8}, TileRect2::T);
			dlg.frame.draw({r.x, r.y + FRAME_TITLEBAR_H, r.w, r.h - FRAME_TITLEBAR_H}, TileRect2::B);
		}
		else
		{
			dlg.titlebar.draw({r.x, r.y, r.w, FRAME_TITLEBAR_H});
		}
	}

	// Draw the titlebar buttons.
	auto& icons = GuiDraw::getIcons();
	int titleTextW = r.w;
	int buttonX = RightX(r) - FRAME_BUTTON_W / 2 - 4;
	if(!pinnedState_)
	{
		if(IsCloseable_)
		{
			color32 col = Color32((action == ACT_CLOSE) ? 200 : 100);
			Draw::sprite(icons.cross, {buttonX, r.y + FRAME_TITLEBAR_H / 2}, col);
			buttonX -= FRAME_BUTTON_W;
			titleTextW -= FRAME_BUTTON_W;
			if(action == ACT_CLOSE)
			{
				GuiMain::setTooltip("Close the dialog");
			}
		}
		if(isMinimizable_)
		{
			auto& tex = minimizedState_ ? icons.plus : icons.minus;
			color32 col = Color32((action == ACT_MINIMIZE) ? 200 : 100);
			Draw::sprite(tex, {buttonX, r.y + FRAME_TITLEBAR_H / 2}, col);
			buttonX -= FRAME_BUTTON_W;
			titleTextW -= FRAME_BUTTON_W;
			if(action == ACT_MINIMIZE)
			{
				GuiMain::setTooltip(minimizedState_ ? "Show the dialog contents" : "Hide the dialog contents");
			}
		}
	}
	if(IsPinnable_)
	{
		auto& tex = pinnedState_ ? icons.unpin : icons.pin;
		color32 col = Color32((action == ACT_PIN) ? 200 : 100);
		Draw::sprite(tex, {buttonX, r.y + FRAME_TITLEBAR_H / 2}, col);
		buttonX -= FRAME_BUTTON_W;
		titleTextW -= FRAME_BUTTON_W;
		if(action == ACT_PIN)
		{
			GuiMain::setTooltip(pinnedState_ ? "Unpin the dialog" : "Pin the dialog");
		}
	}
	
	// Draw the title text.
	TextStyle style;
	style.textFlags = Text::MARKUP | Text::ELLIPSES;
	Text::arrange(Text::MC, style, titleTextW - 8, dialogTitle_.str());
	Text::draw({r.x, r.y, titleTextW, FRAME_TITLEBAR_H});

	// Draw inner dialog area.
	if(!minimizedState_)
	{
		dialogPtr_->onDraw();
	}
}

DialogData::ActionType DialogData::myGetAction(int x, int y) const
{
	if(currentAction_)
	{
		return currentAction_->type;
	}
	else
	{
		recti rect = dialogPtr_->getOuterRect();
		if(IsInside(rect, x, y))
		{
			// Titlebar buttons.
			int dx = x - rect.x - rect.w + FRAME_BUTTON_W, dy = y - rect.y;
			if(dy < FRAME_TITLEBAR_H)
			{
				if(!pinnedState_)
				{
					if(IsCloseable_)
					{
						if(dx >= 0) return ACT_CLOSE;
						dx += FRAME_BUTTON_W;
					}
					if(isMinimizable_)
					{
						if(dx >= 0) return ACT_MINIMIZE;
						dx += FRAME_BUTTON_W;
					}
				}
				if(IsPinnable_)
				{
					if(dx >= 0) return ACT_PIN;
					dx += FRAME_BUTTON_W;
				}
				return pinnedState_ ? ACT_NONE : ACT_DRAG;
			}
		}

		// Horizontal and vertical resizing.
		rect = Expand(rect, FRAME_RESIZE_BORDER);
		if(IsInside(rect, x, y) && !pinnedState_)
		{
			int resizeH = 0, resizeV = 0;
			int dx = x - rect.x, dy = y - rect.y;
			int border = FRAME_RESIZE_BORDER * 2;
			if(isHorizontallyResizable_)
			{
				resizeH = (dx > rect.w - border) - (dx < border);
			}
			if(isVerticallyResizable_)
			{
				resizeV = (dy > rect.h - border) - (dy < border);
			}
			if(resizeH != 0 || resizeV != 0)
			{
				int index = resizeV * 3 + resizeH + 4;
				ActionType actions[] =
				{
					ACT_RESIZE_TL, ACT_RESIZE_T, ACT_RESIZE_TR,
					ACT_RESIZE_L, ACT_RESIZE, ACT_RESIZE_R,
					ACT_RESIZE_BL, ACT_RESIZE_B, ACT_RESIZE_BR
				};
				return actions[index];
			}
		}
	}
	return ACT_NONE;
}

void DialogData::myFinishActions()
{
	delete currentAction_;
	currentAction_ = nullptr;
}

// ================================================================================================
// GuiDialog :: implementation.

#define DATA ((DialogData*)data_)

GuiDialog::GuiDialog(GuiContext* gui)
	: data_(new DialogData(gui, this))
{
}

GuiDialog::~GuiDialog()
{
}

void GuiDialog::onUpdateSize()
{
}

void GuiDialog::onArrange(recti r)
{
}

void GuiDialog::onTick()
{
}

void GuiDialog::onDraw()
{
}

void GuiDialog::requestClose()
{
	DATA->requestClose_ = 1;
}

void GuiDialog::requestPin()
{
	DATA->requestPin_ = 1;
}

void GuiDialog::setPosition(int x, int y)
{
	DATA->rect_.x = x;
	DATA->rect_.y = y;
}

void GuiDialog::setTitle(StringRef str)
{
	DATA->dialogTitle_ = str;
}

void GuiDialog::setWidth(int w)
{
	DATA->rect_.w = w;
}

void GuiDialog::setHeight(int h)
{
	DATA->rect_.h = h;
}

void GuiDialog::setMinimumWidth(int w)
{
	DATA->minSize_.x = max(0, w);
}

void GuiDialog::setMinimumHeight(int h)
{
	DATA->minSize_.y = max(0, h);
}

void GuiDialog::setMaximumWidth(int w)
{
	DATA->maxSize_.x = max(0, w);
}

void GuiDialog::setMaximumHeight(int h)
{
	DATA->maxSize_.y = max(0, h);
}

void GuiDialog::setCloseable(bool enable)
{
	DATA->IsCloseable_ = 1;
}

void GuiDialog::setMinimizable(bool enable)
{
	DATA->isMinimizable_ = 1;
}

void GuiDialog::setResizeable(bool horizontal, bool vertical)
{
	DATA->isHorizontallyResizable_ = horizontal;
	DATA->isVerticallyResizable_ = vertical;
}

GuiContext* GuiDialog::getGui() const
{
	return DATA->gui_;
}

recti GuiDialog::getOuterRect() const
{
	int pad = FRAME_PADDING;
	recti r = Expand(DATA->rect_, pad, pad + FRAME_TITLEBAR_H, pad, pad);
	if(DATA->minimizedState_) r.h = FRAME_TITLEBAR_H + pad;
	return r;
}

recti GuiDialog::getInnerRect() const
{
	return DATA->rect_;
}

bool GuiDialog::isPinned() const
{
	return DATA->pinnedState_;
}

}; // namespace Vortex
