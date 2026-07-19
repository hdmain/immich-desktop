cmake_minimum_required(VERSION 3.21)

project(immich
    VERSION 0.1.5
    DESCRIPTION "Modern Qt 6 desktop application foundation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core Gui Multimedia MultimediaWidgets Network Svg Widgets
)

set(IMMICH_GITHUB_REPO "hdmain/immich-desktop" CACHE STRING
    "GitHub repository used for in-app update checks")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    src/config/AppVersion.h.in
    "${CMAKE_CURRENT_BINARY_DIR}/generated/AppVersion.h"
    @ONLY
)

set(IMMICH_SOURCES
    src/main.cpp
    src/core/AppSettings.cpp
    src/core/AppSettings.h
    src/core/AutoStart.cpp
    src/core/AutoStart.h
    src/core/ImmichClient.cpp
    src/core/ImmichClient.h
    src/core/ImmichTypes.h
    src/core/OfflineStore.cpp
    src/core/OfflineStore.h
    src/core/SingleInstance.cpp
    src/core/SingleInstance.h
    src/core/Theme.cpp
    src/core/Theme.h
    src/core/ThemeManager.cpp
    src/core/ThemeManager.h
    src/core/ThumbnailCache.cpp
    src/core/ThumbnailCache.h
    src/core/UpdateManager.cpp
    src/core/UpdateManager.h
    src/core/UploadQueueStore.cpp
    src/core/UploadQueueStore.h
    src/core/VideoStreamServer.cpp
    src/core/VideoStreamServer.h
    src/ui/AppIcon.cpp
    src/ui/AppIcon.h
    src/ui/FontLoader.cpp
    src/ui/FontLoader.h
    src/ui/IconUtils.cpp
    src/ui/IconUtils.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/pages/AppearancePage.cpp
    src/ui/pages/AppearancePage.h
    src/ui/pages/ConnectionPage.cpp
    src/ui/pages/ConnectionPage.h
    src/ui/pages/DashboardPage.cpp
    src/ui/pages/DashboardPage.h
    src/ui/pages/ExplorePage.cpp
    src/ui/pages/ExplorePage.h
    src/ui/pages/LibraryPage.cpp
    src/ui/pages/LibraryPage.h
    src/ui/pages/SettingsPage.cpp
    src/ui/pages/SettingsPage.h
    src/ui/pages/UpdatesPage.cpp
    src/ui/pages/UpdatesPage.h
    src/ui/widgets/AnimatedButton.cpp
    src/ui/widgets/AnimatedButton.h
    src/ui/widgets/AnimatedStackedWidget.cpp
    src/ui/widgets/AnimatedStackedWidget.h
    src/ui/widgets/ColorButton.cpp
    src/ui/widgets/ColorButton.h
    src/ui/widgets/MediaTile.cpp
    src/ui/widgets/MediaTile.h
    src/ui/widgets/Sidebar.cpp
    src/ui/widgets/Sidebar.h
    src/ui/widgets/TopBar.cpp
    src/ui/widgets/TopBar.h
    src/ui/widgets/VideoPlayerDialog.cpp
    src/ui/widgets/VideoPlayerDialog.h
    resources/resources.qrc
)

if(WIN32)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        get_filename_component(_immich_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(_immich_windres "${_immich_compiler_dir}/windres.exe")
        if(EXISTS "${_immich_windres}")
            set(CMAKE_RC_COMPILER "${_immich_windres}" CACHE FILEPATH "Path to windres")
        endif()
    endif()

    configure_file(
        resources/immich.ico
        "${CMAKE_CURRENT_BINARY_DIR}/immich.ico"
        COPYONLY
    )
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/immich.rc" "IDI_ICON1 ICON \"immich.ico\"\n")
    list(APPEND IMMICH_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/immich.rc")
endif()

qt_add_executable(immich
    WIN32
    ${IMMICH_SOURCES}
)

target_include_directories(immich PRIVATE
    src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(immich PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Network
    Qt6::Svg
    Qt6::Widgets
)

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
install(FILES LICENSE.txt
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

if(WIN32)
    install(CODE "
        set(runtime_dir \"$<TARGET_FILE_DIR:immich>\")
        file(GLOB runtime_dlls \"\${runtime_dir}/*.dll\")
        if(runtime_dlls)
            file(INSTALL
                DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
                TYPE FILE
                FILES \${runtime_dlls}
            )
        endif()

        file(GLOB runtime_children LIST_DIRECTORIES true \"\${runtime_dir}/*\")
        foreach(child IN LISTS runtime_children)
            if(IS_DIRECTORY \"\${child}\")
                file(GLOB plugin_dlls \"\${child}/*.dll\")
                if(plugin_dlls)
                    get_filename_component(plugin_name \"\${child}\" NAME)
                    file(INSTALL
                        DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/\${plugin_name}\"
                        TYPE FILE
                        FILES \${plugin_dlls}
                    )
                endif()
            endif()
        endforeach()
    ")
endif()

if(UNIX AND NOT APPLE)
    install(FILES resources/immich.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES resources/immich-desktop.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    )
    install(FILES resources/immich-logo-512.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps
        RENAME immich.png
    )
endif()

set(CPACK_PACKAGE_NAME "immich-desktop")
set(CPACK_PACKAGE_VENDOR "Immich Desktop Community")
set(CPACK_PACKAGE_CONTACT "Immich Desktop Community")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Unofficial fan-made desktop client for Immich"
)
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "immich desktop")
set(CPACK_PACKAGE_EXECUTABLES "immich" "immich")
set(CPACK_CREATE_DESKTOP_LINKS "immich")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_FILE_NAME
    "immich-desktop-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}"
)

if(WIN32)
    set(CPACK_GENERATOR "NSIS;WIX")
    set(CPACK_NSIS_DISPLAY_NAME "immich desktop")
    set(CPACK_NSIS_PACKAGE_NAME "immich desktop")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\immich.exe")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/immich.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/immich.ico")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources\\\\immich-installer.bmp")
    set(CPACK_WIX_VERSION 3)
    set(CPACK_WIX_UPGRADE_GUID "A62C0898-DA73-47EB-A640-6FC2F37DFA1F")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/immich.ico")
    set(CPACK_WIX_LICENSE_RTF "${CMAKE_CURRENT_SOURCE_DIR}/resources/LICENSE.rtf")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Immich Desktop Community")
    set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
