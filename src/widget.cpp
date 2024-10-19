
// You may need to build the project (run Qt uic code generator) to get "ui_Widget.h" resolved

#include "../header/widget.h"
#include "ui_Widget.h"


Widget::Widget(QWidget *parent) :
        QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
}

Widget::~Widget() {
    delete ui;
}
