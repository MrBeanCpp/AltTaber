#ifndef WIN_SWITCHER_SYSTEMTRAY_H
#define WIN_SWITCHER_SYSTEMTRAY_H

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QActionGroup>
#include "Startup.h"
#include "ConfigManager.h"
#include "UpdateDialog.h"

#define sysTray SystemTray::instance()

class SystemTray : public QSystemTrayIcon {
public:
    SystemTray(const SystemTray&) = delete;
    SystemTray& operator=(const SystemTray&) = delete;

    static SystemTray& instance() {
        // 第一次调用时 才会构造
        static SystemTray instance;
        return instance;
    }

private:
    explicit SystemTray(QWidget* parent = nullptr) : QSystemTrayIcon(parent) {
        setIcon(QIcon(":/img/icon.ico"));
        setMenu(parent);
        setToolTip(IsUserAnAdmin() ? "AltTaber (admin)" : "AltTaber");
    }

    void setMenu(QWidget* parent = nullptr) {
        auto* menu = new QMenu(parent);
        menu->setStyleSheet("QMenu{"
            "background-color:rgb(45,45,45);"
            "color:rgb(220,220,220);"
            "border:1px solid black;"
            "}"
            "QMenu:selected{ background-color:rgb(60,60,60); }");

        auto* act_update = new QAction("Check for Updates", menu);
        auto* act_settings = new QAction("Settings", menu);
        auto* act_startup = new QAction("Start with Windows", menu);
        auto* menu_monitor = new QMenu("Display Monitor", menu);
        auto* act_quit = new QAction("Quit >", menu);

        connect(act_update, &QAction::triggered, this, [] {
            // !对于UI窗体，不要用static变量(`static UpdateDialog dlg;`)，要么局部变量，要么指针
            // ! static局部变量会在程序结束时析构 ! 析构时可能访问qApp资源，而qApp由于`qApp->quit()`已经被清理
            // !导致报错（"No style available without QApplication!"）
            static auto* dlg = new UpdateDialog;
            dlg->show();
        });

        connect(act_settings, &QAction::triggered, this, [] {
            cfg.editConfigFile();
        });
        connect(&cfg, &ConfigManager::configEdited, this, [this] {
            this->showMessage("Config Edited", "auto reloaded");
        });

        act_startup->setCheckable(true);
        // triggered vs toggled: setChecked() will emit `toggled`, but not `triggered` (which is pure user action)
        connect(act_startup, &QAction::triggered, this, [this](bool checked) {
            Startup::toggle();
            if (Startup::isOn() == checked)
                this->showMessage("auto Startup mode", checked ? "ON √" : "OFF ×");
            else
                this->showMessage("Action Failed", "Failed to change Startup mode", Warning);
        });
        // aboutToShow 时查询，反映真实状态
        connect(menu, &QMenu::aboutToShow, act_startup, [act_startup] {
            act_startup->setChecked(Startup::isOn()); // 10-30ms

            static auto text = act_startup->text();
            if (IsUserAnAdmin() && !Startup::isOn_reg())
                act_startup->setText(text + "🔑️"); // 意味着接下来的操作需要管理员权限（操作schtask）
            else
                act_startup->setText(text);
        });

        // menu_monitor
        {
            auto* monitorGroup = new QActionGroup(menu_monitor);
            monitorGroup->addAction("Primary Monitor")->setData(PrimaryMonitor);
            monitorGroup->addAction("Mouse Monitor")->setData(MouseMonitor);

            const auto actions = monitorGroup->actions();
            for (auto* act: actions)
                act->setCheckable(true);

            Q_ASSERT(actions.size() == DisplayMonitor::EnumCount);
            menu_monitor->addActions(actions);

            // 动态响应配置文件修改
            connect(menu_monitor, &QMenu::aboutToShow, this, [monitorGroup] {
                qDebug() << "menu_monitor aboutToShow";
                const auto actions = monitorGroup->actions();
                actions[cfg.getDisplayMonitor()]->setChecked(true);
            });

            connect(monitorGroup, &QActionGroup::triggered, this, [this](QAction* act) {
                auto monitor = static_cast<DisplayMonitor>(act->data().toInt());
                cfg.setDisplayMonitor(monitor);
                this->showMessage("Display Monitor Changed", act->text());
            });
        }

        connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);

        menu->addAction(act_update);
        menu->addAction(act_settings);
        menu->addAction(act_startup);
        menu->addMenu(menu_monitor);
        menu->addAction(act_quit);
        this->setContextMenu(menu);
    }
};

#endif //WIN_SWITCHER_SYSTEMTRAY_H
