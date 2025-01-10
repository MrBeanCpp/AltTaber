#ifndef WIN_SWITCHER_SINGLEAPP_H
#define WIN_SWITCHER_SINGLEAPP_H

#include <QSharedMemory>
#include <QDebug>

class SingleApp {
private:
    QSharedMemory sharedMemory; // auto detach in destructor

public:
    explicit SingleApp(const QString& key) : sharedMemory(key) {
        // region Nothing: Just record for future
        // 确保清理可能存在的残留共享内存 for linux:
        // linux 应用程序崩溃后,共享内存段不会自动销毁,则该程序再次运行会出问题
        // Windows 会自动清理
        // if (sharedMemory.attach())
        //    sharedMemory.detach();
        // endregion https://blog.csdn.net/bloke_come/article/details/106319236
    }

    /// check if another instance is running
    bool isRunning() {
        if (sharedMemory.attach()) { // sharedMemory exists
            sharedMemory.detach();
            return true;
        }

        if (sharedMemory.create(1)) { // create sharedMemory (1 byte)
            qInfo() << "SingleApp: sharedMemory created";
            return false;
        } else {
            qWarning() << "fatal: SharedMemory create failed" << sharedMemory.errorString();
            return true;  // 保守起见，认为已有实例在运行
        }
    }
};

#endif //WIN_SWITCHER_SINGLEAPP_H
