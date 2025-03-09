cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# i can't use windows btw, i live with this.

set(VCPKG_TARGET_TRIPLET x64-linux)

project(ArrowVortex)

set(CMAKE_CONFIGURATION_TYPES Debug Release)

include_directories("${PROJECT_SOURCE_DIR}/src")

find_package(SDL3 CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(mad CONFIG REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# currently unable to build this stuff until i can successfully port.

# add_subdirectory(src/Core)
# add_subdirectory(src/Dialogs)
# add_subdirectory(src/Editor)
# add_subdirectory(src/Managers)
# add_subdirectory(src/Simfile)
# add_subdirectory(src/System)

add_subdirectory(src/NuSystem)

# install(TARGETS ArrowVortex RUNTIME)
install(DIRECTORY bin/assets DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY bin/noteskins DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY bin/settings DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
