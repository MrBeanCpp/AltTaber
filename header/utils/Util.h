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
    void switchToWindow(HWND hwnd);
    bool isWindowAcceptable(HWND hwnd);
    QList<HWND> enumWindows();
    QList<HWND> enumChildWindows(HWND hwnd);
    QList<HWND> listValidWindows();
    HWND getAppFrameWindow(HWND hwnd);
    QIcon getJumboIcon(const QString& filePath);
    QIcon getAppIcon(const QString& path);
    QIcon getCachedIcon(const QString& path);
    QPixmap getWindowIcon(HWND hwnd);
    bool setWindowRoundCorner(HWND hwnd, DWM_WINDOW_CORNER_PREFERENCE pvAttribute = DWMWCP_ROUND);
    bool isKeyPressed(int vkey);
    QIcon overlayIcon(const QPixmap& icon, const QPixmap& overlay, const QRect& overlayRect);

    inline const QString AppCoreWindowClass = "Windows.UI.Core.CoreWindow";
    inline const QString AppFrameWindowClass = "ApplicationFrameWindow";
    inline const QString AppManifest = "AppxManifest.xml";
} // Util

#endif //WIN_SWITCHER_UTIL_H
