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
