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

if (MINGW_BASED)
	# This imports important static libraries needed by MinGW builders.
	list(
		APPEND MINGW_ESSENTIAL_LIBS
		${CMAKE_FIND_ROOT_PATH}/lib/libgcc_s.a
		${CMAKE_FIND_ROOT_PATH}/lib/libwinpthread.a
		${CMAKE_FIND_ROOT_PATH}/lib/libstdc++.a
	)

	# This copies the two most important DLLs ever to run ArrowVortex.
	# If you do not copy/have these, ArrowVortex will not run (on wine).
	file(
		COPY
		${CMAKE_FIND_ROOT_PATH}/bin/libgcc_s_seh-1.dll
		${CMAKE_FIND_ROOT_PATH}/bin/libwinpthread-1.dll
		DESTINATION .
	)
endif()

target_link_libraries(ArrowVortex PRIVATE System ${MINGW_ESSENTIAL_LIBS})
