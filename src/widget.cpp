#include "../header/widget.h"
#include "ui_Widget.h"
#include "utils/Util.h"
#include <QDebug>
#include <QFileInfo>
#include <QWindow>
#include <QKeyEvent>
#include "utils/setWindowBlur.h"
#include <QPainter>
#include <QPen>

Widget::Widget(QWidget *parent) :
        QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);//设置窗口背景透明
    setWindowTitle("Windows Switcher");
    setWindowBlur(hWnd()); // 设置窗口模糊, 必须配合Qt::WA_TranslucentBackground

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
void Widget::paintEvent(QPaintEvent* event)//不绘制会导致鼠标穿透背景
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);//取消边框//pen决定边框颜色
    painter.setBrush(QColor(10, 10, 10, 50));
    painter.drawRect(rect());
}
