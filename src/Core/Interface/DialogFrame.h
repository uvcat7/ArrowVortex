#pragma once

#include <Core/Types/Rect.h>

namespace AV {

struct DialogFrame
{
	enum class ActionType
	{
		None,

		// Resizing by grabbing the dialog frame.
		ResizeBegin,
		ResizeT = ResizeBegin,
		ResizeTR,
		ResizeR,
		ResizeBR,
		ResizeB,
		ResizeBL,
		ResizeL,
		ResizeTL,
		ResizeEnd,

		// Scrolling the dialog by pressing a scroll button.
		ScrollButtonBegin,
		ScrollButtonU = ScrollButtonBegin,
		ScrollButtonR,
		ScrollButtonD,
		ScrollButtonL,
		ScrollButtonEnd,

		// Scrolling the dialog by dragging a scroll bar.
		ScrollDragBegin,
		ScrollDragX = ScrollDragBegin,
		ScrollDragY,
		ScrollDragEnd,

		Drag,
		Close,
		Minimize,
		Pin,
	};

	struct BaseAction
	{
		ActionType type = ActionType::None;
	};

	struct DragAction : public BaseAction
	{
		// Position that was clicked when dragging started, relative to the frame.
		Vec2 grabOffset;
	};

	struct ResizeAction : public BaseAction
	{
		enum class HorizontalAction { none, left, right };

		enum class VerticalAction { none, up, down };

		// Position where the resize is anchored. Corresponds to either a corner of the frame (when resizing in two
		// directions), or the center of a side (when resizing in one direction).
		Vec2 anchor;

		// The horizontal direction in which the resize is happening.
		HorizontalAction horizontalAction = HorizontalAction::none;

		// The vertical direction in which the resize is happening.
		VerticalAction verticalAction = VerticalAction::none;

		// Position that was clicked when resizing started, relative to the side or corner that was grabbed.
		Vec2 grabOffset;
	};

	struct ScrollDragAction : public BaseAction
	{
		// Position that was clicked when scrolling started, relative to position of the bar.
		int grabOffset;
	};

	struct ScrollButtonAction : public BaseAction
	{
		// Fractional scroll delta that accumulates based on delta startTime.
		double accumulatedDelta;
	};

	int x = 0;
	int y = 0;
	int width = 100;
	int height = 100;

	Size2 minSize;
	Size2 prefSize;
	Vec2 pinnedPos;

	std::string title;

	int margin = 2;
	int scrollOffsetX = 0;
	int scrollOffsetY = 0;
	
	unique_ptr<BaseAction> currentAction;

	uint isPinnable : 1 = 1;
	uint isCloseable : 1 = 1;
	uint isMinimizable : 1 = 1;
	uint isAutoSizingX : 1 = 1;
	uint isAutoSizingY : 1 = 1;
	uint resizeBehaviourX : 2 = 0;
	uint resizeBehaviourY : 2 = 0;
	uint showScrollbarX : 1 = 0;
	uint showScrollbarY : 1 = 0;

	uint requestMoveToTop : 1 = 0;
	uint requestClose : 1 = 0;
	uint requestPin : 1 = 0;
	uint requestMinimize : 1 = 0;

	uint isPinned : 1 = 0;
	uint isExpanded : 1 = 1;
};

} // namespace AV
