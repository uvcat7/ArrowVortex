file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB INC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Managers STATIC ${SRC} ${INC})
target_link_libraries(Managers PRIVATE Core Editor Simfile System)
