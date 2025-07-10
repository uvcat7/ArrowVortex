file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB INC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(System STATIC ${SRC} ${INC}) 
target_link_libraries(System PRIVATE Core Editor OpenGL::GL winmm)

add_executable(ArrowVortex WIN32 System.cpp "${CMAKE_CURRENT_SOURCE_DIR}/ArrowVortex.rc")
add_custom_command(
	TARGET ArrowVortex POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/bin
		$<TARGET_FILE_DIR:ArrowVortex>)
target_link_libraries(ArrowVortex PRIVATE System ${CMAKE_FIND_ROOT_PATH}/lib/libgcc_s.a ${CMAKE_FIND_ROOT_PATH}/lib/libwinpthread.a ${CMAKE_FIND_ROOT_PATH}/lib/libstdc++.a)
file(COPY ${CMAKE_FIND_ROOT_PATH}/bin/libgcc_s_seh-1.dll ${CMAKE_FIND_ROOT_PATH}/bin/libwinpthread-1.dll DESTINATION .)
