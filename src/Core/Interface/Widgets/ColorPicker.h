#pragma once

#include <Core/Graphics/Colorf.h>
#include <Core/Interface/Widget.h>

namespace AV {

class WColorPicker : public Widget
{
public:
	~WColorPicker();
	WColorPicker();

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	void setValue(Colorf value);
	Colorf value() const;

	std::function<void()> onChange;

protected:
	UiElement* findMouseOver(int x, int y) override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;

	Colorf myValue;

private:
	struct Expanded;
	Expanded* myExpanded;
};

} // namespace AV
