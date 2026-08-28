set(INC
    Debug.h
    File.h
    Mixer.h
    Menu.h
    OpenGL.h
    Resources.h
    System.h
    Thread.h
)

set(SRC
    Debug.cpp
    File.cpp
    Mixer.cpp
    System.cpp
    Thread.cpp
)

if(APPLE)
    list(APPEND SRC MenuMac.mm)
    set_source_files_properties(MenuMac.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
else()
    list(APPEND SRC Menu.cpp)
endif()

add_library(System STATIC ${SRC} ${INC})
target_compile_definitions(System PRIVATE ARROWVORTEX_NO_APP_ENTRY=1)
target_link_libraries(System PRIVATE Core Editor OpenGL::GL SDL3::SDL3)

set(APP_SOURCES System.cpp)
if(WIN32)
    list(APPEND APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ArrowVortex.rc")
    add_executable(ArrowVortex WIN32 ${APP_SOURCES})
elseif(APPLE)
    add_executable(ArrowVortex MACOSX_BUNDLE ${APP_SOURCES})
    set_target_properties(ArrowVortex PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "ArrowVortex"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.arrowvortex.ArrowVortex"
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/cmake/Info.plist.in"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.arrowvortex.ArrowVortex"
    )

    set(MACOS_ICON "${CMAKE_CURRENT_BINARY_DIR}/ArrowVortex.icns")
    add_custom_command(
        OUTPUT "${MACOS_ICON}"
        COMMAND "${PROJECT_SOURCE_DIR}/tools/make_macos_icon.sh"
                "${PROJECT_SOURCE_DIR}/bin/assets/arrow vortex icon.png"
                "${MACOS_ICON}"
        DEPENDS "${PROJECT_SOURCE_DIR}/bin/assets/arrow vortex icon.png"
                "${PROJECT_SOURCE_DIR}/tools/make_macos_icon.sh"
        VERBATIM
    )
    add_custom_target(ArrowVortexIcon DEPENDS "${MACOS_ICON}")
    add_dependencies(ArrowVortex ArrowVortexIcon)

    add_custom_command(
        TARGET ArrowVortex POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/bin/assets"
                "$<TARGET_BUNDLE_CONTENT_DIR:ArrowVortex>/Resources/assets"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/bin/noteskins"
                "$<TARGET_BUNDLE_CONTENT_DIR:ArrowVortex>/Resources/noteskins"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/bin/settings"
                "$<TARGET_BUNDLE_CONTENT_DIR:ArrowVortex>/Resources/settings"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${MACOS_ICON}"
                "$<TARGET_BUNDLE_CONTENT_DIR:ArrowVortex>/Resources/ArrowVortex.icns"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${PROJECT_SOURCE_DIR}/LICENSE"
                "$<TARGET_BUNDLE_CONTENT_DIR:ArrowVortex>/Resources/LICENSE"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${PROJECT_SOURCE_DIR}/CREDITS"
                "$<TARGET_BUNDLE_CONTENT_DIR:ArrowVortex>/Resources/CREDITS"
        VERBATIM
    )
else()
    add_executable(ArrowVortex ${APP_SOURCES})
endif()

if(NOT APPLE)
    add_custom_command(
        TARGET ArrowVortex POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/bin"
                "$<TARGET_FILE_DIR:ArrowVortex>"
    )
endif()

target_link_libraries(ArrowVortex PRIVATE System SDL3::SDL3)
if(APPLE)
    target_link_libraries(ArrowVortex PRIVATE "-framework Cocoa")
endif()
