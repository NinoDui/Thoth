# Target-2, ThothApp

set(APP_ENTRY_SRC main.cpp AppRunner.cpp AppRunner.h)
set(APP_UI_SRC
    ui/Q_AppMainWindow.cpp
    ui/Q_AppMainWindow.h
    ui/Q_SettingDialog.cpp
    ui/Q_SettingDialog.h
    ui/Q_ShadowingBar.cpp
    ui/Q_ShadowingBar.h
    ui/Q_PlaybackControlBar.cpp
    ui/Q_PlaybackControlBar.h
    ui/StyleLoader.h
)
set(APP_RESOURCES resources/resources.qrc)
add_executable(ThothApp ${APP_ENTRY_SRC} ${APP_UI_SRC} ${APP_RESOURCES})
thoth_set_warning(ThothApp)

target_link_libraries(ThothApp PRIVATE ThothCore Qt6::Widgets)

if(WIN32)
    # set_target_properties(ThothApp PROPERTIES WIN32_EXECUTABLE ON)
endif()

if(APPLE AND THOTH_MACOS_BUNDLE)
    set_target_properties(
        ThothApp
        PROPERTIES MACOSX_BUNDLE ON
                   OUTPUT_NAME "Thoth"
                   MACOSX_BUNDLE_BUNDLE_NAME "Thoth"
                   MACOSX_BUNDLE_GUI_IDENTIFIER "com.ninodui.thoth"
                   MACOSX_BUNDLE_BUNDLE_VERSION "${THOTH_VERSION}"
                   MACOSX_BUNDLE_SHORT_VERSION_STRING "${THOTH_VERSION}"
                   MACOSX_BUNDLE_COPYRIGHT "Copyright © 2026 NinoDui"
                   MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    )
elseif(APPLE)
    # Debug/non-bundle macOS builds: create a minimal .app wrapper post-build. Without an app
    # bundle, macOS refuses to register the process with Text Services Manager (TSM), causing IME
    # input methods to fail and spamming the console with: TSMSendMessageToUIServer:
    # CFMessagePortSendRequest FAILED(-1)
    add_custom_command(
        TARGET ThothApp
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_CURRENT_BINARY_DIR}/Thoth.app/Contents/MacOS"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_CURRENT_BINARY_DIR}/Thoth.app/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:ThothApp>"
                "${CMAKE_CURRENT_BINARY_DIR}/Thoth.app/Contents/MacOS/ThothApp"
        COMMENT "Wrapping debug binary in minimal .app bundle for macOS TSM/IME support"
    )
    # Generate a minimal Info.plist for the debug bundle.
    file(
        WRITE "${CMAKE_CURRENT_BINARY_DIR}/Thoth.app/Contents/Info.plist"
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\"
    \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>English</string>
    <key>CFBundleExecutable</key>
    <string>ThothApp</string>
    <key>CFBundleIdentifier</key>
    <string>com.ninodui.thoth.debug</string>
    <key>CFBundleName</key>
    <string>Thoth (Debug)</string>
    <key>CFBundleDisplayName</key>
    <string>Thoth Debug</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleVersion</key>
    <string>${THOTH_VERSION}</string>
    <key>CFBundleShortVersionString</key>
    <string>${THOTH_VERSION}</string>
    <key>LSMinimumSystemVersion</key>
    <string>12.0</string>
    <key>NSMicrophoneUsageDescription</key>
    <string>Thoth uses the microphone to record your shadowing practice sessions.</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
</dict>
</plist>"
    )
endif()
