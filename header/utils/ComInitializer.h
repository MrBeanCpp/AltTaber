#ifndef WIN_SWITCHER_COMINITIALIZER_H
#define WIN_SWITCHER_COMINITIALIZER_H

#include <windows.h>
#include <QtDebug>

/** @warning
 *
 * 由于STA套件所有的COM组件代码都运行于主STA（第一个调用CoInitialize函数的线程）,
 * 如果你的主线程没有调用CoInitialize，那么第一个调用CoInitialize的工作线程就会成为主STA，
 * 而工作线程随时可能中止，这种情况下，一旦工作线程中止主STA也就不复存在了，
 * 因此你需要在主线程中调用CoInitialize初始化主STA，即使主线程不使用任何COM组件。
 * <br>-- https://www.cnblogs.com/manors/archive/2010/05/17/COM_Initialize_STA_MTA.html<br>
 *
 * 应用程序中第一个使用 0 调用CoInitialize （或使用 COINIT_APARTMENTTHREADED 调用CoInitializeEx ）的线程必须是最后一个调用CoUninitialize的线程。
 * 否则，后续在 STA 上调用CoInitialize将失败，并且应用程序将无法工作。
 * <br>-- https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-coinitialize<br>
*/
// 每个线程都要单独初始化COM
class ComInitializer { // RAII
public:
    ComInitializer() {
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); // == CoInitialize(nullptr); single-threaded apartment
        if (SUCCEEDED(hr))
            qInfo() << (hr == S_OK ? "COM initialized." : "COM has been initialized before.");
        else
            qCritical() << "Failed to initialize COM:" << hr;
    }

    ~ComInitializer() {
        // 调用 CoInitializeEx 的第一个线程必须是调用 CoUninitialize 的最后一个线程。 否则，对 STA 上的 CoInitialize 的后续调用将失败，应用程序将不起作用。
        // 每次成功调用 CoInitialize 或 CoInitializeEx（包括返回S_FALSE）都必须通过对 CoUninitialize 的相应调用来平衡
        if (SUCCEEDED(hr)) {
            CoUninitialize();
            qInfo() << "COM uninitialized.";
        }
    }

private:
    HRESULT hr;
};


#endif //WIN_SWITCHER_COMINITIALIZER_H
