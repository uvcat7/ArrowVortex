cmake_minimum_required(VERSION 3.30)

if(WIN32)
	option(VCPKG "Use vcpkg for dependencies" ON)
else()
	option(VCPKG "Use vcpkg for dependencies" OFF)
endif()

if(VCPKG)
	set(VCPKG_LIBRARY_LINKAGE "static")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(ArrowVortex)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

include_directories("${PROJECT_SOURCE_DIR}/src")

find_package(Freetype REQUIRED)
find_package(MAD REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(src/Core)
add_subdirectory(src/Dialogs)
add_subdirectory(src/Editor)
add_subdirectory(src/Managers)
add_subdirectory(src/Simfile)
add_subdirectory(src/System)

install(TARGETS ArrowVortex RUNTIME)
install(DIRECTORY bin/assets DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY bin/noteskins DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY bin/settings DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
