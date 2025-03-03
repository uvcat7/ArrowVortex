#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WButton : public Widget
{
public:
	~WButton();
	WButton();
	WButton(const char* text);

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	void draw(bool enabled) override;

	std::function<void()> onPress;

protected:
	std::string myText;
};

} // namespace AV
