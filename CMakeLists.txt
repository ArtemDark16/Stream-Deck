cmake_minimum_required(VERSION 3.21)

project(kursova LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)
if (Qt6_FOUND)
    set(QT_PACKAGE Qt6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
    set(QT_PACKAGE Qt5)
endif()

add_executable(kursova WIN32
    kursova/App.cpp
    kursova/Actions.cpp
    kursova/Config.cpp
    kursova/kursova.cpp
)

target_include_directories(kursova PRIVATE kursova)
target_link_libraries(kursova PRIVATE ${QT_PACKAGE}::Widgets Shell32)

if (MSVC)
    target_compile_options(kursova PRIVATE /W3 /permissive-)
endif()

add_custom_command(TARGET kursova POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/kursova/data"
        "$<TARGET_FILE_DIR:kursova>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/kursova/kursova.ico"
        "$<TARGET_FILE_DIR:kursova>/kursova.ico"
)
