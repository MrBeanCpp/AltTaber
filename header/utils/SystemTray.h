#ifndef WIN_SWITCHER_SYSTEMTRAY_H
#define WIN_SWITCHER_SYSTEMTRAY_H

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>
#include "Startup.h"

class SystemTray : public QSystemTrayIcon {
public:
    SystemTray(QWidget* parent = nullptr) {
        setIcon(QIcon(":/img/icon.ico"));
        setMenu(parent);
        setToolTip("AltTaber");
    }

private:
    void setMenu(QWidget* parent = nullptr) {
        QMenu* menu = new QMenu(parent);
        menu->setStyleSheet("QMenu{"
                            "background-color:rgb(45,45,45);"
                            "color:rgb(220,220,220);"
                            "border:1px solid black;"
                            "}"
                            "QMenu:selected{ background-color:rgb(60,60,60); }");
        QAction* act_startup = new QAction("Start with Windows", menu);
        QAction* act_quit = new QAction("Quit >", menu);

        act_startup->setCheckable(true);
        act_startup->setChecked(Startup::isOn());
        connect(act_startup, &QAction::toggled, this, [this](bool checked) {
            Startup::toggle();
            this->showMessage("auto Startup mode", checked ? "ON √" : "OFF ×");
        });

        connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);
        menu->addActions(QList<QAction*>() << act_startup << act_quit);
        this->setContextMenu(menu);
    }
};

#endif //WIN_SWITCHER_SYSTEMTRAY_H
