cmake_minimum_required(VERSION 3.30)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

project(ArrowVortex)

set(CMAKE_CONFIGURATION_TYPES Debug;Release;MinSizeRel)
if("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    set(tidy_flags "--fix-errors;--fix-notes")
    set(format_flags "-i")
else()
    set(tidy_flags "--warnings-as-errors=*")
    set(format_flags "--dry-run")
endif()
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;${tidy_flags};-extra-arg=/EHsc;-p=build")

include_directories("${PROJECT_SOURCE_DIR}/src")

find_package(aubio CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(Freetype REQUIRED)
find_package(iir CONFIG REQUIRED)
find_package(MAD REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Vorbis REQUIRED)

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

file(GLOB_RECURSE path_sources "src/*.cpp" "src/*.hpp" "src/*.c" "src/*.h")
add_custom_target(format ALL)
add_custom_command(
    TARGET format
    POST_BUILD
    COMMAND clang-format ${format_flags} -Werror -style=file ${path_sources}
    USES_TERMINAL
)
add_dependencies(format ArrowVortex)
