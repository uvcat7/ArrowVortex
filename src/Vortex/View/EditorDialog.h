#pragma once

#include <Core/Interface/Widgets/Grid.h>
#include <Core/Interface/Dialog.h>

namespace AV {

enum class DialogId
{
	AdjustSync,
	AdjustTempo,
	ChartList,
	ChartProperties,
	DancingBot,
	GenerateNotes,
	NewChart,
	SongProperties,
	TempoBreakdown,
	WaveformSettings,
	Keysounds,

	Count
};

class EditorDialog : public Dialog
{
public:
	~EditorDialog();
	EditorDialog();

	void onUpdateSize() override;

	void handleEvent(UiEvent& uiEvent) override;

	static const char* getName(DialogId id);

protected:
	void tickContent(bool enabled) override;
	void drawContent(bool enabled) override;
	UiElement* findContentMouseOver(int x, int y) override;

	shared_ptr<Widget> myContent;
};

} // namespace AV
