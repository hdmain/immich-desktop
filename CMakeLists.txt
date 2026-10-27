cmake_minimum_required(VERSION 3.21)

project(immich
    VERSION 0.2.3
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
    Core Gui LinguistTools Multimedia MultimediaWidgets Network Svg Widgets
)

set(IMMICH_GITHUB_REPO "hdmain/immich-desktop" CACHE STRING
    "GitHub repository used for in-app update checks")
# Distribution packages usually manage updates and naming themselves.
option(IMMICH_ENABLE_UPDATER "Check GitHub for new releases and offer to install them" ON)
set(IMMICH_EXECUTABLE_NAME "immich" CACHE STRING "File name of the installed executable")
set(IMMICH_BUILD_INFO "" CACHE STRING
    "Shown after the version, e.g. the package source or commit (\"AUR\", \"beta 1a2b3c4\")")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    src/config/AppVersion.h.in
    "${CMAKE_CURRENT_BINARY_DIR}/generated/AppVersion.h"
    @ONLY
)

set(IMMICH_SOURCES
    src/core/AppSettings.cpp
    src/core/AppSettings.h
    src/core/CustomHeaders.cpp
    src/core/CustomHeaders.h
    src/core/AutoStart.cpp
    src/core/AutoStart.h
    src/core/FolderSyncService.cpp
    src/core/FolderSyncService.h
    src/core/ImmichClient.cpp
    src/core/ImmichClient.h
    src/core/Localization.cpp
    src/core/Localization.h
    src/core/TranslationUpdater.cpp
    src/core/TranslationUpdater.h
    src/core/ImmichTypes.h
    src/core/OfflineStore.cpp
    src/core/OfflineStore.h
    src/core/ProcessMemory.cpp
    src/core/ProcessMemory.h
    src/core/SingleInstance.cpp
    src/core/SingleInstance.h
    src/core/Theme.cpp
    src/core/Theme.h
    src/core/ThemeManager.cpp
    src/core/ThemeManager.h
    src/core/ThumbnailCache.cpp
    src/core/ThumbnailCache.h
    src/core/TlsConfig.cpp
    src/core/TlsConfig.h
    src/core/UpdateManager.cpp
    src/core/UpdateManager.h
    src/core/UploadQueueStore.cpp
    src/core/UploadQueueStore.h
    src/core/VideoStreamServer.cpp
    src/core/VideoStreamServer.h
    src/ui/AppIcon.cpp
    src/ui/AppIcon.h
    src/ui/FontLoader.cpp
    src/ui/BrowseSection.h
    src/ui/FolderPaths.h
    src/ui/FontLoader.h
    src/ui/GridZoom.h
    src/ui/IconUtils.cpp
    src/ui/IconUtils.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MainWindowHost.cpp
    src/ui/MainWindowHost.h
    src/ui/TileImageScheduler.cpp
    src/ui/TileImageScheduler.h
    src/ui/pages/AboutPage.cpp
    src/ui/pages/AboutPage.h
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
    src/ui/pages/SyncFolderPage.cpp
    src/ui/pages/SyncFolderPage.h
    src/ui/pages/UpdatesPage.cpp
    src/ui/pages/UpdatesPage.h
    src/ui/widgets/AnimatedButton.cpp
    src/ui/widgets/AnimatedButton.h
    src/ui/widgets/AnimatedStackedWidget.cpp
    src/ui/widgets/AnimatedStackedWidget.h
    src/ui/widgets/AssetInfoPanel.cpp
    src/ui/widgets/AssetInfoPanel.h
    src/ui/widgets/BrowseTree.cpp
    src/ui/widgets/BrowseTree.h
    src/ui/widgets/ColorButton.cpp
    src/ui/widgets/ColorButton.h
    src/ui/widgets/FlowLayout.cpp
    src/ui/widgets/FlowLayout.h
    src/ui/widgets/MediaTile.cpp
    src/ui/widgets/MediaTile.h
    src/ui/widgets/NoWheelComboBox.h
    src/ui/widgets/MediaViewerDialog.cpp
    src/ui/widgets/MediaViewerDialog.h
    src/ui/widgets/ScrollToTopButton.cpp
    src/ui/widgets/ScrollToTopButton.h
    src/ui/widgets/ShareLinkDialog.cpp
    src/ui/widgets/ShareLinkDialog.h
    src/ui/widgets/SummaryTile.cpp
    src/ui/widgets/SummaryTile.h
    src/ui/widgets/Sidebar.cpp
    src/ui/widgets/Sidebar.h
    src/ui/widgets/Toast.cpp
    src/ui/widgets/Toast.h
    src/ui/widgets/TopBar.cpp
    src/ui/widgets/TopBar.h
    src/ui/widgets/SortButton.cpp
    src/ui/widgets/SortButton.h
    src/ui/widgets/SoftwareVideoWidget.cpp
    src/ui/widgets/SoftwareVideoWidget.h
    src/ui/widgets/VideoHoverPreview.cpp
    src/ui/widgets/VideoHoverPreview.h
    src/ui/widgets/VideoPlayerWidget.cpp
    src/ui/widgets/VideoPlayerWidget.h
    src/ui/widgets/ZoomPanWidget.cpp
    src/ui/widgets/ZoomPanWidget.h
    src/ui/widgets/ZoomSlider.cpp
    src/ui/widgets/ZoomSlider.h
)

