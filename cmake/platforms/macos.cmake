# macOS specific settings

if(NOT APPLE)
    return()
endif()

option(BUILD_MACOS_APP "Deploy as a macOS .app" ON)

enable_language(OBJC)

list(APPEND SYSTEM_PLATFORM_SOURCES ${SOURCE_DIR}/sys/sys_osx.m)

list(APPEND COMMON_LIBRARIES "-framework Cocoa")
list(APPEND CLIENT_LIBRARIES "-framework IOKit")
list(APPEND RENDERER_LIBRARIES "-framework OpenGL")

set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
set(CMAKE_OSX_ARCHITECTURES arm64;x86_64)

if(BUILD_MACOS_APP)
    set(CLIENT_EXECUTABLE_OPTIONS MACOSX_BUNDLE)
    list(APPEND POST_CONFIGURE_FUNCTIONS finish_macos_app)
endif()

function(finish_macos_app)
    get_filename_component(MACOS_ICON_FILE ${MACOS_ICON_PATH} NAME)

    set(MACOS_APP_BUNDLE_NAME ${CLIENT_NAME})
    set(MACOS_APP_EXECUTABLE_NAME ${CLIENT_BINARY})
    set(MACOS_APP_GUI_IDENTIFIER ${MACOS_BUNDLE_ID})
    set(MACOS_APP_ICON_FILE ${MACOS_ICON_FILE})
    set(MACOS_APP_SHORT_VERSION_STRING ${PRODUCT_VERSION})
    set(MACOS_APP_BUNDLE_VERSION ${PRODUCT_VERSION})
    set(MACOS_APP_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
    set(MACOS_APP_COPYRIGHT ${COPYRIGHT})

    if(PROTOCOL_HANDLER_SCHEME)
        set(MACOS_APP_PLIST_URL_TYPES
        "<key>CFBundleURLTypes</key>
        <array>
            <dict>
                <key>CFBundleURLName</key>
                <string>${MACOS_APP_BUNDLE_NAME}</string>
                <key>CFBundleURLSchemes</key>
                <array>
                    <string>${PROTOCOL_HANDLER_SCHEME}</string>
                </array>
            </dict>
        </array>")
    else()
        set(MACOS_APP_PLIST_URL_TYPES "")
    endif()

    configure_file(${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
        ${CMAKE_BINARY_DIR}/Info.plist @ONLY)

    set_target_properties(${CLIENT_BINARY} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/Info.plist)

    set(RESOURCES_DIR $<TARGET_FILE_DIR:${CLIENT_BINARY}>/../Resources)
    add_custom_command(TARGET ${CLIENT_BINARY} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCES_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${MACOS_ICON_PATH} ${RESOURCES_DIR})

    if(USE_RENDERER_DLOPEN)
        set(MACOS_APP_BINARY_DIR ${CLIENT_BINARY}.app/Contents/MacOS)

        if(BUILD_RENDERER_GL1)
            set_output_dirs(${RENDERER_GL1_BINARY} SUBDIRECTORY ${MACOS_APP_BINARY_DIR})
            add_dependencies(${CLIENT_BINARY} ${RENDERER_GL1_BINARY})
        endif()

        if(BUILD_RENDERER_GL2)
            set_output_dirs(${RENDERER_GL2_BINARY} SUBDIRECTORY ${MACOS_APP_BINARY_DIR})
            add_dependencies(${CLIENT_BINARY} ${RENDERER_GL2_BINARY})
        endif()
    endif()
endfunction()

if(NOT "$ENV{APPLE_CERTIFICATE_ID}" STREQUAL "")
    list(APPEND POST_CONFIGURE_FUNCTIONS codesign)

    function(codesign)
        set(DEV_ID "Developer ID Application")

        get_directory_property(INSTALL_TARGETS DIRECTORY
            ${CMAKE_SOURCE_DIR} BUILDSYSTEM_TARGETS)

        # Code sign everything that will be installed
        foreach(TARGET IN LISTS INSTALL_TARGETS)
            get_target_property(DESTINATION ${TARGET} INSTALL_DESTINATION)
            if(NOT DESTINATION)
                continue()
            endif()

            add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND codesign --force --deep --options runtime
                    --sign "$ENV{APPLE_CERTIFICATE_ID}"
                    "$<TARGET_FILE:${TARGET}>"
                COMMENT "Code Signing for macOS: $<TARGET_FILE_BASE_NAME:${TARGET}>")
        endforeach()
    endfunction()
endif()

set(CPACK_GENERATOR "DragNDrop")

set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME} Installer")
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/misc/macos-dmg-background.png")
set(CPACK_DMG_SUBDIRECTORY "${CLIENT_NAME}")

configure_file(
  "${CMAKE_SOURCE_DIR}/misc/macos-dmg-setup.applescript.in"
  "${CMAKE_BINARY_DIR}/macos-dmg-setup.applescript"
  @ONLY
)

set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_BINARY_DIR}/macos-dmg-setup.applescript")
