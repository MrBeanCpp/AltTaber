#include "../header/widget.h"
#include "ui_Widget.h"
#include "utils/Util.h"
#include <QDebug>
#include <QWindow>
#include <QKeyEvent>
#include <QScreen>
#include "utils/setWindowBlur.h"
#include "utils/IconOnlyDelegate.h"
#include <QPainter>
#include <QPen>
#include <QDateTime>
#include <QtWin>

Widget::Widget(QWidget* parent) :
        QWidget(parent), ui(new Ui::Widget) {
    ui->setupUi(this);
    lw = ui->listWidget;
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground); //设置窗口背景透明 !但是会造成show()时的闪烁 和 绘制延迟(?)
    QtWin::taskbarDeleteTab(this); //删除任务栏图标
    setWindowTitle("AltTaber");

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
    lw->setFixedHeight(lw->gridSize().height());
    lw->setUniformItemSizes(true); // optimization ?
    lw->setStyleSheet(R"(
        QListWidget {
            background-color: transparent;
            border: none;
            outline: none; /* 去除选中时的虚线框（在文字为空时，会形成闪电一样的标志 离谱） */
        }
    )");
    // 就算Text为Null，也会占用空间，很难做到真正的IConMode，所以只能delegate自绘
    // 本来为了去除图标选中变色样式，可以对Icon手动addPixmap(..., QIcon::Selected) or (& ~Qt::ItemIsSelectable)
    // 但是采用delegate后，就没必要了
    // will not take ownership of delegate
    lw->setItemDelegate(new IconOnlyDelegate(lw));
    lw->installEventFilter(this);

    connect(lw, &QListWidget::currentItemChanged, [this](QListWidgetItem* cur, QListWidgetItem*) {
        if (cur) showLabelForItem(cur);
    });

    connect(qApp, &QApplication::focusWindowChanged, [this](QWindow* focusWindow) {
        if (focusWindow == nullptr) {
            if (!this->underMouse()) // hide when lost focus & mouse outside (means user choose to)
                hide();
            else { // Windows Terminal will do
                qWarning() << "Someone tried to steal focus!";
            }
        }
    });
}

Widget::~Widget() {
    delete ui;
}

void Widget::keyPressEvent(QKeyEvent* event) {
    auto key = event->key();
    auto modifiers = event->modifiers();
    if (key == Qt::Key_Tab) { // switch to next or prev
        auto i = lw->currentRow();
        bool isShiftPressed = (modifiers & Qt::ShiftModifier);
        // weird formula, but works (hhh)
        auto index = (i - (2 * isShiftPressed - 1) + lw->count()) % lw->count();
        lw->setCurrentRow(index);
    } else if (key == Qt::Key_QuoteLeft && (modifiers & Qt::AltModifier)) {
        if (this->isForeground()) return;
        auto foreWin = GetForegroundWindow();
        if (groupWindowOrder.isEmpty()) {
            auto targetExe = Util::getProcessExePath(foreWin);
            groupWindowOrder = buildGroupWindowOrder(targetExe);
        }
        if (auto nextWin = rotateWindowInGroup(groupWindowOrder, foreWin, !(modifiers & Qt::ShiftModifier))) {
            Util::switchToWindow(nextWin, true);
            qInfo() << "Switch to" << Util::getWindowTitle(nextWin) << Util::getClassName(nextWin);
        }
    }
    QWidget::keyPressEvent(event);
}

bool Widget::forceShow() { // TODO 显示有闪烁 因为Qt::WA_TranslucentBackground，也许可以增加透明度动画，减弱影响
    showMinimized();
    showNormal();
    return isForeground();
}

/// show App description under the icon
void Widget::showLabelForItem(QListWidgetItem* item, QString text) {
    if (!item) return;

    if (text.isNull()) {
        auto path = item->data(Qt::UserRole).value<WindowGroup>().exePath;
        text = Util::getFileDescription(path);
    }
    ui->label->setText(text);
    ui->label->adjustSize();

    auto itemRect = lw->visualItemRect(item);
    auto center = itemRect.center() + QPoint(0, itemRect.height() / 2 + ListWidgetMargin.bottom() / 2);
    center = lw->mapTo(this, center);
    auto labelRect = ui->label->rect();
    labelRect.moveCenter(center);

    auto bound = this->rect().marginsRemoved({5, 0, 5, 0});
    labelRect.moveLeft(qMax(labelRect.left(), bound.left()));
    labelRect.moveRight(qMin(labelRect.right(), bound.right()));

    ui->label->move(labelRect.topLeft());
}