# Share the application UI with LocalizationUiTest. STATIC (not OBJECT): MSVC
# + AUTORCC fails to link dependents against qrc_*.obj from OBJECT libraries.
#
# Icons/fonts (resources.qrc) and the Windows .ico (.rc) must live on the
# *executable* targets. MinGW does not merge .rsrc from static libraries into
# the PE, which left immich.exe with an empty resource table and no app icon.
qt_add_library(immich_app STATIC ${IMMICH_SOURCES})
qt_add_executable(immich WIN32
    src/main.cpp
    resources/resources.qrc
)
target_link_libraries(immich PRIVATE immich_app)

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
    target_sources(immich PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/immich.rc")
endif()

# Qt 6.2-compatible Linguist workflow. Catalogs are embedded so installers,
# AppImages and sandboxed packages use the same translations without extra paths.
file(GLOB IMMICH_TS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/translations/immich_*.ts")
qt_add_translation(IMMICH_QM_FILES ${IMMICH_TS_FILES} OPTIONS -nounfinished)
# Explicit target so MSBuild parallel builds wait for lrelease before rcc embeds .qm.
add_custom_target(immich_translations DEPENDS ${IMMICH_QM_FILES})
find_program(IMMICH_PYTHON NAMES python3 python)
set(_immich_update_translation_status)
if(IMMICH_PYTHON)
    set(_immich_update_translation_status
        COMMAND "${IMMICH_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/update_translation_status.py")
endif()
add_custom_target(update_translations
    COMMAND Qt6::lupdate "${CMAKE_CURRENT_SOURCE_DIR}/src"
        -no-obsolete -locations relative -source-language en -ts ${IMMICH_TS_FILES}
    ${_immich_update_translation_status}
    COMMENT "Updating translation source catalogs and README status table"
    VERBATIM
)
get_target_property(_immich_qmake Qt6::qmake IMPORTED_LOCATION)
execute_process(COMMAND "${_immich_qmake}" -query QT_INSTALL_TRANSLATIONS
    OUTPUT_VARIABLE _immich_qt_translations OUTPUT_STRIP_TRAILING_WHITESPACE)
set(IMMICH_QT_QM_FILES)
foreach(ts_file IN LISTS IMMICH_TS_FILES)
    get_filename_component(catalog "${ts_file}" NAME_WE)
    string(REGEX REPLACE "^immich_" "" language "${catalog}")
    if(EXISTS "${_immich_qt_translations}/qtbase_${language}.qm")
        list(APPEND IMMICH_QT_QM_FILES "${_immich_qt_translations}/qtbase_${language}.qm")
    endif()
endforeach()
foreach(qm_file IN LISTS IMMICH_QM_FILES IMMICH_QT_QM_FILES)
    get_filename_component(qm_name "${qm_file}" NAME)
    set_source_files_properties("${qm_file}" PROPERTIES QT_RESOURCE_ALIAS "${qm_name}")
endforeach()
qt_add_resources(immich translations PREFIX "/i18n"
    FILES ${IMMICH_QM_FILES} ${IMMICH_QT_QM_FILES})
add_dependencies(immich immich_translations)

target_include_directories(immich_app PUBLIC
    src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(immich_app PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Network
    Qt6::Svg
    Qt6::Widgets
)

if(WIN32)
    target_link_libraries(immich_app PUBLIC crypt32 dwmapi Psapi)

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

set_target_properties(immich PROPERTIES OUTPUT_NAME "${IMMICH_EXECUTABLE_NAME}")
if(NOT IMMICH_ENABLE_UPDATER)
    target_compile_definitions(immich_app PUBLIC IMMICH_UPDATER_DISABLED)
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
    configure_file(resources/immich-desktop.desktop.in
        "${CMAKE_CURRENT_BINARY_DIR}/immich-desktop.desktop" @ONLY
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/immich-desktop.desktop"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES resources/immich-desktop.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    )
    foreach(_immich_icon_size IN ITEMS 16 32 48 64 128 256 512)
        install(FILES "resources/immich-logo-${_immich_icon_size}.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_immich_icon_size}x${_immich_icon_size}/apps"
            RENAME immich-desktop.png
        )
    endforeach()
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

enable_testing()
add_subdirectory(tests)
