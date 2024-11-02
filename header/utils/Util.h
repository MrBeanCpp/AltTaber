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
    void switchToWindow(HWND hwnd, bool force = false);
    bool isWindowAcceptable(HWND hwnd);
    QList<HWND> enumWindows();
    QList<HWND> enumChildWindows(HWND hwnd);
    QList<HWND> listValidWindows();
    QList<HWND> listValidWindows(const QString& exePath);
    HWND getAppFrameWindow(HWND hwnd);
    HWND getAppCoreWindow(HWND hwnd);
    bool isAppFrameWindow(HWND hwnd);
    QIcon getJumboIcon(const QString& filePath);
    QIcon getAppIcon(const QString& path);
    QIcon getCachedIcon(const QString& path);
    QPixmap getWindowIcon(HWND hwnd);
    bool setWindowRoundCorner(HWND hwnd, DWM_WINDOW_CORNER_PREFERENCE pvAttribute = DWMWCP_ROUND);
    bool isKeyPressed(int vkey);
    QIcon overlayIcon(const QPixmap& icon, const QPixmap& overlay, const QRect& overlayRect);
    QRect getWindowRect(HWND hwnd);

    inline const QString AppCoreWindowClass = "Windows.UI.Core.CoreWindow";
    inline const QString AppFrameWindowClass = "ApplicationFrameWindow";
    inline const QString AppManifest = "AppxManifest.xml";
} // Util

#endif //WIN_SWITCHER_UTIL_H
