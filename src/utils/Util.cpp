#include <QList>
#include "utils/Util.h"
#include <QDebug>
#include <psapi.h>
#include <QFileInfo>
#include <QDir>
#include <QtWin>
#include <commoncontrols.h>
#include <ShObjIdl_core.h>
#include <QDomDocument>

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

    QString getClassName(HWND hwnd) { // 性能还可以，0.02ms
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

    QString getProcessExePath(HWND hwnd) {
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

    /// 将焦点从[自身]切换至[指定窗口]
    /// <br> 注意：如果自身没有焦点，可能失败
    void switchToWindow(HWND hwnd) {
        auto className = getClassName(hwnd);
        if (className == AppCoreWindowClass) { // UWP 只能对 frame 窗口操作
            if (auto frame = getAppFrameWindow(hwnd))
                hwnd = frame;
        }
        if (IsIconic(hwnd))
            ShowWindow(hwnd, SW_RESTORE);
        // 本窗口是前台窗口，因此可以随意调用该函数转移焦点
        SetForegroundWindow(hwnd);
    }

    /// filter HWND by some rules
    bool isWindowAcceptable(HWND hwnd) {
        static const QStringList BlackList_ClassName = {
                "Progman",
//            "Windows.UI.Core.CoreWindow", // UWP
                "CEF-OSC-WIDGET",
                "WorkerW", // explorer.exe
                "Shell_TrayWnd" // explorer.exe
        };
        static const QStringList BlackList_FileName = { // TODO by user from config
                "Follower.exe",
                "QQ Follower.exe"
        };
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

        if (IsWindowVisible(hwnd)
            && !isWindowCloaked(hwnd)
            && !GetWindow(hwnd, GW_OWNER) // OmApSvcBroker
            && (exStyle & WS_EX_TOOLWINDOW) == 0 // 非工具窗口，但其实有些工具窗口没有这个这个属性
//            && (exStyle & WS_EX_TOPMOST) == 0 // 非置顶窗口
            && GetWindowTextLength(hwnd) > 0
            && !BlackList_ClassName.contains(getClassName(hwnd))
            && !BlackList_FileName.contains(QFileInfo(getProcessExePath(hwnd)).fileName())
            ) {
            return true;
        }
        return false;
    }

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
        if (isWindowAcceptable(hwnd)) {
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
        static const QStringList BlackList_App_ExePath = {
                R"(C:\Windows\ImmersiveControlPanel\SystemSettings.exe)"
        };
        static const QStringList BlackList_App_FileName = { // WindowsApps Windows.UI.Core.CoreWindow
                "Nahimic3.exe",
                "HxOutlook.exe",
                "Video.UI.exe"
        };
        auto winList = Util::enumWindows();
        for (auto hwnd : winList) {
            auto className = Util::getClassName(hwnd);
            // ref: https://blog.csdn.net/qq_59075481/article/details/139574981
            if (className == AppFrameWindowClass) { // UWP的父窗口
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

            auto path = Util::getProcessExePath(hwnd);
            auto fileName = QFileInfo(path).fileName();
            // 进一步对UWP进行过滤（根据路径）
            if (className == AppCoreWindowClass) {
                if (BlackList_App_ExePath.contains(path)
                    || path.startsWith(R"(C:\Windows\SystemApps\)")
                    || BlackList_App_FileName.contains(fileName)) {
                    continue;
                }
            }

            if (hwnd) {
                list << hwnd;
            }
        }
        return list;
    }

    /// 对Core子窗口进行hwnd之类的操作之后，就会脱离原本的ApplicationFrameWindow，所以很难通过关联性去查找了<br>
    /// 这种情况下，通过标题和类名比较好<br>
    /// 这里需要查找Frame窗口的原因是，restore等操作只能对其生效！
    HWND getAppFrameWindow(HWND hwnd) {
        auto className = Util::getClassName(hwnd);
        auto title = Util::getWindowTitle(hwnd);
        if (className == AppCoreWindowClass) {
            if (auto res = FindWindowW(LPCWSTR(AppFrameWindowClass.utf16()), LPCWSTR(title.utf16())))
                return res;
            qWarning() << "Failed to find ApplicationFrameWindow of " << title << hwnd;
            return nullptr;
        } else
            return hwnd;
    }

    /// Get 256x256 icon
    // 不用Index，直接用SHGetFileInfo获取图标的话，最大只能32x32
    // 对于QFileIconProvider的优势是可以多线程
    // ref: https://github.com/stianhoiland/cmdtab/blob/746c41226cdd820c26eadf00eb86b45896dc1dcd/src/cmdtab.c#L333
    // ref: https://blog.csdn.net/ssss_sj/article/details/9786403
    // ExtractIconEx 最大返回32x32
    // IShellItemImageFactory::GetImage 获取的图像有锯齿（64x64），而256x256倒是好一点，但是若exe没有这么大的图标，缩放后还是会很小（中心）
    // SHGetFileInfo 获取的图标最大只能32x32, 但是可以通过Index + SHGetImageList获取更大的图标(Jumbo)，这就是QFileIconProvider的实现
    // 没办法，对于不包含大图标的exe，周围会被填充透明，导致真实图标很小（例如，[Follower]获取64x64的图标，但只有左上角有8x8图标，其余透明）
    // 更诡异的是，48x48的Icon，Follower是可以正常获取的，比64x64的实际Icon尺寸还要大，倒行逆施
    // 但是我无法得知真实图标大小，无法进行缩放，只能作罢
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

    /// 普通API很难获取UWP的图标，遂手动解析AppxManifest.xml
    /// <br> relativePath: e.g. Assets\\StoreLogo.png
    QString getLogoPathFromAppxManifest(const QString& manifestPath)
    {
        QFile file(manifestPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "无法打开AppxManifest.xml文件" << manifestPath;
            return {};
        }

        QDomDocument doc;
        if (!doc.setContent(&file)) {
            file.close();
            qDebug() << "无法解析XML文件";
            return {};
        }
        file.close();

        static const QStringList LogoAttributes = {
//                "Square150x150Logo", // 周围会被填充大量透明，导致图标很小
                "Square44x44Logo",
                "Square30x30Logo"
                "SmallLogo"
        };

        QDomElement root = doc.documentElement();
        QDomElement appElement = root.firstChildElement("Applications").firstChildElement("Application");
        QDomElement visualElement = appElement.firstChildElement("uap:VisualElements");
        for (const auto& attr : LogoAttributes) {
            if (auto value = visualElement.attribute(attr); !value.isEmpty())
                return value;
        }

        QDomElement storeLogo = root.firstChildElement("Properties").firstChildElement("Logo");
        if (!storeLogo.isNull())
            return storeLogo.text();

        return {};
    }

    /// 匹配某个变体，如：StoreLogo.scale-200.png
    QIcon loadUWPLogo(const QString& logoPath)
    {
        QFileInfo fileInfo(logoPath);
        QDir dir = fileInfo.absoluteDir();
        QString wildcard = fileInfo.baseName() + "*." + fileInfo.suffix();

        if (!dir.exists()) {
            qDebug() << "Directory does not exist!";
            return {};
        }

        QStringList filters;
        filters << wildcard; // 例如 "StoreLogo*.png"

        QStringList matchingFiles = dir.entryList(filters, QDir::Files, QDir::Size); // sort by size

        // `remove_if` 不会真正删除，只是移动到后面，返回新的end迭代器
        auto _ = std::remove_if(matchingFiles.begin(), matchingFiles.end(), [](const QString& fileName) {
            return fileName.contains("_contrast-black.") || fileName.contains("_contrast-white.");
        });

        if (!matchingFiles.isEmpty()) {
            QString logoFile = dir.absoluteFilePath(matchingFiles.first());
            return QIcon(logoFile);
        } else {
            qWarning() << "No matching files found!";
        }
        return {};
    }

    QIcon getAppIcon(const QString& path) {
        QFileInfo fileInfo(path);
        const auto dir = fileInfo.absolutePath();
        const auto manifestPath = dir + "\\AppxManifest.xml";
        const auto logoPath = dir + "\\" + getLogoPathFromAppxManifest(manifestPath);
        return loadUWPLogo(logoPath);
    }

    bool isUsingDefaultIcon(const QString& exePath) {
        UINT iconCount = ExtractIconEx(exePath.toStdWString().c_str(), -1, nullptr, nullptr, 0);
        return iconCount == 0;
    }

    /// Cached Icon, including UWP
    QIcon getCachedIcon(const QString& path) {
        static QHash<QString, QIcon> IconCache;
        if (auto icon = IconCache.value(path); !icon.isNull())
            return icon;

        qDebug() << "Icon not found in cache, loading..." << path;
        QIcon icon;
        if (isUsingDefaultIcon(path)) {
            qDebug() << "Default Icon, maybe UWP" << path;
            icon = getAppIcon(path);
        } else
            icon = getJumboIcon(path);
        IconCache.insert(path, icon);
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

    /// Convenience function of `GetAsyncKeyState`
    bool isKeyPressed(int vkey) {
        return GetAsyncKeyState(vkey) & 0x8000;
    }
} // Util