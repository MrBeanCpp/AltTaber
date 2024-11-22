#include "utils/AppUtil.h"
#include "utils/Util.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <appmodel.h>
#include <KnownFolders.h>
#include <ShlGuid.h>
#include <commoncontrols.h>
#include <ShObjIdl_core.h>
#include <QDomDocument>
#include <QPainter>
#include <propkey.h>
#include <atlbase.h>
#include <QFileInfo>

namespace AppUtil {
    /// 对Core子窗口进行hwnd之类的操作之后，就会脱离原本的ApplicationFrameWindow，所以很难通过关联性去查找了<br>
    /// 这种情况下，通过标题和类名比较好<br>
    /// 这里需要查找Frame窗口的原因是，restore等操作只能对其生效！
    HWND getAppFrameWindow(HWND hwnd) {
        auto className = Util::getClassName(hwnd);
        if (className == AppFrameWindowClass) return hwnd;
        auto title = Util::getWindowTitle(hwnd);
        if (auto res = FindWindowW(LPCWSTR(AppFrameWindowClass.utf16()), LPCWSTR(title.utf16())))
            return res;
        qWarning() << "Failed to find ApplicationFrameWindow of " << title << hwnd;
        return nullptr;
    }

    HWND getAppCoreWindow(HWND hwnd) {
        auto className = Util::getClassName(hwnd);
        if (className == AppCoreWindowClass) return hwnd;
        const auto childList = Util::enumChildWindows(hwnd);
        const auto title = Util::getWindowTitle(hwnd);
        for (HWND child: childList) { // 优先查找子窗口，某些情况下会脱离父窗口（比如最小化！）
            if (Util::getClassName(child) == AppCoreWindowClass && Util::getWindowTitle(child) == title) {
                return child;
            }
        }

        // FindWindow只查找顶层窗口(top-level)，不会查找子窗口！
        if (auto res = FindWindowW(LPCWSTR(AppCoreWindowClass.utf16()), LPCWSTR(title.utf16())))
            return res;
        qWarning() << "Failed to find ApplicationCoreWindow of " << title << hwnd;
        return nullptr;
    }

    bool isAppFrameWindow(HWND hwnd) {
        return Util::getClassName(hwnd) == AppFrameWindowClass;
    }

    /// 普通API很难获取UWP的图标，遂手动解析AppxManifest.xml
    /// <br> relativePath: e.g. Assets\\StoreLogo.png
    // ref: https://github.com/microsoft/PowerToys/blob/5b616c9eed776566e728ee6dd710eb706e73e300/src/modules/launcher/Plugins/Microsoft.Plugin.Program/Programs/UWPApplication.cs#L394
    // 函数名：LogoUriFromManifest 说明是从AppxManifest.xml中获取Logo路径
    // ref: https://learn.microsoft.com/zh-cn/windows/uwp/app-resources/images-tailored-for-scale-theme-contrast
    // ref: https://stackoverflow.com/questions/39910625/how-to-properly-structure-uwp-app-icons-in-appxmanifest-xml-file-for-a-win32-app
    QString getLogoPathFromAppxManifest(const QString& manifestPath) {
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
        for (const auto& attr: LogoAttributes) {
            if (auto value = visualElement.attribute(attr); !value.isEmpty())
                return value;
        }

        QDomElement storeLogo = root.firstChildElement("Properties").firstChildElement("Logo");
        if (!storeLogo.isNull())
            return storeLogo.text();

        return {};
    }

