﻿#include <QApplication>
#include <QPushButton>
#include <QDebug>
#include <windows.h>
#include <QTimer>
#include "../header/widget.h"
#include "winEventHook.h"
#include "utils/Util.h"
#include <QKeyEvent>

Widget *widget = nullptr;

LRESULT keyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        if (wParam == WM_SYSKEYDOWN) { // Alt & [Alt按下时的Tab]属于SysKey
            auto* pKeyBoard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
//            qDebug() << "Key pressed:" << pKeyBoard->vkCode;

            bool isAltPressed = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

            if (pKeyBoard->vkCode == VK_TAB && isAltPressed) {
                qDebug() << "Alt+Tab detected!";
                if (widget) {
                    if (!widget->isForeground()) {
                        // 异步，防止阻塞；超过1s会导致被系统强制绕过，传递给下一个钩子
                        QMetaObject::invokeMethod(widget, "forceShow", Qt::QueuedConnection);
                    } else {
                        // 转发Alt+Tab给Widget
                        auto tabDownEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::AltModifier);
                        QApplication::postEvent(widget, tabDownEvent); // async
                    }
                }
                return 1; // 阻止事件传递
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)keyboardProc, GetModuleHandle(nullptr), 0);
    if (hook == nullptr) {
        qDebug() << "Failed to install hook";
    }
    QObject::connect(&a, &QApplication::aboutToQuit, [hook](){
        UnhookWindowsHookEx(hook);
        unhookWinEvent();
        qDebug() << "Hook uninstalled";
    });

    widget = new Widget;
    setWinEventHook([&](DWORD event, HWND hwnd) {
        // 某些情况下，Hook拦截不到Alt+Tab（如VMware获取焦点且虚拟机开启时，即便focus在标题栏上）
        // （GPT建议RegisterRawInputDevices，不知道有没有效果，感觉比较危险）
        // 此时需要通过监控前台窗口检测系统的任务切换窗口唤出，并弹出本程序
        // 一旦焦点脱离VMWare，Hook就能正常工作，接下里就可以正常拦截Alt+Tab
        // 而再次连续按下TAB（Alt按住的情况下），也不会出现反复弹出系统任务切换窗口的情况（如果不进行Hook拦截就会这样，所以两种方法结合使用）
        if (event == EVENT_SYSTEM_FOREGROUND && hwnd == GetForegroundWindow()) { // 前台窗口变化
            // 使用Alt+TAB呼出任务切换窗口时，会触发两次EVENT_SYSTEM_FOREGROUND事件
            // 第二次是在目标窗口已经切换到前台后触发的，非常诡异
            // 可以用 GetForegroundWindow() || isAltPressed 来二次确认
            auto className = Util::getClassName(hwnd);
            qDebug() << "Class Name:" << className;

            // ForegroundStaging貌似是辅助过渡动画
            if (className == "ForegroundStaging" || className == "XamlExplorerHostIslandWindow") { // 任务切换窗口
                qDebug() << "任务切换 detected!";
                int t = 0;
                do {
                    // 貌似如果不是本进程第一个窗口的话，这招无法前置，比如你在这里new Widget
                    widget->forceShow();
                    t++;
                    if (t > 1)
                        qDebug() << "Retry" << t;
                    Sleep(10);
                } while (!widget->isForeground() && t < 5);
            }
        }
    });

    return QApplication::exec();
}
