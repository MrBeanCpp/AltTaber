#include <QList>
#include "utils/Util.h"
#include <QDebug>
#include <psapi.h>
#include <QFileInfo>
#include <QDir>
#include <QtWin>
#include <commoncontrols.h>
#include <ShObjIdl_core.h>

namespace Util {
    QString getWindowTitle(HWND hwnd) {
        const int len = GetWindowTextLength(hwnd) + 1; // include '\0'
        if (len <= 1) return {};
        auto *title = new wchar_t[len];
        GetWindowText(hwnd, title, len);
        QString result = QString::fromWCharArray(title);
        delete[] title;
        return result;
    }

    QString getClassName(HWND hwnd) {
        const int MAX = 256;
        wchar_t className[MAX];
        GetClassName(hwnd, className, MAX);
        return QString::fromWCharArray(className);
    }

    /// 判断窗口是否被隐藏，与IsWindowVisible不同，两者需要同时判断（疑惑）
    bool isWindowCloaked(HWND hwnd) {
        bool isCloaked = false;
        auto rt = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
        return rt == S_OK && isCloaked;
    }

    QString getProcessExePath(HWND hwnd)
    {
        DWORD processId = 0;
        GetWindowThreadProcessId(hwnd, &processId);

        if (processId == 0)
            return {};

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        if (hProcess == nullptr)
            return {};

        TCHAR processName[MAX_PATH] = TEXT("<unknown>");
        // https://www.cnblogs.com/mooooonlight/p/14491399.html
        if (GetModuleFileNameEx(hProcess, nullptr, processName, MAX_PATH)) {
            CloseHandle(hProcess);
            return QString::fromWCharArray(processName);
        }

        CloseHandle(hProcess);
        return {};
    }

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
        static const QStringList BlackList_ClassName = {
            "Progman",
//            "Windows.UI.Core.CoreWindow", // UWP
            "CEF-OSC-WIDGET"
        };

        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

        if (IsWindowVisible(hwnd)
        && !isWindowCloaked(hwnd)
        && !GetWindow(hwnd, GW_OWNER) // OmApSvcBroker
        && (exStyle & WS_EX_TOOLWINDOW) == 0 // 非工具窗口，但其实有些工具窗口没有这个这个属性
//        && (exStyle & WS_EX_TOPMOST) == 0 // 非置顶窗口
        && !BlackList_ClassName.contains(getClassName(hwnd))
        && GetWindowTextLength(hwnd) > 0
        ) {
            auto *windowList = reinterpret_cast<QList<HWND> *>(lParam);
            windowList->append(hwnd);
        }
        return TRUE;
    }

    QList<HWND> enumWindows() {
        QList<HWND> list;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&list));
        return list;
    }

    BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM lParam) {
        auto *windowList = reinterpret_cast<QList<HWND> *>(lParam);
        windowList->append(hwnd);
        return TRUE;
    }

    QList<HWND> enumChildWindows(HWND hwnd) {
        QList<HWND> list;
        EnumChildWindows(hwnd, EnumChildWindowsProc, reinterpret_cast<LPARAM>(&list));
        return list;
    }

    QList<HWND> listValidWindows() {
        QList<HWND> list;
        static const QStringList BlackList_ExePath = {
                R"(C:\Windows\ImmersiveControlPanel\SystemSettings.exe)"
        };
        static const QStringList BlackList_App_FileName = { // WindowsApps Windows.UI.Core.CoreWindow
                "Nahimic3.exe",
                "HxOutlook.exe",
                "Video.UI.exe"
        };
        auto winList = Util::enumWindows();
        for (auto hwnd : winList) {
            // ref: https://blog.csdn.net/qq_59075481/article/details/139574981
            if (Util::getClassName(hwnd) == "ApplicationFrameWindow") { // UWP的父窗口
                const auto childList = Util::enumChildWindows(hwnd);
                hwnd = nullptr;
                for (HWND child : childList) {
                    // UWP的本体应该是`Windows.UI.Core.CoreWindow`，但是enumWindows有时候枚举不到
                    // 只能通过`ApplicationFrameWindow`曲线救国
                    // 但是有些情况下，UWP的本体又会从`ApplicationFrameWindow`中分离出来，不属于子窗口，两种情况都要处理
                    if (!Util::getWindowTitle(child).isEmpty()) {
                        hwnd = child;
                        break;
                    }
                }
                // 如果子窗口都是无标题，说明是奇怪窗口
            }

            auto className = Util::getClassName(hwnd);
            // 进一步对UWP进行过滤（根据路径）
            if (className == "Windows.UI.Core.CoreWindow") {
                auto path = Util::getProcessExePath(hwnd);
                QFileInfo fileInfo(path);
                if (BlackList_ExePath.contains(path)
                    || path.startsWith(R"(C:\Windows\SystemApps\)")
                    || BlackList_App_FileName.contains(fileInfo.fileName())) {
                    continue;
                }
            }

            if (hwnd) {
                list << hwnd;
            }
        }
        return list;
    }

    /// Get 256x256 icon
    // 不用Index，直接用SHGetFileInfo获取图标的话，最大只能32x32
    // 对于QFileIconProvider的优势是可以多线程
    // ref: https://github.com/stianhoiland/cmdtab/blob/746c41226cdd820c26eadf00eb86b45896dc1dcd/src/cmdtab.c#L333
    // ref: https://blog.csdn.net/ssss_sj/article/details/9786403
    QIcon getJumboIcon(const QString& filePath) {
        SHFILEINFOW sfi = {nullptr};
        // Get the icon index using SHGetFileInfo
        SHGetFileInfo(filePath.toStdWString().c_str(), 0, &sfi, sizeof(SHFILEINFOW), SHGFI_SYSICONINDEX);

        // 48x48 icons, use SHIL_EXTRALARGE
        // 256x256 icons (after Vista), use SHIL_JUMBO
        IImageList* imageList;
        HRESULT hResult = SHGetImageList(SHIL_JUMBO, IID_IImageList, (void**)&imageList);

        QIcon icon;
        if (hResult == S_OK) {
            HICON hIcon;
            hResult = imageList->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hIcon);

            if (hResult == S_OK) {
                icon = QtWin::fromHICON(hIcon);
                DestroyIcon(sfi.hIcon);
            }
        }
        imageList->Release();
        return icon;
    }

    /// 设置窗口圆角 原来这么方便嘛！ 为什么Qt搜不到！
    // https://github.com/stianhoiland/cmdtab/blob/746c41226cdd820c26eadf00eb86b45896dc1dcd/src/cmdtab.c#L1275
    // https://github.com/DinoChan/WindowChromeApplyRoundedCorners
    bool setWindowRoundCorner(HWND hwnd, DWM_WINDOW_CORNER_PREFERENCE pvAttribute) {
        HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pvAttribute, sizeof(pvAttribute));
        if (FAILED(hr)) {
            qWarning() << "Failed to set rounded corners for window:" << hr;
            return false;
        }
        return true;
    }
} // Util