#ifndef WIN_SWITCHER_UTIL_H
#define WIN_SWITCHER_UTIL_H

#include <Windows.h>
#include <QString>
#include <QIcon>
#include <dwmapi.h>

namespace Util {
    QString getWindowTitle(HWND hwnd);
    QString getClassName(HWND hwnd);
    QString getProcessExePath(HWND hwnd);
    QList<HWND> enumWindows();
    QList<HWND> enumChildWindows(HWND hwnd);
    QList<HWND> listValidWindows();
    HWND getUwpFrameWindow(HWND hwnd);
    QIcon getJumboIcon(const QString& filePath);
    bool setWindowRoundCorner(HWND hwnd, DWM_WINDOW_CORNER_PREFERENCE pvAttribute =  DWMWCP_ROUND);

    inline QString UwpCoreWindowClass = "Windows.UI.Core.CoreWindow";
    inline QString UwpAppFrameWindowClass = "ApplicationFrameWindow";
} // Util

#endif //WIN_SWITCHER_UTIL_H
