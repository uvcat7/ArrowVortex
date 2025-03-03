#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WCheckbox : public Widget
{
public:
	~WCheckbox();
	WCheckbox();
	WCheckbox(const char* text);

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	void draw(bool enabled) override;

	void setValue(bool value);
	bool value() const;

	std::function<void()> onChange;

protected:
	Rect myBoxRect() const;

	std::string myText;
	bool myValue;
};

} // namespace AV
