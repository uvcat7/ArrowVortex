list(APPEND INC
	"ChartMan.h"
	"MetadataMan.h"
	"NoteMan.h"
	"NoteskinMan.h"
	"SimfileMan.h"
	"StyleMan.h"
	"TempoMan.h"
)

list(APPEND SRC
	"ChartMan.cpp"
	"MetadataMan.cpp"
	"NoteMan.cpp"
	"NoteskinMan.cpp"
	"SimfileMan.cpp"
	"StyleMan.cpp"
	"TempoMan.cpp"
)


add_library(Managers STATIC ${SRC} ${INC})
target_link_libraries(Managers PRIVATE Core Editor Simfile System)
