#ifndef WIN_SWITCHER_UTIL_H
#define WIN_SWITCHER_UTIL_H

#include <Windows.h>
#include <QString>
#include <dwmapi.h>

namespace Util {
    QString getWindowTitle(HWND hwnd);
    QString getClassName(HWND hwnd);
    QString getProcessExePath(HWND hwnd);
    QList<HWND> enumWindows();
    QList<HWND> enumChildWindows(HWND hwnd);
    QList<HWND> listValidWindows();
    bool setWindowRoundCorner(HWND hwnd, DWM_WINDOW_CORNER_PREFERENCE pvAttribute =  DWMWCP_ROUND);
} // Util

#endif //WIN_SWITCHER_UTIL_H
