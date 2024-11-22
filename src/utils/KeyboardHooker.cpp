#include "utils/KeyboardHooker.h"
#include <QDebug>
#include <QApplication>
#include <QKeyEvent>
#include "utils/Util.h"
#include "widget.h"

LRESULT keyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    using Hooker = KeyboardHooker;
    if (nCode == HC_ACTION) {
        if (wParam == WM_SYSKEYDOWN || wParam == WM_KEYDOWN) { // Alt & [Alt按下时的Tab]属于SysKey
            auto* pKeyBoard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            // inner: `GetAsyncKeyState`, doc warns this usage, but it seems to work fine(?)
            // If it's broken, maybe we can record Modifier manually in every callback
            /* Note from Docs:
             * When this callback function is called in response to a change in the state of a key,
             * the callback function is called before the asynchronous state of the key is updated.
             * Consequently, the asynchronous state of the key cannot be determined by calling GetAsyncKeyState from within the callback function.
             * */
            bool isAltPressed = Util::isKeyPressed(VK_MENU);

            if (isAltPressed && Hooker::receiver) {
                if (pKeyBoard->vkCode == VK_TAB) {
                    qDebug() << "Alt+Tab detected!";
                    if ((HWND) Hooker::receiver->winId() != GetForegroundWindow()) { // not Foreground
                        // 异步，防止阻塞；超过1s会导致被系统强制绕过，传递给下一个钩子
                        QMetaObject::invokeMethod(Hooker::receiver, "requestShow", Qt::QueuedConnection);
                    } else {
                        // 转发Alt+Tab给Widget
                        auto shiftModifier = Util::isKeyPressed(VK_SHIFT) ? Qt::ShiftModifier : Qt::NoModifier;
                        auto tabDownEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::AltModifier | shiftModifier);
                        QApplication::postEvent(Hooker::receiver, tabDownEvent); // async
                    }
                    return 1; // 阻止事件传递
                } else if (pKeyBoard->vkCode == VK_OEM_3) { // ~`
                    qDebug() << "Alt+` detected!";
                    auto shiftModifier = Util::isKeyPressed(VK_SHIFT) ? Qt::ShiftModifier : Qt::NoModifier;
                    auto event = new QKeyEvent(QEvent::KeyPress, Qt::Key_QuoteLeft, Qt::AltModifier | shiftModifier);
                    QApplication::postEvent(Hooker::receiver, event); // async
                    return 1; // 阻止事件传递
                }
            }
        } else if (wParam == WM_KEYUP) { // Amazing, Alt Down is `WM_SYSKEYDOWN`, but release is `WM_KEYUP`
            auto* pKeyBoard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            if (pKeyBoard->vkCode == VK_LMENU && Hooker::receiver) {
                // BUG: Alt + 方向键 长按，过一秒会触发Alt release，而Alt + 其他键则不会，可能是Windows保护机制或键盘问题？
                qDebug() << "Alt released!";
                auto event = new QKeyEvent(QEvent::KeyRelease, Qt::Key_Alt, Qt::NoModifier);
                QApplication::postEvent(Hooker::receiver, event); // async
                // not block
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

KeyboardHooker::KeyboardHooker(QWidget* _receiver) {
    if (KeyboardHooker::receiver) {
        qWarning() << "Only one KeyboardHooker can be installed!";
        return;
    }
    // 回调函数的执行与消息循环密切相关，在Get/PeekMessage时，系统才会触发回调; [https://learn.microsoft.com/en-us/windows/win32/winmsg/mouseproc]
    h_keyboard = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC) keyboardProc, GetModuleHandle(nullptr), 0);
    if (!h_keyboard) {
        qWarning() << "Failed to install h_keyboard!";
        return;
    }
    if (!_receiver) {
        qWarning() << "Receiver is nullptr!";
        return;
    }
    KeyboardHooker::receiver = _receiver;
    qInfo() << "KeyboardHooker installed";
}

KeyboardHooker::~KeyboardHooker() {
    if (!h_keyboard) return;
    UnhookWindowsHookEx(h_keyboard);
    KeyboardHooker::receiver = nullptr;
    qDebug() << "KeyboardHooker uninstalled";
}
