file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB INC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Editor STATIC ${SRC} ${INC})
target_link_libraries(Editor PRIVATE Core Dialogs MAD::MAD Managers Simfile System Vorbis::vorbisfile Ogg::ogg)
