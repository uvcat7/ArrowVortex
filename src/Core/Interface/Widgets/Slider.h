#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WSlider : public Widget
{
public:
	~WSlider();
	WSlider();

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	void tick(bool enabled) override;
	void draw(bool enabled) override;

	void setRange(double begin, double end);

	void setValue(double value);
	double value() const;

	std::function<void()> onChange;

private:
	void myUpdateValue(double v);
	void myDrag(Vec2 pos);

	double myBegin;
	double myEnd;
	double myValue;
};

} // namespace AV
