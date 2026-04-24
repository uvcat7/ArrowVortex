list(APPEND INC
	"Chart.h"
	"Common.h"
	"NoteList.h"
	"Notes.h"
	"Parsing.h"
	"SegmentGroup.h"
	"SegmentList.h"
	"Segments.h"
	"Simfile.h"
	"Tempo.h"
	"TimingData.h"
)

list(APPEND SRC
	"Chart.cpp"
	"LoadDwi.cpp"
	"LoadOsu.cpp"
	"LoadSm.cpp"
	"NoteList.cpp"
	"Notes.cpp"
	"Parsing.cpp"
	"SaveOsu.cpp"
	"SaveSm.cpp"
	"SegmentGroup.cpp"
	"SegmentList.cpp"
	"Segments.cpp"
	"Simfile.cpp"
	"Tempo.cpp"
	"Testing.cpp"
	"TimingData.cpp"
)


add_library(Simfile STATIC ${SRC} ${INC})
target_link_libraries(Simfile PRIVATE Core Editor Managers System)
