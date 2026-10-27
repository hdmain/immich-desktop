find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Network Test)

qt_add_executable(TlsConfigTest
    TlsConfigTest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/CustomHeaders.cpp
    ${CMAKE_SOURCE_DIR}/src/core/CustomHeaders.h
    ${CMAKE_SOURCE_DIR}/src/core/TlsConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/core/TlsConfig.h
    ${CMAKE_SOURCE_DIR}/src/core/VideoStreamServer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/VideoStreamServer.h
)

target_include_directories(TlsConfigTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Gui headers come in via AppSettings → Theme (QColor). QTEST_GUILESS_MAIN still
# uses QCoreApplication, so headless CI does not need a display/platform plugin.
target_link_libraries(TlsConfigTest PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Test
)

add_test(NAME TlsConfigTest COMMAND TlsConfigTest)
set_tests_properties(TlsConfigTest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(CustomHeadersTest
    CustomHeadersTest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/CustomHeaders.cpp
    ${CMAKE_SOURCE_DIR}/src/core/CustomHeaders.h
)

target_include_directories(CustomHeadersTest PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(CustomHeadersTest PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
)

add_test(NAME CustomHeadersTest COMMAND CustomHeadersTest)

qt_add_executable(LocalizationTest LocalizationTest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Localization.cpp)
target_include_directories(LocalizationTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(LocalizationTest PRIVATE Qt6::Core Qt6::Test)
foreach(qm_file IN LISTS IMMICH_QM_FILES IMMICH_QT_QM_FILES)
    get_filename_component(qm_name "${qm_file}" NAME)
    set_source_files_properties("${qm_file}" PROPERTIES QT_RESOURCE_ALIAS "${qm_name}")
endforeach()
qt_add_resources(LocalizationTest test_translations PREFIX "/i18n"
    FILES ${IMMICH_QM_FILES} ${IMMICH_QT_QM_FILES})
add_dependencies(LocalizationTest immich_translations)
add_test(NAME LocalizationTest COMMAND LocalizationTest)

qt_add_executable(LocalizationUiTest
    LocalizationUiTest.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc)
target_link_libraries(LocalizationUiTest PRIVATE immich_app Qt6::Test)
qt_add_resources(LocalizationUiTest ui_test_translations PREFIX "/i18n"
    FILES ${IMMICH_QM_FILES} ${IMMICH_QT_QM_FILES})
add_dependencies(LocalizationUiTest immich_translations)
add_test(NAME LocalizationUiTest COMMAND LocalizationUiTest)
set_tests_properties(LocalizationUiTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(TranslationUpdaterTest TranslationUpdaterTest.cpp
    ${CMAKE_SOURCE_DIR}/src/core/TranslationUpdater.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Localization.cpp)
target_include_directories(TranslationUpdaterTest PRIVATE ${CMAKE_SOURCE_DIR}/src
    "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(TranslationUpdaterTest PRIVATE Qt6::Core Qt6::Network Qt6::Test)
qt_add_resources(TranslationUpdaterTest updater_translations PREFIX "/i18n"
    FILES ${IMMICH_QM_FILES})
qt_add_translation(REMOTE_TEST_QM translations/remote_de.ts OPTIONS -nounfinished)
set_source_files_properties(${REMOTE_TEST_QM} PROPERTIES QT_RESOURCE_ALIAS "remote_de.qm")
qt_add_resources(TranslationUpdaterTest updater_fixtures PREFIX "/fixtures" FILES ${REMOTE_TEST_QM})
add_dependencies(TranslationUpdaterTest immich_translations)
add_test(NAME TranslationUpdaterTest COMMAND TranslationUpdaterTest)

if(IMMICH_PYTHON)
    add_test(NAME PolishCatalogTest COMMAND "${IMMICH_PYTHON}" -X utf8
        "${CMAKE_CURRENT_SOURCE_DIR}/PolishCatalogTest.py")
    add_test(NAME ChineseCatalogTest COMMAND "${IMMICH_PYTHON}" -X utf8
        "${CMAKE_CURRENT_SOURCE_DIR}/ChineseCatalogTest.py")
endif()

qt_add_executable(ProfileTest ProfileTest.cpp ${CMAKE_SOURCE_DIR}/resources/resources.qrc)
target_link_libraries(ProfileTest PRIVATE immich_app Qt6::Test)
add_test(NAME ProfileTest COMMAND ProfileTest)
set_tests_properties(ProfileTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
