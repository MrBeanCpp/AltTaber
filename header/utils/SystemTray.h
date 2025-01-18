#ifndef WIN_SWITCHER_SYSTEMTRAY_H
#define WIN_SWITCHER_SYSTEMTRAY_H

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QActionGroup>
#include "Startup.h"
#include "ConfigManager.h"

class SystemTray : public QSystemTrayIcon {
public:
    explicit SystemTray(QWidget* parent = nullptr) {
        setIcon(QIcon(":/img/icon.ico"));
        setMenu(parent);
        setToolTip("AltTaber");
    }

private:
    void setMenu(QWidget* parent = nullptr) {
        auto* menu = new QMenu(parent);
        menu->setStyleSheet("QMenu{"
                            "background-color:rgb(45,45,45);"
                            "color:rgb(220,220,220);"
                            "border:1px solid black;"
                            "}"
                            "QMenu:selected{ background-color:rgb(60,60,60); }");
        auto* act_startup = new QAction("Start with Windows", menu);
        auto* menu_monitor = new QMenu("Display Monitor", menu);
        auto* act_quit = new QAction("Quit >", menu);

        act_startup->setCheckable(true);
        act_startup->setChecked(Startup::isOn());
        connect(act_startup, &QAction::toggled, this, [this](bool checked) {
            Startup::toggle();
            this->showMessage("auto Startup mode", checked ? "ON √" : "OFF ×");
        });

        { // menu_monitor
            auto* monitorGroup = new QActionGroup(menu_monitor);
            monitorGroup->addAction("Primary Monitor")->setData(PrimaryMonitor);
            monitorGroup->addAction("Mouse Monitor")->setData(MouseMonitor);

            const auto actions = monitorGroup->actions();
            for (auto* act: actions)
                act->setCheckable(true);

            Q_ASSERT(actions.size() == DisplayMonitor::EnumCount);
            actions[cfg.getDisplayMonitor()]->setChecked(true);
            menu_monitor->addActions(actions);

            connect(monitorGroup, &QActionGroup::triggered, this, [this](QAction* act) {
                DisplayMonitor monitor = (DisplayMonitor) act->data().toInt();
                cfg.setDisplayMonitor(monitor);
                this->showMessage("Display Monitor Changed", act->text());
            });
        }

        connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);

        menu->addAction(act_startup);
        menu->addMenu(menu_monitor);
        menu->addAction(act_quit);
        this->setContextMenu(menu);
    }
};

#endif //WIN_SWITCHER_SYSTEMTRAY_H
