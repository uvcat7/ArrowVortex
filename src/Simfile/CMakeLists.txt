file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB INC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(Simfile STATIC ${SRC} ${INC})
target_link_libraries(Simfile PRIVATE Core Editor Managers System)
