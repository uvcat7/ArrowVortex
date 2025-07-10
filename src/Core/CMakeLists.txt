file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB INC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Core STATIC ${SRC} ${INC})
target_link_libraries(Core PRIVATE Freetype::Freetype System OpenGL::GL)
