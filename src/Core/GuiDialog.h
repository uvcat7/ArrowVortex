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

	GuiDialog* dialogPtr_;
	GuiContext* gui_;

	uint IsPinnable_ : 1;
	uint IsCloseable_ : 1;
	uint isMinimizable_ : 1;
	uint isHorizontallyResizable_ : 1;
	uint isVerticallyResizable_ : 1;

	uint requestClose_ : 1;
	uint requestPin_ : 1;
	uint requestMinimize_ : 1;
	uint requestMoveToTop_ : 1;

	uint pinnedState_ : 1;
	uint minimizedState_ : 1;

private:
	friend class GuiDialog;

	void ClampRect_();
	void HandleResize_();
	void HandleDrag_();
	void UpdateMouseCursor_();

	ActionType myGetAction(int x, int y) const; // TODO finish cleanup of names starting with "my"
	void myFinishActions();

	vec2i minSize_;
	vec2i maxSize_;
	vec2i pinnedPosition_;

	String dialogTitle_;

	BaseAction* currentAction_;
};

}; // namespace Vortex
