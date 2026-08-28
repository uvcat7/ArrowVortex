cmake_minimum_required(VERSION 3.28)

project(ArrowVortex VERSION 1.1.0 LANGUAGES C CXX)

include(CTest)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()

option(ARROWVORTEX_ENABLE_TIDY "Run clang-tidy while compiling" OFF)
if(ARROWVORTEX_ENABLE_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE clang-tidy REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY
        "${CLANG_TIDY_EXECUTABLE};--warnings-as-errors=*;--exclude-header-filter=SDL3*"
    )
endif()

include_directories("${PROJECT_SOURCE_DIR}/src")

find_package(PkgConfig QUIET)

find_package(Aubio CONFIG QUIET NAMES aubio Aubio)
if(NOT TARGET Aubio::aubio)
    if(NOT PkgConfig_FOUND)
        message(FATAL_ERROR "aubio was not found and pkg-config is unavailable")
    endif()
    pkg_check_modules(AUBIO REQUIRED IMPORTED_TARGET aubio)
    add_library(Aubio::aubio ALIAS PkgConfig::AUBIO)
endif()

find_package(FFMPEG QUIET)
if(NOT FFMPEG_FOUND)
    if(NOT PkgConfig_FOUND)
        message(FATAL_ERROR "FFmpeg was not found and pkg-config is unavailable")
    endif()
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec libavformat libavutil libswresample libswscale)
    set(FFMPEG_LIBRARIES PkgConfig::FFMPEG)
endif()

find_package(Freetype REQUIRED)
find_package(iir CONFIG REQUIRED)
find_package(MAD REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

# System.h exposes SDL types and is included throughout the internal libraries.
# Propagate SDL's include path and link requirement consistently to every target.
link_libraries(SDL3::SDL3)

find_package(Stb CONFIG QUIET)
if(NOT Stb_FOUND AND NOT Stb_INCLUDE_DIR)
    find_path(Stb_INCLUDE_DIR NAMES stb_image.h REQUIRED)
endif()

find_package(Vorbis REQUIRED COMPONENTS vorbis vorbisfile)

add_subdirectory(src/Core)
add_subdirectory(src/Dialogs)
add_subdirectory(src/Editor)
add_subdirectory(src/Managers)
add_subdirectory(src/Simfile)
add_subdirectory(src/System)

if(BUILD_TESTING)
    add_executable(ArrowVortexSimfileRoundTrip tests/SimfileRoundTrip.cpp)
    if(UNIX AND NOT APPLE)
        # The application libraries intentionally retain upstream's circular
        # references. GNU ld needs an explicit rescan group when the headless
        # test executable links those static archives directly.
        target_link_libraries(ArrowVortexSimfileRoundTrip PRIVATE
            "$<LINK_GROUP:RESCAN,Simfile,System,Core,Editor,Managers,Dialogs>"
        )
    else()
        target_link_libraries(ArrowVortexSimfileRoundTrip PRIVATE Simfile System)
    endif()
    if(APPLE)
        target_link_libraries(ArrowVortexSimfileRoundTrip PRIVATE
                              "-framework Cocoa")
    endif()

    foreach(format sm ssc osu dwi)
        add_test(
            NAME simfile-roundtrip-${format}
            COMMAND $<TARGET_FILE:ArrowVortexSimfileRoundTrip>
                    "${PROJECT_SOURCE_DIR}/tests/fixtures/roundtrip.${format}"
        )
        set_tests_properties(simfile-roundtrip-${format} PROPERTIES
            TIMEOUT 30
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
        )
    endforeach()
endif()

if(APPLE)
    install(TARGETS ArrowVortex BUNDLE DESTINATION .)
else()
    install(TARGETS ArrowVortex RUNTIME DESTINATION bin)
    install(DIRECTORY bin/assets DESTINATION bin)
    install(DIRECTORY bin/noteskins DESTINATION bin)
    install(DIRECTORY bin/settings DESTINATION bin)
    install(FILES LICENSE CREDITS DESTINATION .)
endif()

file(GLOB_RECURSE path_sources CONFIGURE_DEPENDS
    "src/*.cpp" "src/*.mm" "src/*.hpp" "src/*.c" "src/*.h")
if(WIN32)
    add_custom_target(format ALL)
    add_custom_command(
        TARGET format POST_BUILD
        COMMAND clang-format --dry-run -Werror -style=file ${path_sources}
        USES_TERMINAL
    )
    add_dependencies(format ArrowVortex)
endif()
