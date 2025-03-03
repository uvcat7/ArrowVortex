#pragma once

#include <Core/Common/Event.h>

#include <Core/Graphics/Draw.h>
#include <Core/Interface/Widgets/VerticalTabs.h>

#include <Vortex/View/EditorDialog.h>

namespace AV {

class DialogDancingBot : public EditorDialog
{
public:
	enum class Foot { Left = 0, Right = 1, Both = 2};

	~DialogDancingBot();
	DialogDancingBot();
	
	void draw(bool enabled) override;

	void onDoFootswitchesChanged();
	void onDoCrossoversChanged();

	void drawPad(Rect rect);

private:
	struct FootCoord { float x, y, zoom; };

	void myHandleChartChanged();
	void myInitWidgets();
	void myAssignFeetToNotes();
	void myGetFeetPositions(Rect rect, FootCoord* out, int player);

	struct PadWidget;
	struct FormOptions;

	FormOptions* myFormOptions = nullptr;
	shared_ptr<PadWidget> myPadWidget;
	shared_ptr<WVerticalTabs> myTabs;

	vector<int> myPadLayout;
	vector<uint> myFeetBits;
	SpriteEx myPadSpr[6];
	SpriteEx myFeetSpr[2];
	Texture myPadTex;
	Texture myFeetTex;

	EventSubscriptions mySubscriptions;
};

} // namespace AV
