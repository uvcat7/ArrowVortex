file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB INC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Dialogs STATIC ${SRC} ${INC})
target_link_libraries(Dialogs PRIVATE Core Editor Managers Simfile System)
