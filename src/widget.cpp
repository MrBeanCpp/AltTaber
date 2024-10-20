#include "../header/widget.h"
#include "ui_Widget.h"
#include "utils/Util.h"
#include <QDebug>
#include <QFileInfo>

Widget::Widget(QWidget *parent) :
        QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);

    auto list = Util::listValidWindows();
    for (auto hwnd : list) {
        qDebug() << Util::getWindowTitle(hwnd) << Util::getClassName(hwnd) << Util::getProcessExePath(hwnd);
    }
}

Widget::~Widget() {
    delete ui;
}
