#include "utils/winEventHook.h"
#include <QDebug>
#include <QTime>

static HWINEVENTHOOK handler = nullptr;
static WinEventCallback callback = nullptr;

void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread,
                           DWORD dwmsEventTime) {
    if (idObject != OBJID_WINDOW) // 确保对象是窗口，而不是子对象（如按钮）
        return;

    if (callback)
        callback(event, hwnd);
}

bool setWinEventHook(WinEventCallback _callback) {
    if (handler) {
        qWarning() << "WinEventHook already set. Unhook first.";
        return false;
    }

    ::callback = std::move(_callback);

    // WINEVENT_OUTOFCONTEXT：表示回调函数是在调用线程的上下文中调用的，而不是在生成事件的线程的上下文中。这种方式不需要DLL模块句柄（hmodWinEventProc 设置为 NULL）
    handler = SetWinEventHook(
            EVENT_SYSTEM_FOREGROUND,
            EVENT_SYSTEM_FOREGROUND,
            nullptr,
            WinEventProc,
            0,
            0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS // 跳过自身进程的事件
    );

    qDebug() << "Set WinEventHook.";

    return handler != nullptr;
}

void unhookWinEvent() {
    if (!handler) return;

    callback = nullptr;
    UnhookWinEvent(handler);
    handler = nullptr;

    qDebug() << "Unhook win event.";
}
