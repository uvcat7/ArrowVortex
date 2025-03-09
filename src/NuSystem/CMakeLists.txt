file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB INC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(NuSystem STATIC ${SRC} ${INC})
target_link_libraries(NuSystem PRIVATE SDL3::SDL3)

add_executable(ArrowVortex System.cpp "${CMAKE_CURRENT_SOURCE_DIR}/ArrowVortex.rc")
add_custom_command(
	TARGET ArrowVortex POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/bin
		$<TARGET_FILE_DIR:ArrowVortex>)
target_link_libraries(ArrowVortex PRIVATE NuSystem)
