#include "../header/widget.h"
#include "ui_Widget.h"
#include "utils/Util.h"
#include <QDebug>
#include <QWindow>
#include <QKeyEvent>
#include <QScreen>
#include "utils/setWindowBlur.h"
#include <QPainter>
#include <QPen>
#include <QDateTime>
#include <QtWin>

Widget::Widget(QWidget *parent) :
        QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
    lw = ui->listWidget;
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);//设置窗口背景透明
    QtWin::taskbarDeleteTab(this); //删除任务栏图标
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
        if (focusWindow == nullptr) // hide when lost focus
            hide();
    });
}

Widget::~Widget() {
    delete ui;
}

void Widget::keyPressEvent(QKeyEvent *event) {
    auto key = event->key();
    if (key == Qt::Key_Tab) { // switch to next
        auto i = lw->currentRow();
        lw->setCurrentRow((i + 1) % lw->count());
    }
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
        if (auto item = lw->currentItem()) {
            if (auto group = item->data(Qt::UserRole).value<WindowGroup>(); !group.windows.empty()) {
                WindowInfo targetWin = group.windows.at(0); // TODO 需要排序（lastActive不可用情况下）
                const auto lastActive = winActiveOrder.value(group.exePath, {nullptr, QDateTime()}).first;
                for (auto& info : group.windows) {
                    if (info.hwnd == lastActive) {
                        targetWin = info;
                        break;
                    }
                }
                if (targetWin.hwnd) {
                    Util::switchToWindow(targetWin.hwnd);
                    qInfo() << "Switch to" << targetWin << group.exePath;
                }
            }
        }
    }
    QWidget::keyReleaseEvent(event);
}

void Widget::paintEvent(QPaintEvent*) { //不绘制会导致鼠标穿透背景
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);//取消边框//pen决定边框颜色
    painter.setBrush(QColor(10, 10, 10, 50));
    painter.drawRect(rect());
}

/// 通知前台窗口变化
void Widget::notifyForegroundChanged(HWND hwnd) { // TODO 处理UWP hwnd不对应的问题！
    if (hwnd == this->hWnd()) return;
    auto path = Util::getProcessExePath(hwnd);
    // TODO 不能让winActiveOrder无限增长，需要定时清理
    winActiveOrder[path] = {hwnd, QDateTime::currentDateTime()}; // TODO 需要记录同组窗口之间的顺序
    qDebug() << "Foreground changed" << Util::getWindowTitle(hwnd) << Util::getClassName(hwnd);
}

bool Widget::requestShow() {
    QMap<QString, WindowGroup> winGroupMap;
    auto list = Util::listValidWindows();
    for (auto hwnd : list) {
        if (hwnd == this->hWnd()) continue; // skip self
        auto path = Util::getProcessExePath(hwnd);
        if (path.isEmpty()) continue; // TODO 可能需要管理员权限
        auto& winGroup = winGroupMap[path];
        if (winGroup.exePath.isEmpty()) { // QIcon::isNull 判断可能不太准（例如空图标）
            winGroup.exePath = path;
            winGroup.icon = Util::getCachedIcon(path); // TODO background thread
        }
        winGroup.addWindow({Util::getWindowTitle(hwnd), Util::getClassName(hwnd), hwnd});
    }
    auto winGroupList = winGroupMap.values();
    // 按照活跃度排序
    std::sort(winGroupList.begin(), winGroupList.end(), [this](const WindowGroup &a, const WindowGroup &b) {
        auto timeA = winActiveOrder.value(a.exePath, {nullptr, QDateTime()}).second;
        auto timeB = winActiveOrder.value(b.exePath, {nullptr, QDateTime()}).second;
        if (timeA.isNull() && timeB.isNull()) return false;
        if (timeA.isValid() && timeB.isValid()) return timeA > timeB;
        return timeA.isValid();
    });

    lw->clear();
    for (auto& winGroup : winGroupList) {
        auto item = new QListWidgetItem(winGroup.icon, {}); // null != "", which will completely hide text area
        item->setData(Qt::UserRole, QVariant::fromValue(winGroup));
        lw->addItem(item);
    }
    if (lw->count() >= 2) {
        auto foreWinPath = Util::getProcessExePath(GetForegroundWindow());
        // 如果第一个item是前台窗口，就选中第二个
        // 因为有些情况：选中桌面 并不会产生一个item
        lw->setCurrentRow(foreWinPath == winGroupList.at(0).exePath ? 1 : 0);
    } else if (lw->count() == 1) {
        lw->setCurrentRow(0);
    }

    if (auto firstItem = lw->item(0)) {
        auto firstRect = lw->visualItemRect(firstItem);
        auto width = lw->gridSize().width() * lw->count() + (firstRect.x() - lw->frameWidth()); // 一些微小的噼里啪啦修正
        lw->setFixedWidth(width);
        lw->setFixedHeight(firstRect.height());

        // move to center
        auto screen = QApplication::primaryScreen(); // TODO maybe nullptr 处理多屏幕 & DPI
        auto lwRect = lw->rect();
        auto thisRect = lwRect.marginsAdded({20, 20, 20, 20});
        thisRect.moveCenter(screen->geometry().center());
        this->setGeometry(thisRect); // global pos
        lwRect.moveCenter(this->rect().center()); // local pos
        lw->setGeometry(lwRect);
    } else {
        // no item, hide ? TODO
        return false;
    }

    return forceShow();
}
