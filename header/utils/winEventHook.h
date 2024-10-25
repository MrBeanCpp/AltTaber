#ifndef WINEVENTHOOK_H
#define WINEVENTHOOK_H

#include <Windows.h>
#include <functional>
/// DWORD event, HWND hwnd
using WinEventCallback = std::function<void(DWORD, HWND)>;

bool setWinEventHook(WinEventCallback _callback);
void unhookWinEvent();

#endif // WINEVENTHOOK_H
