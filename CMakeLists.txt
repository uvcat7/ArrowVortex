cmake_minimum_required(VERSION 3.30)

# If you're on Linux, you're probably not going to be building
# with normal tools anytime soon. We'll just assume you have
# W64-MinGW32.
if (LINUX)
	set(CMAKE_SYSTEM_NAME Windows)

	set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
	set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

	if (NOT DEFINED CMAKE_FIND_ROOT_PATH)
		set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
	endif()

	set(MINGW_BASED true)

	option(VCPKG "Use vcpkg for dependencies" ON)
endif()

if(WIN32)
	option(VCPKG "Use vcpkg for dependencies" ON)
else()
	option(VCPKG "Use vcpkg for dependencies" OFF)
endif()

if(VCPKG)
	set(VCPKG_LIBRARY_LINKAGE "static")
	if(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	elseif(DEFINED ENV{VCPKG_ROOT})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	endif()
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
install(DIRECTORY bin/assets DESTINATION bin)
install(DIRECTORY bin/noteskins DESTINATION bin)
install(DIRECTORY bin/settings DESTINATION bin)
