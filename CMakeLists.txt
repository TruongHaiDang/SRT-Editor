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

find_package(Taglib CONFIG QUIET)
set(TAGLIB_ADDITIONAL_INCLUDE_DIRS "")

if (NOT Taglib_FOUND)
    include(FetchContent)
    FetchContent_Declare(taglib
        GIT_REPOSITORY https://github.com/taglib/taglib.git
        GIT_TAG v1.13
    )
    FetchContent_MakeAvailable(taglib)
    set(TAGLIB_ADDITIONAL_INCLUDE_DIRS
        ${taglib_SOURCE_DIR}
        ${taglib_SOURCE_DIR}/taglib
        ${taglib_SOURCE_DIR}/taglib/toolkit
        ${taglib_BINARY_DIR}
        ${taglib_BINARY_DIR}/taglib
        ${taglib_BINARY_DIR}/taglib/toolkit)
endif()

set(TAGLIB_TARGET TagLib::tag)
if (NOT TARGET ${TAGLIB_TARGET})
    set(TAGLIB_TARGET Taglib::tag)
endif()
if (NOT TARGET ${TAGLIB_TARGET})
    set(TAGLIB_TARGET tag)
endif()
if (NOT TARGET ${TAGLIB_TARGET})
    message(FATAL_ERROR "Unable to locate TagLib target")
endif()

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
    src/text_to_speech_window.cpp
    inc/text_to_speech_window.h
    src/audio.cpp
    inc/audio.h
    ui/main_window.ui
    ui/settings_window.ui
    ui/translator_window.ui
    ui/text_to_speech_window.ui
    ui/app_qrc.qrc
)

target_include_directories(SRT-Editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(SRT-Editor PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    CURL::libcurl
    ${TAGLIB_TARGET}
)

if (TAGLIB_ADDITIONAL_INCLUDE_DIRS)
    target_include_directories(SRT-Editor PRIVATE ${TAGLIB_ADDITIONAL_INCLUDE_DIRS})
endif()
