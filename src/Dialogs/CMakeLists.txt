list(APPEND INC
	"AdjustSync.h"
	"AdjustTempo.h"
	"AdjustTempoSM5.h"
	"ChartList.h"
	"ChartProperties.h"
	"CustomSnap.h"
	"DancingBot.h"
	"Dialog.h"
	"GenerateNotes.h"
	"LabelBreakdown.h"
	"NewChart.h"
	"PreviewSettings.h"
	"SongProperties.h"
	"TempoBreakdown.h"
	"WaveformSettings.h"
	"Zoom.h"
)

list(APPEND SRC
	"AdjustSync.cpp"
	"AdjustTempo.cpp"
	"AdjustTempoSM5.cpp"
	"ChartList.cpp"
	"ChartProperties.cpp"
	"CustomSnap.cpp"
	"DancingBot.cpp"
	"Dialog.cpp"
	"GenerateNotes.cpp"
	"LabelBreakdown.cpp"
	"NewChart.cpp"
	"PreviewSettings.cpp"
	"SongProperties.cpp"
	"TempoBreakdown.cpp"
	"WaveformSettings.cpp"
	"Zoom.cpp"
)

add_library(Dialogs STATIC ${SRC} ${INC})
target_link_libraries(Dialogs PRIVATE Core Editor Managers Simfile System)
