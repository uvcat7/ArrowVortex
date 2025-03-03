#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

struct DialogFrame;

// Base class for dialog objects.
class Dialog : public UiElement
{
public:
	enum class ResizeBehavior
	{
		// Resizing is disabled, the size is kept equal to the preferred size.
		Fixed,

		// Resizing is enabled, the maximum size is restricted to the preferred size.
		Bound,

		// Resizing is enabled, the maximum size is not restricted.
		Unbound,
	};

	virtual ~Dialog();

	Dialog();

	void setPosition(int x, int y);
	void setTitle(stringref title);
	void setMargin(int margin);

	void setResizeBehaviorH(ResizeBehavior behaviour);
	void setResizeBehaviorV(ResizeBehavior behaviour);

	void setWidth(int w);
	void setHeight(int h);
	void setMinimumSize(int w, int h);
	void setPreferredSize(int w, int h);

	void requestClose();
	void requestPin();

	void setCloseable(bool enable);
	void setMinimizable(bool enable);

	Size2 getMinimumSize() const;
	Size2 getPreferredSize() const;

	Rect getContentRect() const;

	bool isPinned() const;

	void resetCloseRequest() const;
	bool hasCloseRequest() const;

	void onMousePress(MousePress& press) override;
	void onMouseRelease(MouseRelease& release) override;

	void arrange();

	UiElement* findMouseOver(int x, int y) override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;

	DialogFrame* frame() { return myFrame; }

protected:
	virtual void onUpdateSize();

	virtual void tickContent(bool enabled) = 0;
	virtual void drawContent(bool enabled) = 0;
	virtual UiElement* findContentMouseOver(int x, int y) = 0;

private:
	DialogFrame* myFrame;
};

} // namespace AV
