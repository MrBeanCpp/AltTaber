#include "utils/TaskbarWheelHooker.h"
#include <QTimer>
#include <QDebug>
#include "utils/uiautomation.h"
#include "utils/AppUtil.h"
#include "utils/Util.h"
#include <QCursor>

LRESULT mouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_MOUSEWHEEL) {
        auto* data = (MSLLHOOKSTRUCT*) lParam;
        HWND topLevelHwnd = Util::topWindowFromPoint(data->pt);
        if (Util::getClassName(topLevelHwnd) == "Shell_TrayWnd") {
            auto delta = (short) HIWORD(data->mouseData);

            auto el = UIAutomation::getElementUnderMouse();
            if (el.getClassName() == "Taskbar.TaskListButtonAutomationPeer") {
                auto appid = el.getAutomationId().mid(QStringLiteral("Appid: ").size());
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
        HWND topLevelHwnd = Util::topWindowFromPoint(QCursor::pos());
        bool isTaskbar = (Util::getClassName(topLevelHwnd) == QStringLiteral("Shell_TrayWnd")); // TODO 副屏是 Shell_SecondaryTrayWnd
        if (isLastTaskbar != isTaskbar) {
            isLastTaskbar = isTaskbar;
            if (isTaskbar) {
                // 依赖事件循环
                h_mouse = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) mouseProc, GetModuleHandle(nullptr), 0);
                if (h_mouse == nullptr)
                    qCritical() << "Failed to install h_mouse";
                qDebug() << "#Enter Taskbar";
            } else {
                UnhookWindowsHookEx(h_mouse);
                h_mouse = nullptr;
                qDebug() << "#Leave Taskbar";
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
