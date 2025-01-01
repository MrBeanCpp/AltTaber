cmake_minimum_required(VERSION 3.29)
project(Win_Switcher)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_PREFIX_PATH "D:/Qt/6.8.1/msvc2022_64/lib/cmake")
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS ui) # 如果.ui文件不在.cpp同一目录下，需要添加搜索路径

# 设置版本信息变量
set(VERSION_MAJOR 0)
set(VERSION_MINOR 3)
set(VERSION_PATCH 0)

set(COMPANY_NAME "MrBeanCpp")
set(FILE_DESCRIPTION "AltTaber - Window Switcher")
set(PRODUCT_NAME "AltTaber")
# 设置图标的相对路径
# 图标会被缓存，修改后不生效的话，需要删除AppData\Local\IconCache.db & 重启资源管理器，该文件在PC重启后自动创建
set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/img/icon.ico")

set(VERSIONINFO_RC "${CMAKE_CURRENT_BINARY_DIR}/VersionInfo.rc")
# 处理.in模板文件 生成版本信息文件
configure_file(
        VersionInfo.rc.in
        ${VERSIONINFO_RC}
        @ONLY # 只处理@var@格式的变量
)

include_directories(header)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Xml
        REQUIRED)

add_executable(Win_Switcher src/main.cpp
        ${VERSIONINFO_RC}
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
        header/utils/IconOnlyDelegate.h
        header/utils/uiautomation.h
        src/utils/uiautomation.cpp
        src/utils/AppUtil.cpp
        header/utils/AppUtil.h
        src/utils/KeyboardHooker.cpp
        header/utils/KeyboardHooker.h
        src/utils/TaskbarWheelHooker.cpp
        header/utils/TaskbarWheelHooker.h
        src/utils/QtWin.cpp
        header/utils/QtWin.h
        header/utils/ComInitializer.h)
target_link_libraries(Win_Switcher
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Xml
)

target_link_libraries(${PROJECT_NAME}
        Dwmapi
)

# 启用`gui_private`模块
# 必须放在find_package之后，否则`Qt6Gui_PRIVATE_INCLUDE_DIRS`变量未定义
#target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "AltTaber") # 设置输出文件名
if (CMAKE_BUILD_TYPE MATCHES "Release")
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE true) # 设置为窗口应用程序, 无控制台，同时会导致Clion无法捕获输出
endif ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        QT_DEPRECATED_WARNINGS
        UNICODE # for Windows API，使用W版本
        _UNICODE # for C/C++ 标准库
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/utf-8")
endif ()