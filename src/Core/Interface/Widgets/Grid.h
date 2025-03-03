#pragma once

#include <Core/Interface/Widget.h>

namespace AV {

class WGrid : public Widget
{
public:
	WGrid();
	~WGrid();

	void addRow(int height, int bottomMargin, int stretch = 0);
	void addRow(shared_ptr<Widget> autosize, int bottomMargin, int stretch = 0);

	void addColumn(int width, int rightMargin, int stretch = 0);
	void addColumn(shared_ptr<Widget> autosize, int rightMargin, int stretch = 0);

	void addWidget(shared_ptr<Widget> widget, Row row, int column, Row rowSpan = 1, int columnSpan = 1);

	void setRowMinSize(Row row, int minSize);
	void setColMinSize(int col, int minSize);
  
	void onMeasure() override;
	void onArrange(Rect r) override;

	UiElement* findMouseOver(int x, int y) override;
	void handleEvent(UiEvent& uiEvent) override;
	void tick(bool enabled) override;
	void draw(bool enabled) override;

private:
	struct GridData;
	GridData* myGrid;
};

} // namespace AV
