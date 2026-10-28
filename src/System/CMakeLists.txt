list(APPEND INC
	"Debug.h"
	"File.h"
	"Mixer.h"
	"Menu.h"
	"OpenGL.h"
	"Resources.h"
	"System.h"
	"Thread.h"
)

list(APPEND SRC
	"Debug.cpp"
	"File.cpp"
	"Mixer.cpp"
    "Menu.cpp"
	"System.cpp"
	"Thread.cpp"
)

add_library(System STATIC ${SRC} ${INC}) 
target_link_libraries(System PRIVATE Core Editor OpenGL::GL SDL3::SDL3)

if(APPLE)
	set(ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ArrowVortex.icns")
	set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	add_executable(ArrowVortex MACOSX_BUNDLE System.cpp ${ICON_FILE})
	string(TIMESTAMP BUILD_DATE "%Y.%m.%d")
	set_target_properties(ArrowVortex PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
		MACOSX_BUNDLE_BUNDLE_NAME "ArrowVortex"
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.uvcat7.ArrowVortex"
		MACOSX_BUNDLE_ICON_FILE "ArrowVortex.icns"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION}"
		MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}.${BUILD_DATE}")
	# SDL_GetBasePath() points at Contents/Resources inside a bundle.
	set(RESOURCE_DIR "$<TARGET_BUNDLE_CONTENT_DIR:ArrowVortex>/Resources")
else()
	add_executable(ArrowVortex WIN32 System.cpp "${CMAKE_CURRENT_SOURCE_DIR}/ArrowVortex.rc")
	set(RESOURCE_DIR "$<TARGET_FILE_DIR:ArrowVortex>")
endif()
add_custom_command(
	TARGET ArrowVortex POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/bin
		${RESOURCE_DIR})
target_link_libraries(ArrowVortex PRIVATE System SDL3::SDL3)
