#ifndef WIN_SWITCHER_STARTUP_H
#define WIN_SWITCHER_STARTUP_H

#include <QString>
#include <QSettings>
#include <QDir>
#include <QApplication>
#include <QDebug>

class Startup {
public:
    Startup() = delete;

    static void on() {
        QSettings reg(REG_AUTORUN, QSettings::NativeFormat);
        reg.setValue(REG_APP_NAME, applicationPath());
    }

    static void off() {
        QSettings reg(REG_AUTORUN, QSettings::NativeFormat);
        reg.remove(REG_APP_NAME);
    }

    static void toggle() {
        isOn() ? off() : on();
    }

    static void set(bool _on) {
        _on ? on() : off();
    }

    static bool isOn() {
        QSettings reg(REG_AUTORUN, QSettings::NativeFormat);
        auto appPath = applicationPath();
        auto path = reg.value(REG_APP_NAME);
        if (path.isValid() && path.toString() != appPath) // just for warning
            qWarning() << "REG: AutoRun path mismatch:" << path.toString() << appPath;
        return path.toString() == appPath;
    }

    static QString applicationPath() {
        return QDir::toNativeSeparators(QApplication::applicationFilePath());
    }

private:
    // HKEY_CURRENT_USER 仅仅对当前用户有效，但不需要管理员权限
    inline static const auto REG_AUTORUN = R"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";
    // 标识符，不能重复
    inline static const auto REG_APP_NAME = "AltTaber.MrBeanCpp";
};

#endif //WIN_SWITCHER_STARTUP_H