    /// 匹配某个变体，如：StoreLogo.scale-200.png
    QIcon loadUWPLogo(const QString& logoPath) {
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
            qDebug() << "Found matching file:" << matchingFiles.first();
            QString logoFile = dir.absoluteFilePath(matchingFiles.first());
            return QIcon(logoFile);
        } else {
            qWarning() << "No matching files found!";
        }
        return {};
    }

    QIcon getAppIcon(const QString& path) {
        QFileInfo fileInfo(path);
        const auto dir = fileInfo.absolutePath() + "\\";
        const auto manifestPath = dir + AppManifest;
        const auto logoPath = dir + getLogoPathFromAppxManifest(manifestPath);
        return loadUWPLogo(logoPath);
    }

    // 用于比较包版本的辅助函数
    bool comparePackageFullNames(const wchar_t* a, const wchar_t* b) {
        PACKAGE_VERSION versionA, versionB;
        UINT32 lengthA = 0, lengthB = 0;

        // Get appropriate length
        if (SUCCEEDED(PackageIdFromFullName(a, PACKAGE_INFORMATION_BASIC, &lengthA, nullptr)) &&
            SUCCEEDED(PackageIdFromFullName(b, PACKAGE_INFORMATION_BASIC, &lengthB, nullptr))) {
            std::vector<BYTE> bufferA(lengthA), bufferB(lengthB);
            auto* idA = reinterpret_cast<PACKAGE_ID*>(bufferA.data());
            auto* idB = reinterpret_cast<PACKAGE_ID*>(bufferB.data());

            if (SUCCEEDED(PackageIdFromFullName(a, PACKAGE_INFORMATION_BASIC, &lengthA, bufferA.data())) &&
                SUCCEEDED(PackageIdFromFullName(b, PACKAGE_INFORMATION_BASIC, &lengthB, bufferB.data()))) {
                versionA = idA->version;
                versionB = idB->version;

                if (versionA.Major != versionB.Major) return versionA.Major > versionB.Major;
                if (versionA.Minor != versionB.Minor) return versionA.Minor > versionB.Minor;
                if (versionA.Build != versionB.Build) return versionA.Build > versionB.Build;
                return versionA.Revision > versionB.Revision;
            }
        }

        // 如果无法比较版本，则按字符串比较
        return wcscmp(a, b) > 0;
    }

    // ref: https://www.cnblogs.com/xyycare/p/18265865/cpp-get-msix-lnk-location
    QString getUWPInstallDirByAUMID(const QString& AUMID) {
        // 1. PackageManager (C++/WinRT) 的方法崩溃（可能需要管理员权限），遂弃之
        // 2. PowerShell + Get-AppxPackage 的方法不需要管理员权限，但速度较慢
        // "Get-AppxPackage -Name '%1' | Select-Object -ExpandProperty InstallLocation"

        QString installPath;
        // 不包含版本号
        QString packageFamilyName = AUMID.split('!').first();

        UINT32 count = 0;
        UINT32 bufferLength = 0;
        // 先获取包的数量和缓冲区大小
        LONG result = GetPackagesByPackageFamily(packageFamilyName.toStdWString().c_str(), &count, nullptr, &bufferLength, nullptr);
        if (result == ERROR_INSUFFICIENT_BUFFER) {
            std::vector<PWSTR> fullNames(count);
            std::vector<wchar_t> buffer(bufferLength);
            // 获取包的全名（包含版本号 + 开发商）
            result = GetPackagesByPackageFamily(packageFamilyName.toStdWString().c_str(), &count, fullNames.data(), &bufferLength,
                                                buffer.data());

            if (result == ERROR_SUCCESS && count > 0) {
                // 按版本排序包名，最新版本在前
                std::sort(fullNames.begin(), fullNames.end(), comparePackageFullNames);

                for (UINT32 i = 0; i < count; ++i) {
                    UINT32 pathLength = 0;
                    result = GetPackagePathByFullName(fullNames[i], &pathLength, nullptr);

                    if (result == ERROR_INSUFFICIENT_BUFFER) {
                        std::vector<wchar_t> pathBuffer(pathLength);
                        result = GetPackagePathByFullName(fullNames[i], &pathLength, pathBuffer.data());

                        if (result == ERROR_SUCCESS) {
                            installPath = QString::fromWCharArray(pathBuffer.data());
                            break;  // 找到最新版本的包就退出
                        }
                    }
                }
            }
        }

        if (installPath.isEmpty()) {
            qWarning() << "Failed to find package for AUMID:" << AUMID;
        }

        return installPath;
    }

    /// 一个AppxManifest.xml可以包含多个exe，通过Id来区分，就离谱<br>
    /// relative path
    QString getExecutableFromAppxManifest(const QString& manifestPath, const QString& id) {
        QString exe;

        QFile file(manifestPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open AppxManifest.xml";
            return exe;
        }

        QDomDocument doc;
        if (!doc.setContent(&file)) {
            qWarning() << "Failed to parse XML content";
            return exe;
        }
        file.close();

        QDomElement root = doc.documentElement();
        QDomNodeList applications = root.elementsByTagName("Application");

        // 遍历所有 "Application" 节点，查找匹配的 Id
        for (int i = 0; i < applications.size(); ++i) {
            QDomElement appElement = applications.at(i).toElement();
            if (!appElement.isNull() && appElement.attribute("Id") == id) {
                exe = appElement.attribute("Executable");
                break;
            }
        }

        return exe;
    }

    QString getUwpExePathByAUMID(const QString& AUMID) {
        auto applicationId = AUMID.split('!').last();
        auto dir = getUWPInstallDirByAUMID(AUMID);
        auto manifest = dir + '\\' + AppManifest;
        auto exePath = getExecutableFromAppxManifest(manifest, applicationId);
        return dir + '\\' + exePath;
    }

    /// name, appid, exePath
    QList<std::tuple<QString, QString, QString>> getStartAppList() {
        auto co_init = CoInitialize(nullptr);

        IShellItem* psi = nullptr;
        // like Get-StartApps
        HRESULT hr = SHCreateItemInKnownFolder(FOLDERID_AppsFolder, 0, nullptr, IID_PPV_ARGS(&psi));

        QList<std::tuple<QString, QString, QString>> appList;

        if (SUCCEEDED(hr)) {
            IEnumShellItems* pEnum = nullptr;
            hr = psi->BindToHandler(nullptr, BHID_EnumItems, IID_PPV_ARGS(&pEnum));

            if (SUCCEEDED(hr)) {
                IShellItem* pChildItem = nullptr;
                while (pEnum->Next(1, &pChildItem, nullptr) == S_OK) {
                    PWSTR _displayName = nullptr;
                    PWSTR _relativePath = nullptr;
                    auto hr_name = pChildItem->GetDisplayName(SIGDN_NORMALDISPLAY, &_displayName);
                    auto hr_path = pChildItem->GetDisplayName(SIGDN_PARENTRELATIVEPARSING, &_relativePath);

                    QString name, relPath;
                    if (SUCCEEDED(hr_path) && SUCCEEDED(hr_name)) {
                        name = QString::fromWCharArray(_displayName);
                        relPath = QString::fromWCharArray(_relativePath);
                    } else
                        qWarning() << "Failed to get display name or relPath.";

                    CComPtr<IPropertyStore> store;
                    // 可以GetCount枚举所有属性
                    hr = pChildItem->BindToHandler(nullptr, BHID_PropertyStore, IID_PPV_ARGS(&store));
                    QString exePath;
                    if (SUCCEEDED(hr)) {
                        PROPVARIANT var;
                        PropVariantInit(&var);
                        // 获取 System.Link.TargetParsingPath 属性
                        hr = store->GetValue(PKEY_Link_TargetParsingPath, &var);
                        if (SUCCEEDED(hr) && var.vt == VT_LPWSTR) {
                            exePath = QString::fromWCharArray(var.pwszVal);
                        } else {
                            exePath = AppUtil::getUwpExePathByAUMID(relPath);
                        }
                        PropVariantClear(&var);
                    } else
                        qWarning() << "Failed to get property store.";

                    appList.append({name, relPath, exePath});

                    CoTaskMemFree(_relativePath);
                    CoTaskMemFree(_displayName);
                    pChildItem->Release();
                }
                pEnum->Release();
            }
            psi->Release();
        }

        if (SUCCEEDED(co_init))
            CoUninitialize();

        return appList;
    }

    /// Warning: 对于没有加入StartMenu且不是UWP的应用，fail, like `Rizonesoft.Notepad3`<br>
    /// 还有更奇怪的，Clash for Windows, 在getStartAppList()中的AppID是绝对路径，但是AutoAnimation获取的是"com.lbyczf.clashwin"对不上<br>
    /// So: 如果AppID无匹配，则转向Name匹配
    QString getExePathFromAppIdOrName(const QString& appid, const QString& appName) {
        static QHash<QString, QString> appid2Path;
        static QHash<QString, QString> name2Path;
        static auto buildStartAppMaps = []() { // cache, 耗时
            auto list = getStartAppList(); // build from Start Apps
            for (const auto& [name, appId, exePath]: list) {
                appid2Path.insert(appId, exePath);
                name2Path.insert(name, exePath);
            }
        };
        if (appid2Path.isEmpty()) // init & cache
            buildStartAppMaps();

        if (appid.isEmpty()) return {};
        if (QFile::exists(appid)) // if appid is exe path
            return appid;
        if (appid == "Microsoft.Windows.Explorer") // ControlPanel Explorer RunDialog 比较特殊 貌似其实都是explorer
            return "C:\\Windows\\explorer.exe";
        static QStringList BlackList;
        if (BlackList.contains(appid)) return {};

        if (auto exe = appid2Path.value(appid); !exe.isEmpty())
            return exe;
        if (auto exe = name2Path.value(appName); !exe.isEmpty())
            return exe;

        buildStartAppMaps(); // rebuild
        qDebug() << "Not Found. Rebuild AppId2ExePathMap";

        if (auto exe = appid2Path.value(appid); !exe.isEmpty())
            return exe;
        if (auto exe = name2Path.value(appName); !exe.isEmpty())
            return exe;

        // add to blacklist, if also failed after rebuild
        qWarning() << "Failed to find exe path for appid:" << appid;
        BlackList.append(appid);
        return {};
    }
}