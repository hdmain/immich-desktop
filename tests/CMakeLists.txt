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

target_link_libraries(TlsConfigTest PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Test
)

add_test(NAME TlsConfigTest COMMAND TlsConfigTest)

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
