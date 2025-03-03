#pragma once

#include <Core/Widgets.h>

namespace Vortex {

class DialogData : public GuiWidget
{
public:
	enum ActionType
	{
		ACT_NONE,
		ACT_DRAG,
		ACT_CLOSE,
		ACT_MINIMIZE,
		ACT_PIN,
		ACT_RESIZE,
		ACT_RESIZE_L,
		ACT_RESIZE_T,
		ACT_RESIZE_R,
		ACT_RESIZE_B,
		ACT_RESIZE_BL,
		ACT_RESIZE_BR,
		ACT_RESIZE_TL,
		ACT_RESIZE_TR,
	};
	struct BaseAction
	{
		ActionType type;
	};
	struct DragAction : public BaseAction
	{
		vec2i offset;
	};
	struct ResizeAction : public BaseAction
	{
		vec2i anchor, offset;
		int dirH, dirV;
	};

	DialogData(GuiContext* gui, GuiDialog* dialog);
	~DialogData();

	void onMousePress(MousePress& evt) override;
	void onMouseRelease(MouseRelease& evt) override;

	void arrange();
	void tick();
	void draw();

	GuiDialog* myDialog;
	GuiContext* myGui;

	uint myIsPinnable : 1;
	uint myIsCloseable : 1;
	uint myIsMinimizable : 1;
	uint myIsResizeableH : 1;
	uint myIsResizeableV : 1;

	uint myRequestClose : 1;
	uint myRequestPin : 1;
	uint myRequestMinimize : 1;
	uint myRequestMoveToTop : 1;

	uint myIsPinned : 1;
	uint myIsMinimized : 1;

private:
	friend class GuiDialog;

	void myClampRect();
	void myHandleResize();
	void myHandleDrag();
	void myUpdateMouseCursor();

	ActionType myGetAction(int x, int y) const;
	void myFinishActions();

	vec2i myMinSize;
	vec2i myMaxSize;
	vec2i myPinnedPos;

	String myTitle;

	BaseAction* myAction;
};

}; // namespace Vortex
