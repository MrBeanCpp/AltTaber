#ifndef WIN_SWITCHER_UTIL_H
#define WIN_SWITCHER_UTIL_H

#include <Windows.h>
#include <QString>
#include <QIcon>
#include <dwmapi.h>
#include <QElapsedTimer>

namespace Util {
    QString getWindowTitle(HWND hwnd);
    QString getClassName(HWND hwnd);
    QString getProcessExePath(HWND hwnd);
    QString getFileDescription(const QString& path);
    bool isTopMost(HWND hwnd);
    void switchToWindow(HWND hwnd, bool force = false);
    void bringWindowToTop(HWND hwnd, HWND hWndInsertAfter = HWND_TOPMOST);
    bool isWindowAcceptable(HWND hwnd);
    QList<HWND> enumWindows();
    QList<HWND> enumChildWindows(HWND hwnd);
    QList<HWND> listValidWindows();
    QList<HWND> listValidWindows(const QString& exePath);
    QIcon getJumboIcon(const QString& filePath);
    QIcon getCachedIcon(const QString& path);
    QPixmap getWindowIcon(HWND hwnd);
    bool setWindowRoundCorner(HWND hwnd, DWM_WINDOW_CORNER_PREFERENCE pvAttribute = DWMWCP_ROUND);
    bool isKeyPressed(int vkey);
    QIcon overlayIcon(const QPixmap& icon, const QPixmap& overlay, const QRect& overlayRect);
    HWND topWindowFromPoint(const POINT& pos);
    HWND topWindowFromPoint(const QPoint& pos);
} // Util

#endif //WIN_SWITCHER_UTIL_H
