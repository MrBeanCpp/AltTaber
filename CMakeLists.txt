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
        WinExtras
        Xml
        REQUIRED)

add_executable(Win_Switcher src/main.cpp
        src/widget.cpp
        header/widget.h
        ui/widget.ui
        src/utils/winEventHook.cpp
        header/utils/winEventHook.h
        src/utils/Util.cpp
        header/utils/Util.h
        header/utils/setWindowBlur.h
        src/utils/setWindowBlur.cpp
        src/utils/IconOnlyDelegate.cpp
        header/utils/IconOnlyDelegate.h)
target_link_libraries(Win_Switcher
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::WinExtras
        Qt5::Xml
)

target_link_libraries(${PROJECT_NAME}
        Dwmapi
        Comctl32
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        QT_DEPRECATED_WARNINGS
        UNICODE # for Windows API，使用W版本
        _UNICODE # for C/C++ 标准库
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/utf-8")
endif()