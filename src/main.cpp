#include <QApplication>
#include <windows.h>
#include <QTimer>
#include "../header/widget.h"
#include "utils/winEventHook.h"
#include "utils/Util.h"
#include "utils/uiautomation.h"
#include "utils/AppUtil.h"
#include "utils/KeyboardHooker.h"

Widget* winSwitcher = nullptr;

LRESULT mouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_MOUSEWHEEL) {
        auto* data = (MSLLHOOKSTRUCT*) lParam;
        HWND hwnd = WindowFromPoint(data->pt);
        HWND topLevelHwnd = GetAncestor(hwnd, GA_ROOT);
        if (Util::getClassName(topLevelHwnd) == "Shell_TrayWnd") {
            auto delta = (short) HIWORD(data->mouseData);
            qDebug() << delta << Util::getClassName(topLevelHwnd);

            auto el = UIAutomation::getElementUnderMouse();
            if (el.getClassName() == "Taskbar.TaskListButtonAutomationPeer") {
                auto appid = el.getAutomationId().mid(QStringLiteral("Appid: ").size());
                qDebug() << el.getName() << el.getClassName() << AppUtil::getExePathFromAppId(appid);
            }

//        QPoint pos(data->pt.x, data->pt.y);
//        qApp->postEvent(receiver, new QWheelEvent(pos, delta, Qt::NoButton, Qt::NoModifier));
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    HHOOK h_mouse = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) mouseProc, GetModuleHandle(nullptr), 0);
    if (h_mouse == nullptr)
        qDebug() << "Failed to install h_mouse";
    QObject::connect(&a, &QApplication::aboutToQuit, [h_mouse]() {
        UnhookWindowsHookEx(h_mouse);
        unhookWinEvent();
        qDebug() << "Hook uninstalled";
        UIAutomation::cleanup();
    });

    winSwitcher = new Widget;
    winSwitcher->prepareListWidget(); // 优化：对ListWidget进行预先初始化，首次执行`setCurrentRow`特别耗时(472ms)

    KeyboardHooker kbHooker(winSwitcher);

    setWinEventHook([](DWORD event, HWND hwnd) {
        // 某些情况下，Hook拦截不到Alt+Tab（如VMware获取焦点且虚拟机开启时，即便focus在标题栏上）
        // （GPT建议RegisterRawInputDevices，不知道有没有效果，感觉比较危险）
        // 此时需要通过监控前台窗口检测系统的任务切换窗口唤出，并弹出本程序
        // 一旦焦点脱离VMWare，Hook就能正常工作，接下里就可以正常拦截Alt+Tab
        // 而再次连续按下TAB（Alt按住的情况下），也不会出现反复弹出系统任务切换窗口的情况（如果不进行Hook拦截就会这样，所以两种方法结合使用）
        if (event == EVENT_SYSTEM_FOREGROUND && hwnd == GetForegroundWindow()) { // 前台窗口变化 TODO 记录窗口关闭事件
            // 使用Alt+TAB呼出任务切换窗口时，会触发两次EVENT_SYSTEM_FOREGROUND事件
            // 第二次是在目标窗口已经切换到前台后触发的，非常诡异
            // 可以用 GetForegroundWindow() || isAltPressed 来二次确认
            winSwitcher->notifyForegroundChanged(hwnd);
            auto className = Util::getClassName(hwnd);
            // ForegroundStaging貌似是辅助过渡动画
            if (className == "ForegroundStaging" || className == "XamlExplorerHostIslandWindow") { // 任务切换窗口
                qDebug() << "任务切换 detected!";
                int t = 0;
                do {
                    // 貌似如果不是本进程第一个窗口的话，这招无法前置，比如你在这里new Widget
                    winSwitcher->requestShow();
                    t++;
                    if (t > 1)
                        qDebug() << "Retry" << t;
                    Sleep(10);
                } while (!winSwitcher->isForeground() && t < 5);
            }
        }
    });

    return QApplication::exec();
}
