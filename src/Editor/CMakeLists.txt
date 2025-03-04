file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB INC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Editor STATIC ${SRC} ${INC})
target_link_libraries(Editor PRIVATE Core Dialogs mad::mad Managers Simfile System Vorbis::vorbisfile Vorbis::vorbis Ogg::ogg)
