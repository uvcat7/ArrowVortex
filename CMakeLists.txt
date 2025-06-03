cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(ArrowVortex)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

include_directories("${PROJECT_SOURCE_DIR}/src")

# removing configuration from MAD, Ogg, and Vorbis, because it does something funny and idk.

find_package(Freetype REQUIRED)
find_package(MAD REQUIRED)
find_package(Ogg REQUIRED)
find_package(Vorbis REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(src/Core)
add_subdirectory(src/Dialogs)
add_subdirectory(src/Editor)
add_subdirectory(src/Managers)
add_subdirectory(src/Simfile)
add_subdirectory(src/System)

# NuSystem - 2024, 2025.
#  "the job wasn't done"

# install(TARGETS ArrowVortex RUNTIME)
install(DIRECTORY bin/assets DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY bin/noteskins DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY bin/settings DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
