#ifndef WIN_SWITCHER_WIDGET_H
#define WIN_SWITCHER_WIDGET_H

#include <QWidget>
#include <Windows.h>
#include <QListWidget>
#include <QDebug>

struct WindowGroup;
struct WindowInfo {
    QString title;
    QString className;
    HWND hwnd = nullptr;
};

// C++17 inline: 防止重定义
inline QDebug operator<<(QDebug dbg, const WindowInfo& info) {
    dbg.nospace() << "WindowInfo(" << info.title << ", " << info.className << ", " << info.hwnd << ")";
    return dbg.space();
}

Q_DECLARE_METATYPE(WindowInfo) // for QVariant
struct WindowGroup {
    WindowGroup() = default;

    void addWindow(const WindowInfo& window) {
        windows.append(window);
    }

    QString exePath;
    QIcon icon;
    QList<WindowInfo> windows;
};

Q_DECLARE_METATYPE(WindowGroup) // for QVariant

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget {
Q_OBJECT
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

public:
    explicit Widget(QWidget* cur = nullptr);
    QList<WindowGroup> prepareWindowGroupList();
    bool prepareListWidget();
    Q_INVOKABLE bool requestShow();
    void notifyForegroundChanged(HWND hwnd);

    HWND hWnd() { return (HWND) winId(); }

    bool isForeground() { return GetForegroundWindow() == hWnd(); }

    ~Widget() override;

private:
    bool forceShow();
    void showLabelForItem(QListWidgetItem* item);
    auto getLastActiveGroupWindow(const QString& exePath) -> QPair<HWND, QDateTime>;
    QList<HWND> buildGroupWindowOrder(const QString& exePath);
    static HWND rotateWindowInGroup(const QList<HWND>& windows, HWND current, bool forward = true);

private:
    Ui::Widget* ui;
    QListWidget* lw = nullptr;
    const QMargins ListWidgetMargin{24, 24, 24, 24};
    /// exePath -> (HWND, time)
    QHash<QString, QList<QPair<HWND, QDateTime>>> winActiveOrder;
    QList<HWND> groupWindowOrder; // for Alt+`
};


#endif //WIN_SWITCHER_WIDGET_H
