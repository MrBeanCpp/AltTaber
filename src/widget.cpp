#include "../header/widget.h"
#include "ui_Widget.h"
#include "utils/Util.h"
#include <QDebug>
#include <QFileInfo>
#include <QWindow>
#include <QKeyEvent>
#include <QFileIconProvider>
#include <QScreen>
#include "utils/setWindowBlur.h"
#include <QPainter>
#include <QPen>

Widget::Widget(QWidget *parent) :
        QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
    lw = ui->listWidget;
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);//设置窗口背景透明
    setWindowTitle("Windows Switcher");

    Util::setWindowRoundCorner(this->hWnd()); // 设置窗口圆角
    setWindowBlur(hWnd()); // 设置窗口模糊, 必须配合Qt::WA_TranslucentBackground

    lw->setViewMode(QListView::IconMode);
    lw->setMovement(QListView::Static);
    lw->setFlow(QListView::LeftToRight);
    lw->setWrapping(false);
    lw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lw->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lw->setIconSize({64, 64});
    lw->setGridSize({80, 80});

    connect(qApp, &QApplication::focusWindowChanged, [this](QWindow *focusWindow) {
        if (focusWindow == nullptr) {
            hide();
            qDebug() << "Focus window changed to nullptr";
        } else {
            qDebug() << "Focus window changed to" << focusWindow->title();
        }
    });
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

void Widget::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Alt) {
        hide();
    }
    QWidget::keyReleaseEvent(event);
}

void Widget::showEvent(QShowEvent *event) {
    if (!this->isMinimized()) {
        QFileIconProvider iconProvider;
        lw->clear();
        auto list = Util::listValidWindows();
        for (auto hwnd : list) {
            if (hwnd == this->hWnd()) continue; // skip self
            auto path = Util::getProcessExePath(hwnd);
            qDebug() << Util::getWindowTitle(hwnd) << Util::getClassName(hwnd) << path;
//            auto icon = iconProvider.icon(QFileInfo(path));
            auto icon = Util::getJumboIcon(path);
            auto pixmap = icon.pixmap(lw->iconSize());
            // ExtractIconEx 最大返回32x32
            // IShellItemImageFactory::GetImage 获取的图像有锯齿（64x64），而256x256倒是好一点，但是若exe没有这么大的图标，缩放后还是会很小（中心）
            // SHGetFileInfo 获取的图标最大只能32x32, 但是可以通过Index + SHGetImageList获取更大的图标(Jumbo)，这就是QFileIconProvider的实现
            // 没办法，对于不包含大图标的exe，周围会被填充透明，导致真实图标很小（例如，[Follower]获取64x64的图标，但只有左上角有8x8图标，其余透明）
            // 更诡异的是，48x48的Icon，Follower是可以正常获取的，比64x64的实际Icon尺寸还要大，倒行逆施
            // 但是我无法得知真实图标大小，无法进行缩放，只能作罢
            auto item = new QListWidgetItem(QIcon(pixmap), {}); // null != "", which will completely hide text area
//        item->setData(Qt::UserRole, QVariant::fromValue(hwnd));
            lw->addItem(item);
        }

        if (auto firstItem = lw->item(0)) {
            auto firstRect = lw->visualItemRect(firstItem);
            auto width = lw->gridSize().width() * lw->count() + (firstRect.x() - lw->frameWidth()); // 一些微小的噼里啪啦修正
            lw->setFixedWidth(width);
            lw->setFixedHeight(firstRect.height());

            // move to center
            auto screen = QApplication::primaryScreen(); // maybe nullptr
            auto lwRect = lw->rect();
            auto thisRect = lwRect.marginsAdded({20, 20, 20, 20});
            thisRect.moveCenter(screen->geometry().center());
            this->setGeometry(thisRect); // global pos
            lwRect.moveCenter(this->rect().center()); // local pos
            lw->setGeometry(lwRect);
        } else {
            // TODO no item, hide
        }
    }
}

void Widget::paintEvent(QPaintEvent*) { //不绘制会导致鼠标穿透背景
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);//取消边框//pen决定边框颜色
    painter.setBrush(QColor(10, 10, 10, 50));
    painter.drawRect(rect());
}
