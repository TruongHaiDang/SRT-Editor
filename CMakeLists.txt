cmake_minimum_required(VERSION 3.16)

project(SRT-Editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(CURL REQUIRED)

add_executable(SRT-Editor
    src/main.cpp
    inc/main.h
    src/main_window.cpp
    inc/main_window.h
    src/settings.cpp
    inc/settings.h
    src/settings_window.cpp
    inc/settings_window.h
    src/translator.cpp
    inc/translator.h
    src/translator_window.cpp
    inc/translator_window.h
    ui/main_window.ui
    ui/settings_window.ui
    ui/translator_window.ui
    ui/app_qrc.qrc
)

target_include_directories(SRT-Editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(SRT-Editor PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    CURL::libcurl
)
