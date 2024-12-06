#include "utils/TaskbarWheelHooker.h"
#include <QTimer>
#include "utils/uiautomation.h"
#include "utils/AppUtil.h"
#include "utils/Util.h"
#include <QTime>

LRESULT mouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_MOUSEWHEEL) {
        auto* data = (MSLLHOOKSTRUCT*) lParam;
        HWND topLevelHwnd = Util::topWindowFromPoint(data->pt);
        if (Util::isTaskbarWindow(topLevelHwnd)) {
            auto delta = (short) HIWORD(data->mouseData);

            auto el = UIAutomation::getElementUnderMouse();
            qDebug() << delta << el.getClassName() << el.getAutomationId() << el.getName();
            if (el.getClassName() == "CEF-OSC-WIDGET") { // Nvidia Overlay
                // 当所有窗口最小化后，会出现这种情况，但是焦点和前台都不是他，离谱
                qDebug() << "detect CEF, try active taskbar";
                Util::switchToWindow(topLevelHwnd, true); // 只能通过变焦到Taskbar使Element正常检测
                el = std::move(UIAutomation::getElementUnderMouse());
                qDebug() << (el.getClassName() != "CEF-OSC-WIDGET" ? "successful!" : "failed");
            }
            if (el.getClassName() == "Taskbar.TaskListButtonAutomationPeer") {
                auto appid = el.getAutomationId().mid(QStringLiteral("Appid: ").size()); // TODO 在副屏 有时候会是"TaskbarFrame"
                auto name = el.getName();
                int windows = 0;
                const auto Dash = QStringLiteral(" - ");
                if (auto dashIdx = name.lastIndexOf(Dash); dashIdx != -1) { // "Clash for Windows - 1 个运行窗口"
                    std::stringstream ss(name.mid(dashIdx + Dash.size()).toStdString());
                    ss >> windows; // 从标题手动解析真实窗口数量，程序内部由于过滤逻辑的存在，可能不准确
                    name = name.left(dashIdx);
                }
                auto exePath = AppUtil::getExePathFromAppIdOrName(appid, name);
//                qDebug() << name << windows << appid;
                emit TaskbarWheelHooker::instance->tabWheelEvent(exePath, delta > 0, windows);
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

TaskbarWheelHooker::TaskbarWheelHooker() {
    if (instance) {
        qCritical() << "Only one TaskbarWheelHooker can be installed!";
        return;
    }
    instance = this;
    AppUtil::getExePathFromAppIdOrName(); // cache

    auto* timer = new QTimer(this);
    timer->callOnTimeout(this, [this]() {
        static bool isLastTaskbar = false;
        HWND topLevelHwnd = Util::topWindowFromPoint(Util::getCursorPos());
        bool isTaskbar = Util::isTaskbarWindow(topLevelHwnd);
        if (isLastTaskbar != isTaskbar) {
            isLastTaskbar = isTaskbar;
            if (isTaskbar) {
                // 依赖事件循环
                h_mouse = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) mouseProc, GetModuleHandle(nullptr), 0);
                if (h_mouse == nullptr)
                    qCritical() << "Failed to install h_mouse";
                qDebug() << "#Enter Taskbar" << QTime::currentTime();
            } else {
                UnhookWindowsHookEx(h_mouse);
                h_mouse = nullptr;
                qDebug() << "#Leave Taskbar" << QTime::currentTime();
                emit leaveTaskbar();
            }
        }
    });
    timer->start(50);
}

TaskbarWheelHooker::~TaskbarWheelHooker() {
    if (h_mouse) {
        UnhookWindowsHookEx(h_mouse);
        qDebug() << "MouseHooker uninstalled";
    }
    TaskbarWheelHooker::instance = nullptr;
    UIAutomation::cleanup();
}
