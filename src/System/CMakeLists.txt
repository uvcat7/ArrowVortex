file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB INC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(System STATIC ${SRC} ${INC}) 
target_link_libraries(System PRIVATE Core Editor OpenGL::GL winmm)

add_executable(ArrowVortex WIN32 System.cpp "${CMAKE_CURRENT_SOURCE_DIR}/ArrowVortex.rc")
target_link_libraries(ArrowVortex PRIVATE System)