void Widget::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Alt) {
        groupWindowOrder.clear(); // for Alt + `
        if (this->isVisible()) {
            // active selected window
            if (auto item = lw->currentItem()) {
                if (auto group = item->data(Qt::UserRole).value<WindowGroup>(); !group.windows.empty()) {
                    WindowInfo targetWin = group.windows.at(0); // TODO 需要排序（lastActive不可用情况下）
                    const auto hwndOrder = winActiveOrder.value(group.exePath);
                    const auto lastActive = hwndOrder.isEmpty() ? nullptr : hwndOrder.last().first;
                    for (auto& info: group.windows) {
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
            hide(); //! must hide after active target window, or focus may fallback to prev foreground window (like 网易云音乐)
        }
    }
    QWidget::keyReleaseEvent(event);
}

void Widget::paintEvent(QPaintEvent*) { //不绘制会导致鼠标穿透背景
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);//取消边框//pen决定边框颜色
    painter.setBrush(QColor(25, 25, 25, 100));
    painter.drawRect(rect());
}

/// 通知前台窗口变化
void Widget::notifyForegroundChanged(HWND hwnd) { // TODO isVisible or AltDown时，关闭前台更新通知
    if (hwnd == this->hWnd()) return;
    if (!Util::isWindowAcceptable(hwnd)) return;
    auto path = Util::getProcessExePath(hwnd); // TODO 比较耗时，最好仅在单次show期间缓存，同时避免hwnd复用造成缓存错误
    // TODO 不能让winActiveOrder无限增长，需要定时清理
    winActiveOrder[path] << qMakePair(hwnd, QDateTime::currentDateTime()); // TODO QList改成QHash！，自动去重！
    qDebug() << "Focus changed:" << Util::getWindowTitle(hwnd) << Util::getClassName(hwnd) << path << Util::getFileDescription(path);
} // TODO 控制面板 和 资源管理器 exe是同一个，如何区分图标

/// collect, filter, sort Windows for presentation
QList<WindowGroup> Widget::prepareWindowGroupList() {
    QMap<QString, WindowGroup> winGroupMap;
    auto list = Util::listValidWindows();
    for (auto hwnd: list) {
        if (hwnd == this->hWnd()) continue; // skip self
        auto path = Util::getProcessExePath(hwnd);
        if (path.isEmpty()) continue; // TODO 可能需要管理员权限
        auto& winGroup = winGroupMap[path];
        if (winGroup.exePath.isEmpty()) { // QIcon::isNull 判断可能不太准（例如空图标）
            winGroup.exePath = path;
            auto icon = Util::getCachedIcon(path); // TODO background thread
            if (path.endsWith("QQ\\bin\\QQ.exe", Qt::CaseInsensitive)) { // draw chat partner for classical QQ
                QPixmap overlay = Util::getWindowIcon(hwnd);
                const auto iSize = lw->iconSize();
                QPixmap bgPixmap = icon.pixmap(iSize);
                icon = Util::overlayIcon(bgPixmap, overlay, {{iSize.width() / 2, iSize.height() / 2}, iSize / 2});
            }
            winGroup.icon = icon;
        }
        winGroup.addWindow({Util::getWindowTitle(hwnd), Util::getClassName(hwnd), hwnd});
    }
    auto winGroupList = winGroupMap.values();
    // 按照活跃度排序
    std::sort(winGroupList.begin(), winGroupList.end(), [this](const WindowGroup& a, const WindowGroup& b) {
        auto timeA = getLastActiveGroupWindow(a.exePath).second;
        auto timeB = getLastActiveGroupWindow(b.exePath).second;
        if (timeA.isNull() && timeB.isNull()) return false;
        if (timeA.isValid() && timeB.isValid()) return timeA > timeB;
        return timeA.isValid();
    });
    return winGroupList;
}

bool Widget::prepareListWidget() {
    auto winGroupList = prepareWindowGroupList();
    lw->clear();
    for (auto& winGroup: winGroupList) {
        auto item = new QListWidgetItem(winGroup.icon, {}); // null != "", which will completely hide text area
        item->setData(Qt::UserRole, QVariant::fromValue(winGroup));
        item->setSizeHint(lw->gridSize()); // 决定了delegate的绘制区域，比grid小的话，paintRect就不居中了，而且update也不及时
//        item->setFlags(item->flags() & ~Qt::ItemIsSelectable); // 不可选中
        lw->addItem(item);
    }

    // calculate Geometry
    if (auto firstItem = lw->item(0)) {
        auto firstRect = lw->visualItemRect(firstItem);
        auto width = lw->gridSize().width() * lw->count() + (firstRect.x() - lw->frameWidth()); // 一些微小的噼里啪啦修正
        lw->setFixedWidth(width);

        // move to center
        auto screen = QApplication::primaryScreen(); // TODO maybe nullptr 处理多屏幕 & DPI
        auto lwRect = lw->rect();
        auto thisRect = lwRect.marginsAdded(ListWidgetMargin);
        thisRect.moveCenter(screen->geometry().center());
        this->setGeometry(thisRect); // global pos
        lwRect.moveCenter(this->rect().center()); // local pos
        lw->setGeometry(lwRect);
    } else {
        // no item, hide ? TODO
        return false;
    }

    // set current item
    if (lw->count() >= 2) {
        auto foreWin = GetForegroundWindow();
        bool isFirstItemForeground = false;
        for (auto& info: winGroupList.at(0).windows) {
            if (info.hwnd == foreWin) {
                isFirstItemForeground = true;
                break;
            }
        }
        // 如果第一个item是前台窗口，就选中第二个
        // 因为有些情况：选中桌面 并不会产生一个item
        lw->setCurrentRow(isFirstItemForeground ? 1 : 0); //! 首次显示时，该行特别耗时：472ms
    } else if (lw->count() == 1) {
        lw->setCurrentRow(0);
    }

    return true;
}

bool Widget::requestShow() { // TODO 当前台是开始菜单（Win）时，会导致显示 但无法操控
    return prepareListWidget() && forceShow();
}

auto Widget::getLastActiveGroupWindow(const QString& exePath) -> QPair<HWND, QDateTime> {
    auto hwndOrder = winActiveOrder.value(exePath);
    if (hwndOrder.isEmpty()) return {nullptr, QDateTime()};
    return hwndOrder.last();
}

/// group by exePath, sort by active order (last active first)
QList<HWND> Widget::buildGroupWindowOrder(const QString& exePath) {
    auto windows = Util::listValidWindows(exePath); // filter by path
    auto activeOrder = winActiveOrder.value(exePath);
    QHash<HWND, QDateTime> activeOrdMap(activeOrder.begin(), activeOrder.end());
    // sort by active order
    std::sort(windows.begin(), windows.end(), [&activeOrdMap](HWND a, HWND b) {
        return activeOrdMap.value(a) > activeOrdMap.value(b);
    }); // TODO update winActiveOrder!
    return windows;
}

bool Widget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == lw && event->type() == QEvent::Wheel) {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);
        auto cursorPos = wheelEvent->position().toPoint();
        if (auto item = lw->itemAt(cursorPos)) {
            if (lw->currentItem() != item)
                lw->setCurrentItem(item);
            auto windowGroup = item->data(Qt::UserRole).value<WindowGroup>();
            if (windowGroup.windows.size() <= 1) return false;

            static QListWidgetItem* lastItem = nullptr;
            static HWND hwnd = nullptr;
            if (lastItem != item) { // Alt+Tab也可能造成切换
                lastItem = item;
                hwnd = nullptr;
                groupWindowOrder.clear();
            }
            auto targetExe = windowGroup.exePath;
            static bool isLastRollUp = true;
            bool isRollUp = wheelEvent->angleDelta().x() > 0; // ListWidget的方向改成了从左到右，所以滚轮方向从y()变成x()了
            if (groupWindowOrder.isEmpty())
                groupWindowOrder = buildGroupWindowOrder(targetExe);

            if (!hwnd) { // first time
                if (!(hwnd = getLastActiveGroupWindow(targetExe).first))
                    hwnd = groupWindowOrder.first(); // 没有lastActive记录，就随便选一个
            } else { // select next window
                if (isLastRollUp == isRollUp) // 滚轮方向切换时，不轮换窗口
                    hwnd = rotateWindowInGroup(groupWindowOrder, hwnd, isRollUp);
            }
            isLastRollUp = isRollUp;

            HWND nextFocus = hwnd; // this隐藏后的焦点备选窗口, for `swtichToWindow` after AltUp
            if (isRollUp) {
                Util::bringWindowToTop(hwnd); // wihout activate
            } else {
                if (auto normal = rotateNormalWindowInGroup(groupWindowOrder, hwnd, false)) { // skip minimized
                    ShowWindow(normal, SW_SHOWMINNOACTIVE); // minimize
                    hwnd = normal;
                    nextFocus = hwnd;
                }
                if (auto normal = rotateNormalWindowInGroup(groupWindowOrder, hwnd, false))
                    nextFocus = normal; // 备选焦点切换为下一个非最小化窗口 after AltUp
            }
            notifyForegroundChanged(nextFocus);
            showLabelForItem(item, Util::getWindowTitle(nextFocus));

            return true; // stop propagation
        }
    }
    return false;
}

/// select next(forward)(older) or prev window in group<br>
/// Do nothing, but select HWND
HWND Widget::rotateWindowInGroup(const QList<HWND>& windows, HWND current, bool forward) {
    const auto N = windows.size();
    for (int i = 0; i < N && N > 1; i++) {
        if (windows.at(i) == current) {
            auto next_i = forward ? (i + 1) : (i - 1);
            auto next = windows.at((next_i + N) % N);
            return next;
        }
    }
    return nullptr;
}

/// Select next (including `current`) normal (!minimized) window in group<br>
/// return nullptr if all minimized
HWND Widget::rotateNormalWindowInGroup(const QList<HWND>& windows, HWND current, bool forward) {
    for (int i = 0; IsIconic(current) && i < windows.size(); i++) // skip minimized
        current = rotateWindowInGroup(windows, current, forward);
    return IsIconic(current) ? nullptr : current;
}
