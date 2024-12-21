#ifndef WIN_SWITCHER_QTWIN_H
#define WIN_SWITCHER_QTWIN_H

#include <QWidget>

/// WinExtra module has been removed from Qt6, QAQ
namespace QtWin {
    void taskbarDeleteTab(QWidget* window);
    QPixmap fromHICON(HICON icon);
} // QtWin

#endif //WIN_SWITCHER_QTWIN_H
