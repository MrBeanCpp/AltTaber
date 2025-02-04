#ifndef WIN_SWITCHER_CONFIGMANAGER_H
#define WIN_SWITCHER_CONFIGMANAGER_H

#include <QSettings>
#include <QApplication>
#include "ConfigManagerBase.h"

// 注意：对于大量使用的类，header-only 模式会导致编译时间过长
#define cfg ConfigManager::instance()

enum DisplayMonitor {
    PrimaryMonitor, // 0 主显示器
    MouseMonitor, // 1 跟随鼠标
    EnumCount // Just for count
};

class ConfigManager : public ConfigManagerBase {
    inline static const QString FileName = "config.ini";

public:
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    static ConfigManager& instance() {
        static const auto filePath = QApplication::applicationDirPath() + "/" + FileName;
        static ConfigManager instance{filePath}; // multiple threads safe
        return instance;
    }

public:
    DisplayMonitor getDisplayMonitor() {
        auto monitor = get("DisplayMonitor", DisplayMonitor::MouseMonitor).toInt();
        if (monitor < 0 || monitor >= DisplayMonitor::EnumCount) {
            qWarning() << "Invalid DisplayMonitor enum" << monitor;
            monitor = DisplayMonitor::MouseMonitor;
        }
        return static_cast<DisplayMonitor>(monitor);
    }

    void setDisplayMonitor(DisplayMonitor monitor) {
        set("DisplayMonitor", monitor);
    }

private:
    explicit ConfigManager(const QString& filename) : ConfigManagerBase(filename) {}
};


#endif //WIN_SWITCHER_CONFIGMANAGER_H
