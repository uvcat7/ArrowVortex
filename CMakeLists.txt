cmake_minimum_required(VERSION 3.30)

# Find VCPKG_ROOT from environment variables.
if (DEFINED ENV{VCPKG_ROOT})
	set(VCPKG_ROOT ENV{VCPKG_ROOT})
elseif(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
	set(VCPKG_ROOT ENV{VCPKG_INSTALLATION_ROOT})
endif()

if (LINUX)
	if (NOT DEFINED VCPKG_ROOT)
		message(FATAL_ERROR "VCPKG_ROOT must be set to build on Linux, CMake will exit.")
	endif()

	set(CMAKE_SYSTEM_NAME Windows)

	set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
	set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
	set(VCPKG_TARGET_TRIPLET x64-mingw-static)

	if (NOT DEFINED CMAKE_FIND_ROOT_PATH)
		set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
	endif()

	set(MINGW_BASED true)
endif()

if(DEFINED VCPKG_ROOT)
	set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
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
install(DIRECTORY bin/assets DESTINATION bin)
install(DIRECTORY bin/noteskins DESTINATION bin)
install(DIRECTORY bin/settings DESTINATION bin)
