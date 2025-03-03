#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WLabel : public Widget
{
public:
	~WLabel();
	WLabel();
	WLabel(stringref text);
	WLabel(const char* text);

	void draw(bool enabled) override;

	void setText(const char* text);

protected:
	std::string myText;
};

} // namespace AV
