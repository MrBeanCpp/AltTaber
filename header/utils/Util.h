#ifndef WIN_SWITCHER_UTIL_H
#define WIN_SWITCHER_UTIL_H

#include <Windows.h>
#include <QString>

namespace Util {
    QString getWindowTitle(HWND hwnd);
    QString getClassName(HWND hwnd);
    QString getProcessExePath(HWND hwnd);
    QList<HWND> enumWindows();
    QList<HWND> enumChildWindows(HWND hwnd);
    QList<HWND> listValidWindows();
} // Util

#endif //WIN_SWITCHER_UTIL_H
