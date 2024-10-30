#include "utils/setWindowBlur.h"

void setWindowBlur(HWND hWnd) {
    HMODULE hUser = GetModuleHandle(L"user32.dll");
    if (hUser) {
        auto setWindowCompositionAttribute = (pfnSetWindowCompositionAttribute) GetProcAddress(hUser, "SetWindowCompositionAttribute");
        if (setWindowCompositionAttribute) {
            ACCENT_POLICY accent = {ACCENT_ENABLE_BLURBEHIND, 0, 0, 0}; //ACCENT_ENABLE_ACRYLICBLURBEHIND亚克力效果，与第三个参数配合使用，但卡得一批
            WINDOWCOMPOSITIONATTRIBDATA data;
            data.Attrib = WCA_ACCENT_POLICY;
            data.pvData = &accent;
            data.cbData = sizeof(accent);
            setWindowCompositionAttribute(hWnd, &data);
        }
    }
}
