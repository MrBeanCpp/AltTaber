#ifndef WIN_SWITCHER_STARTUP_H
#define WIN_SWITCHER_STARTUP_H

#include <QString>
#include <QSettings>
#include <QDir>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <shlobj_core.h>

// ScheduledTask.h 可选，作为一个插件，用以支持以[管理员权限]自启动
#if __has_include("ScheduledTask.h")
# define HAS_SCHTASK
# include "ScheduledTask.h"
#else
# warning "ScheduledTask.h not found, use registry instead."
#endif

class Startup {
public:
    Startup() = delete;
    friend class SystemTray;

    static void on() {
#ifdef HAS_SCHTASK
        // MS doc: 此函数是 CheckTokenMembership 的包装器, 建议直接调用该函数来确定管理员组状态; 而不是调用 IsUserAnAdmin
        if (IsUserAnAdmin()) {
            off_reg();
            if (!ScheduledTask::createTask(SCHTASK_NAME))
                QMessageBox::warning(nullptr, "Failed to create ScheduledTask", "maybe check log?(if any)");
            // 这里使用`QMessageBox`而非`sysTray.showMessage`，是为了避免循环依赖 & 保持Startup独立性
        } else {
            // 但是一般认为 reg || schtasks 有一个存在就是自启动了，此时不会调用该函数，也不会进入这个if
            if (ScheduledTask::queryTask(SCHTASK_NAME)) { // no admin, can't delete
                QMessageBox::warning(nullptr, "Conflict: schtask vs reg",
                                     "ScheduledTask exists, but no privilege to delete. Please run as Administrator & do it again.");
            } else
                on_reg();
        }
#else
        on_reg();
#endif
    }

    static void off() {
        off_reg();
#ifdef HAS_SCHTASK
        if (ScheduledTask::queryTask(SCHTASK_NAME)) {
            if (!ScheduledTask::deleteTask(SCHTASK_NAME)) {
                bool isAdmin = IsUserAnAdmin();
                qWarning() << "Failed to delete ScheduledTask:" << SCHTASK_NAME << isAdmin;
                QMessageBox::warning(nullptr, QString("Failed to delete ScheduledTask: %1").arg(SCHTASK_NAME),
                                     isAdmin ?
                                     "em mm, something went wrong... maybe check log?(if any)" :
                                     "Please run as Administrator & do it again.");
            }
        }
#endif
    }

    static void toggle() {
        isOn() ? off() : on();
    }

    static void set(bool _on) {
        _on ? on() : off();
    }

    static bool isOn() {
#ifdef HAS_SCHTASK
        return isOn_reg() || ScheduledTask::queryTask(SCHTASK_NAME);
#else
        return isOn_reg();
#endif
    }

private:
    static QString applicationPath() {
        return QDir::toNativeSeparators(QApplication::applicationFilePath());
    }

    static void on_reg() {
        QSettings reg(REG_AUTORUN, QSettings::NativeFormat);
        reg.setValue(REG_APP_NAME, applicationPath());
    }

    static void off_reg() {
        QSettings reg(REG_AUTORUN, QSettings::NativeFormat);
        reg.remove(REG_APP_NAME);
    }

    static bool isOn_reg() {
        QSettings reg(REG_AUTORUN, QSettings::NativeFormat);
        auto appPath = applicationPath();
        auto path = reg.value(REG_APP_NAME);
        if (path.isValid() && path.toString() != appPath) // just for warning
            qWarning() << "REG: AutoRun path mismatch:" << path.toString() << appPath;
        return path.toString() == appPath;
    }

private:
    // HKEY_CURRENT_USER 仅仅对当前用户有效，但不需要管理员权限
    inline static const auto REG_AUTORUN = R"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";
    // 标识符，不能重复
    inline static const auto REG_APP_NAME = "AltTaber.MrBeanCpp";
#ifdef HAS_SCHTASK
    inline static const auto SCHTASK_NAME = "AltTaber Startup";
#endif
};

#endif //WIN_SWITCHER_STARTUP_H
