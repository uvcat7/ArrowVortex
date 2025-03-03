#pragma once

#include <Core/Interface/Widgets/LineEdit.h>

namespace AV {

class WSpinner : public Widget
{
public:
	~WSpinner();
	WSpinner();

	void onMousePress(MousePress& input) override;
	void onMouseRelease(MouseRelease& input) override;

	UiElement* findMouseOver(int x, int y) override;
	void onArrange(Rect r) override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;

	void setRange(double min, double max);
	void setStep(double step);
	void setPrecision(int minDecimalPlaces, int maxDecimalPlaces);
	
	void setValue(double value);
	double value() const;
	int ivalue() const;

	std::function<void()> onChange;

private:
	void myUpdateText();
	Rect myButtonRect();
	void myOnTextChanged();

	shared_ptr<WLineEdit> myEdit;
	bool myIsUpActive;
	float myRepeatTimer;
	double myMin, myMax, myStep;
	double myValue;
	int myMinDecimalPlaces;
	int myMaxDecimalPlaces;
};

} // namespace AV
