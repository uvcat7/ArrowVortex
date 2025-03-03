#include <Core/GuiDialog.h>
#include <Core/GuiContext.h>
#include <Core/Utils.h>
#include <Core/GuiDraw.h>
#include <Core/GuiContext.h>
#include <Core/GuiWidget.h>

namespace Vortex {

#define MY_GUI ((GuiContextImpl*)myGui)

static const int FRAME_TITLEBAR_H = 24;
static const int FRAME_PADDING = 4;
static const int FRAME_RESIZE_BORDER = 5;
static const int FRAME_BUTTON_W = 16;

// ================================================================================================
// Dialog Frame implementation.

DialogData::~DialogData()
{
	delete myDialog;
	delete myAction;
}

DialogData::DialogData(GuiContext* gui, GuiDialog* dialog)
	: GuiWidget(gui)
	, myDialog(dialog)
	, myGui(gui)
	, myIsPinnable(1)
	, myIsCloseable(1)
	, myIsMinimizable(1)
	, myIsResizeableH(0)
	, myIsResizeableV(0)
	, myRequestClose(0)
	, myRequestPin(0)
	, myRequestMinimize(0)
	, myRequestMoveToTop(0)
	, myIsPinned(0)
	, myIsMinimized(0)
	, myMinSize({0, 0})
	, myMaxSize({INT_MAX, INT_MAX})
	, myPinnedPos({0, 0})
	, myAction(nullptr)
{
	myRect = {16, 16, 256, 256};
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

void DialogData::myHandleDrag()
{
	if(myAction && myAction->type == ACT_DRAG)
	{
		auto action = (DragAction*)myAction;
		vec2i mpos = myGui->getMousePos();

		myRect.x = mpos.x + action->offset.x;
		myRect.y = mpos.y + action->offset.y;
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

void DialogData::myHandleResize()
{
	if(myAction && myAction->type >= ACT_RESIZE)
	{
		auto action = (ResizeAction*)myAction;
		vec2i mpos = myGui->getMousePos();
		vec2i anchor = action->anchor;

		if(action->dirH < 0)
		{
			myRect.w = anchor.x - mpos.x + action->offset.x;
			myRect.x = anchor.x - myRect.w;

		}
		else if(action->dirH > 0)
		{
			myRect.x = anchor.x;
			myRect.w = mpos.x - anchor.x + action->offset.x;
		}

		if(action->dirV < 0)
		{
			myRect.h = anchor.y - mpos.y + action->offset.y;
			myRect.y = anchor.y - myRect.h;

		}
		else if(action->dirV > 0)
		{
			myRect.y = anchor.y;
			myRect.h = mpos.y - anchor.y + action->offset.y;
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

			if(actionType != ACT_NONE || IsInside(myRect, evt.x, evt.y))
			{
				myRequestMoveToTop = 1;
			}
			if(actionType == ACT_DRAG)
			{
				startCapturingMouse();
				myAction = StartDrag(myRect, evt.x, evt.y);
			}
			else if(actionType >= ACT_RESIZE)
			{
				startCapturingMouse();
				myAction = StartResize(actionType, myRect, evt.x, evt.y);
			}
			else if(actionType == ACT_CLOSE)
			{
				myRequestClose = 1;
			}
			else if(actionType == ACT_MINIMIZE)
			{
				myRequestMinimize = 1;
			}
			else if(actionType == ACT_PIN)
			{
				myRequestPin = 1;
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

void DialogData::myClampRect()
{
	recti bounds = Shrink(myGui->getView(),
		FRAME_PADDING, FRAME_PADDING + FRAME_TITLEBAR_H, FRAME_PADDING, FRAME_PADDING);

	if(myIsPinned)
	{
		myRect.x = myPinnedPos.x;
		myRect.y = myPinnedPos.y;
	}

	if(myAction && myAction->type >= ACT_RESIZE)
	{
		auto a = (ResizeAction*)myAction;
		if(a->dirH < 0) myRect.w = min(myRect.w, a->anchor.x - bounds.x);
		if(a->dirH > 0) myRect.w = min(myRect.w, bounds.x + bounds.w - a->anchor.x);
		if(a->dirV < 0) myRect.h = min(myRect.h, a->anchor.y - bounds.y);
		if(a->dirV > 0) myRect.h = min(myRect.h, bounds.y + bounds.h - a->anchor.y);
	}

	myRect.w = max(myMinSize.x, min(myMaxSize.x, min(bounds.w, myRect.w)));
	myRect.h = max(myMinSize.y, min(myMaxSize.y, min(bounds.h, myRect.h)));

	if(myAction && myAction->type >= ACT_RESIZE)
	{
		auto a = (ResizeAction*)myAction;
		if(a->dirH < 0) myRect.x = a->anchor.x - myRect.w;
		if(a->dirV < 0) myRect.y = a->anchor.y - myRect.h;
	}

	int marginH = myIsMinimized ? (FRAME_PADDING * -2) : myRect.h;
	myRect.x = max(min(myRect.x, bounds.x + bounds.w - myRect.w), bounds.x);
	myRect.y = max(min(myRect.y, bounds.y + bounds.h - marginH), bounds.y);
}

void DialogData::arrange()
{
	if(myRequestMinimize)
	{
		myRequestMinimize = 0;
		myIsMinimized = !myIsMinimized;
		if(myIsMinimized) myIsPinned = 0;
	}
	if(myRequestPin)
	{
		myRequestPin = 0;
		myIsPinned = !myIsPinned;
		if(myIsPinned)
		{
			myIsMinimized = 0;
			myPinnedPos = Pos(myRect);
		}
	}

	myDialog->onUpdateSize();
	myHandleResize();
	myHandleDrag();
	myClampRect();
}

void DialogData::myUpdateMouseCursor()
{
	if(!isMouseOver()) return;

	vec2i mpos = myGui->getMousePos();
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
	if(!myIsMinimized)
	{
		myDialog->onTick();
	}

	vec2i mpos = myGui->getMousePos();
	recti rect = myDialog->getOuterRect();
	rect = Expand(rect, FRAME_RESIZE_BORDER);

	if(IsInside(rect, mpos.x, mpos.y))
	{
		captureMouseOver();
	}

	handleInputs(myGui->getEvents());

	myUpdateMouseCursor();
}

void DialogData::draw()
{
	auto& dlg = GuiDraw::getDialog();

	vec2i mpos = myGui->getMousePos();
	auto action = myGetAction(mpos.x, mpos.y);
	recti r = myDialog->getOuterRect();

	// Draw the frame.
	if(myIsPinned)
	{
		dlg.frame.draw({r.x, r.y, r.w, r.h}, 0);
		recti tb = Shrink(r, 2);
		tb.h = FRAME_TITLEBAR_H - 2;
		Draw::fill(tb, COLOR32(0, 0, 0, 48));
	}
	else
	{
		if(!myIsMinimized)
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
	if(!myIsPinned)
	{
		if(myIsCloseable)
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
		if(myIsMinimizable)
		{
			auto& tex = myIsMinimized ? icons.plus : icons.minus;
			color32 col = Color32((action == ACT_MINIMIZE) ? 200 : 100);
			Draw::sprite(tex, {buttonX, r.y + FRAME_TITLEBAR_H / 2}, col);
			buttonX -= FRAME_BUTTON_W;
			titleTextW -= FRAME_BUTTON_W;
			if(action == ACT_MINIMIZE)
			{
				GuiMain::setTooltip(myIsMinimized ? "Show the dialog contents" : "Hide the dialog contents");
			}
		}
	}
	if(myIsPinnable)
	{
		auto& tex = myIsPinned ? icons.unpin : icons.pin;
		color32 col = Color32((action == ACT_PIN) ? 200 : 100);
		Draw::sprite(tex, {buttonX, r.y + FRAME_TITLEBAR_H / 2}, col);
		buttonX -= FRAME_BUTTON_W;
		titleTextW -= FRAME_BUTTON_W;
		if(action == ACT_PIN)
		{
			GuiMain::setTooltip(myIsPinned ? "Unpin the dialog" : "Pin the dialog");
		}
	}
	
	// Draw the title text.
	TextStyle style;
	style.textFlags = Text::MARKUP | Text::ELLIPSES;
	Text::arrange(Text::MC, style, titleTextW - 8, myTitle.str());
	Text::draw({r.x, r.y, titleTextW, FRAME_TITLEBAR_H});

	// Draw inner dialog area.
	if(!myIsMinimized)
	{
		myDialog->onDraw();
	}
}

DialogData::ActionType DialogData::myGetAction(int x, int y) const
{
	if(myAction)
	{
		return myAction->type;
	}
	else
	{
		recti rect = myDialog->getOuterRect();
		if(IsInside(rect, x, y))
		{
			// Titlebar buttons.
			int dx = x - rect.x - rect.w + FRAME_BUTTON_W, dy = y - rect.y;
			if(dy < FRAME_TITLEBAR_H)
			{
				if(!myIsPinned)
				{
					if(myIsCloseable)
					{
						if(dx >= 0) return ACT_CLOSE;
						dx += FRAME_BUTTON_W;
					}
					if(myIsMinimizable)
					{
						if(dx >= 0) return ACT_MINIMIZE;
						dx += FRAME_BUTTON_W;
					}
				}
				if(myIsPinnable)
				{
					if(dx >= 0) return ACT_PIN;
					dx += FRAME_BUTTON_W;
				}
				return myIsPinned ? ACT_NONE : ACT_DRAG;
			}
		}

		// Horizontal and vertical resizing.
		rect = Expand(rect, FRAME_RESIZE_BORDER);
		if(IsInside(rect, x, y) && !myIsPinned)
		{
			int resizeH = 0, resizeV = 0;
			int dx = x - rect.x, dy = y - rect.y;
			int border = FRAME_RESIZE_BORDER * 2;
			if(myIsResizeableH)
			{
				resizeH = (dx > rect.w - border) - (dx < border);
			}
			if(myIsResizeableV)
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
	delete myAction;
	myAction = nullptr;
}

// ================================================================================================
// GuiDialog :: implementation.

#define DATA ((DialogData*)myData)

GuiDialog::GuiDialog(GuiContext* gui)
	: myData(new DialogData(gui, this))
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
	DATA->myRequestClose = 1;
}

void GuiDialog::requestPin()
{
	DATA->myRequestPin = 1;
}

void GuiDialog::setPosition(int x, int y)
{
	DATA->myRect.x = x;
	DATA->myRect.y = y;
}

void GuiDialog::setTitle(StringRef str)
{
	DATA->myTitle = str;
}

void GuiDialog::setWidth(int w)
{
	DATA->myRect.w = w;
}

void GuiDialog::setHeight(int h)
{
	DATA->myRect.h = h;
}

void GuiDialog::setMinimumWidth(int w)
{
	DATA->myMinSize.x = max(0, w);
}

void GuiDialog::setMinimumHeight(int h)
{
	DATA->myMinSize.y = max(0, h);
}

void GuiDialog::setMaximumWidth(int w)
{
	DATA->myMaxSize.x = max(0, w);
}

void GuiDialog::setMaximumHeight(int h)
{
	DATA->myMaxSize.y = max(0, h);
}

void GuiDialog::setCloseable(bool enable)
{
	DATA->myIsCloseable = 1;
}

void GuiDialog::setMinimizable(bool enable)
{
	DATA->myIsMinimizable = 1;
}

void GuiDialog::setResizeable(bool horizontal, bool vertical)
{
	DATA->myIsResizeableH = horizontal;
	DATA->myIsResizeableV = vertical;
}

GuiContext* GuiDialog::getGui() const
{
	return DATA->myGui;
}

recti GuiDialog::getOuterRect() const
{
	int pad = FRAME_PADDING;
	recti r = Expand(DATA->myRect, pad, pad + FRAME_TITLEBAR_H, pad, pad);
	if(DATA->myIsMinimized) r.h = FRAME_TITLEBAR_H + pad;
	return r;
}

recti GuiDialog::getInnerRect() const
{
	return DATA->myRect;
}

bool GuiDialog::isPinned() const
{
	return DATA->myIsPinned;
}

}; // namespace Vortex
