list(APPEND INC
	"Debug.h"
	"File.h"
	"Mixer.h"
	"OpenGL.h"
	"Resources.h"
	"System.h"
	"Thread.h"
)

list(APPEND SRC
	"Debug.cpp"
	"File.cpp"
	"Mixer.cpp"
	"System.cpp"
	"Thread.cpp"
)

add_library(System STATIC ${SRC} ${INC}) 
target_link_libraries(System PRIVATE Core Editor OpenGL::GL winmm SDL3::SDL3)

add_executable(ArrowVortex WIN32 System.cpp "${CMAKE_CURRENT_SOURCE_DIR}/ArrowVortex.rc")
add_custom_command(
	TARGET ArrowVortex POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/bin
		$<TARGET_FILE_DIR:ArrowVortex>)
target_link_libraries(ArrowVortex PRIVATE System SDL3::SDL3)
