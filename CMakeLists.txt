cmake_minimum_required(VERSION 3.28)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(ArrowVortex)

set(CMAKE_CONFIGURATION_TYPES Debug;Release;MinSizeRel)
if("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    set(tidy_flags "--fix-errors;--fix-notes")
    set(format_flags "-i")
else()
    set(tidy_flags "--warnings-as-errors=*")
    set(format_flags "--dry-run")
endif()
if(WIN32)
    set(tidy_extra "-extra-arg=/EHsc;--exclude-header-filter=SDL3*;")
endif()
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;${tidy_flags};${tidy_extra}-p=build")

# Sometimes caching will cause CMAKE_TOOLCHAIN_FILE, build presets, and includes to not update/work, so include by hand.
include_directories("${PROJECT_SOURCE_DIR}/src")

if(LINUX AND "${PRESET_NAME}" STREQUAL "")
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(PRESET_NAME "linux-debug")
    else()
        set(PRESET_NAME "linux-release")
    endif()
endif()

message(STATUS "Including toolchain includes: ${PROJECT_SOURCE_DIR}/out/build/${PRESET_NAME}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include") 
include_directories("${PROJECT_SOURCE_DIR}/out/build/${PRESET_NAME}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
include_directories("${PROJECT_SOURCE_DIR}/build/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")

if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    if("$ENV{VCPKG_ROOT}" STREQUAL "")
        message(WARNING "%VCPKG_ROOT% is not defined, attempting fallback generation.") 
        if (LINUX)
            include("/opt/vcpkg/scripts/buildsystems/vcpkg.cmake")
        elseif(WIN32)
            include("C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\scripts\buildsystems\vcpkg.cmake")
        endif()
    else()
        message(WARNING "CMAKE_TOOLCHAIN_FILE is not defined, manually including.") 
        include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    endif()
endif()

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
add_custom_target(format ALL)
add_custom_command(
    TARGET format
    POST_BUILD
    COMMAND clang-format ${format_flags} -Werror -style=file ${path_sources}
    USES_TERMINAL
)
add_dependencies(format ArrowVortex)
