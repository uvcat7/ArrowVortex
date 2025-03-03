#pragma once

#include <Core/Common/Input.h>

#include <Core/Types/Rect.h>

#include <Core/Interface/UiElement.h>

namespace AV {

// Base class for widget objects.
class Widget : public UiElement
{
public:
	typedef void (*WidgetAction)(Widget& w);

	virtual ~Widget();

	Widget();
	Widget(Widget* parent);

	// Update functions.
	void measure();
	void arrange(Rect r);

	// Set functions.
	void setEnabled(bool enabled);

	// Get functions.
	bool isEnabled() const;
	int getWidth() const;
	int getHeight() const;
	int getMinWidth() const;
	int getMinHeight() const;
	Rect getRect() const;

	UiElement* findMouseOver(int x, int y) override;

protected:
	virtual void onMeasure();
	virtual void onArrange(Rect r);

protected:
	int myWidth;
	int myHeight;
	int myMinWidth;
	int myMinHeight;
	bool myIsEnabled;
	Rect myRect;
};

} // namespace AV