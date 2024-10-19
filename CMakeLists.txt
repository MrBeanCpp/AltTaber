cmake_minimum_required(VERSION 3.29)
project(Win_Switcher)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Qt/5.15.2/msvc2019_64")
include_directories(header)
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS ui) # 如果.ui文件不在.cpp同一目录下，需要添加搜索路径

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

add_executable(Win_Switcher src/main.cpp
        src/widget.cpp
        header/widget.h
        ui/widget.ui
        src/winEventHook.cpp
        header/winEventHook.h)
target_link_libraries(Win_Switcher
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/utf-8")
endif()