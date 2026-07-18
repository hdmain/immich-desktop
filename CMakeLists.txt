cmake_minimum_required(VERSION 3.21)

project(immich
    VERSION 0.1.0
    DESCRIPTION "Modern Qt 6 desktop application foundation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Svg Widgets)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    src/config/AppVersion.h.in
    "${CMAKE_CURRENT_BINARY_DIR}/generated/AppVersion.h"
    @ONLY
)

qt_add_executable(immich
    WIN32
    src/main.cpp
    src/core/AppSettings.cpp
    src/core/AppSettings.h
    src/core/Theme.cpp
    src/core/Theme.h
    src/core/ThemeManager.cpp
    src/core/ThemeManager.h
    src/ui/IconUtils.cpp
    src/ui/IconUtils.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/pages/AppearancePage.cpp
    src/ui/pages/AppearancePage.h
    src/ui/pages/DashboardPage.cpp
    src/ui/pages/DashboardPage.h
    src/ui/widgets/AnimatedButton.cpp
    src/ui/widgets/AnimatedButton.h
    src/ui/widgets/AnimatedStackedWidget.cpp
    src/ui/widgets/AnimatedStackedWidget.h
    src/ui/widgets/ColorButton.cpp
    src/ui/widgets/ColorButton.h
    src/ui/widgets/Sidebar.cpp
    src/ui/widgets/Sidebar.h
    src/ui/widgets/TopBar.cpp
    src/ui/widgets/TopBar.h
    resources/resources.qrc
)

target_include_directories(immich PRIVATE
    src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(immich PRIVATE Qt6::Core Qt6::Gui Qt6::Svg Qt6::Widgets)

if(WIN32)
    target_link_libraries(immich PRIVATE dwmapi)

    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIRECTORY "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${QT_BIN_DIRECTORY}"
        REQUIRED
    )

    add_custom_command(TARGET immich POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-translations
            --compiler-runtime
            "$<TARGET_FILE:immich>"
        COMMENT "Deploying Qt runtime libraries"
        VERBATIM
    )
endif()

include(GNUInstallDirs)
install(TARGETS immich
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES resources/immich.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES resources/immich-logo.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/1024x1024/apps
        RENAME immich.png
    )
endif()
