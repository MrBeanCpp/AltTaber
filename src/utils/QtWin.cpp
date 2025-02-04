#include <ShObjIdl_core.h>
#include "utils/QtWin.h"
#include <windows.h>
#include <QtDebug>

namespace QtWin {
    /// internal
    ITaskbarList3* qt_createITaskbarList3() { // 每个线程统一进行COM初始化，不在特定函数中进行
        ITaskbarList3* pTbList = nullptr;
        HRESULT result = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList3,
                                          reinterpret_cast<void**>(&pTbList));
        if (SUCCEEDED(result)) {
            if (FAILED(pTbList->HrInit())) {
                pTbList->Release();
                pTbList = nullptr;
            }
        }
        return pTbList;
    }

    void taskbarDeleteTab(QWidget* window) {
        ITaskbarList* pTbList = qt_createITaskbarList3();
        if (pTbList) {
            pTbList->DeleteTab(reinterpret_cast<HWND>(window->winId()));
            pTbList->Release();
        }
    }

    /// new implementation for Qt6
    QPixmap fromHICON(HICON icon) {
        return QPixmap::fromImage(QImage::fromHICON(icon));
    }
} // QtWin
