#include "../header/widget.h"
#include "ui_Widget.h"
#include "utils/Util.h"
#include <QDebug>
#include <QFileInfo>
#include <QWindow>
#include <QKeyEvent>

Widget::Widget(QWidget *parent) :
        QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::FramelessWindowHint);
    setWindowTitle("Windows Switcher");

    connect(qApp, &QApplication::focusWindowChanged, [this](QWindow *focusWindow) {
        if (focusWindow == nullptr) {
            hide();
            qDebug() << "Focus window changed to nullptr";
        } else {
            qDebug() << "Focus window changed to" << focusWindow->title();
        }
    });

    auto list = Util::listValidWindows();
    for (auto hwnd : list) {
        qDebug() << Util::getWindowTitle(hwnd) << Util::getClassName(hwnd) << Util::getProcessExePath(hwnd);
    }
}

Widget::~Widget() {
    delete ui;
}

void Widget::keyPressEvent(QKeyEvent *event) {
    qDebug() << "Key pressed:" << event->key();
    QWidget::keyPressEvent(event);
}

bool Widget::forceShow() {
    showMinimized();
    showNormal();
    return GetForegroundWindow() == (HWND)winId();
}
