cmake_minimum_required(VERSION 3.28)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(ArrowVortex)

set(CMAKE_CONFIGURATION_TYPES Debug;Release;MinSizeRel)
if("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    set(tidy_flags "--fix-errors;--fix-notes;")
    set(format_flags "-i")
else()
    set(tidy_flags "--warnings-as-errors=*;")
    set(format_flags "--dry-run")
endif()
if(WIN32)
    set(tidy_extra "-extra-arg=/EHsc;--exclude-header-filter=SDL3*;")
endif()

if(NOT APPLE)
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;${tidy_flags}${tidy_extra}-p=build")
endif()

# Sometimes caching will cause CMAKE_TOOLCHAIN_FILE, build presets, and includes to not update/work, so include fallbacks by hand.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR "${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
        message(WARNING "CMAKE_TOOLCHAIN_FILE is not defined, manually including.")
        include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    elseif(NOT "${VCPKG_ROOT}" STREQUAL "")
        message(WARNING "CMAKE_TOOLCHAIN_FILE is not defined, manually including.")
        include("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    elseif(UNIX AND NOT APPLE)
        message(WARNING "{VCPKG_ROOT} is not defined, attempting fallback generation.")
        include("/opt/vcpkg/scripts/buildsystems/vcpkg.cmake")
    elseif(WIN32)
        message(WARNING "%VCPKG_ROOT% is not defined, attempting fallback generation.")
        include("C:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
    endif()
endif()

if("${toolchain_include}" STREQUAL "")
    if("${YML}" STREQUAL "")
        set(toolchain_include "${PROJECT_SOURCE_DIR}/out/build/${PRESET_NAME}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
    else()
        set(toolchain_include "${PROJECT_SOURCE_DIR}/build/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
    endif()
endif()

if(APPLE)
    set(_homebrew_prefix "/opt/homebrew")
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(_homebrew_prefix "/usr/local")
    endif()
    set(_homebrew_sdl3 "${_homebrew_prefix}/opt/sdl3")
    if(EXISTS "${_homebrew_sdl3}/include")
        list(APPEND CMAKE_PREFIX_PATH "${_homebrew_sdl3}")
        list(APPEND toolchain_include "${_homebrew_sdl3}/include")
        message(STATUS "Detected Homebrew SDL3 at: ${_homebrew_sdl3}")
    endif()
endif()

message(STATUS "Including toolchain folder: ${toolchain_include}") 
include_directories("${toolchain_include}")
include_directories("${PROJECT_SOURCE_DIR}/src")

find_package(Aubio CONFIG NAMES aubio Aubio)
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
# Only prevent format/building on Windows, as Linux's clang-format is slightly different
if(WIN32)
    add_custom_target(format ALL)
    add_custom_command(
        TARGET format
        POST_BUILD
        COMMAND clang-format ${format_flags} -Werror -style=file ${path_sources}
        USES_TERMINAL
    )
    add_dependencies(format ArrowVortex)
endif()
