#ifndef WIN_SWITCHER_TASKBARWHEELHOOKER_H
#define WIN_SWITCHER_TASKBARWHEELHOOKER_H

#include <QObject>
#include <Windows.h>

class TaskbarWheelHooker : public QObject {
Q_OBJECT
public:
    TaskbarWheelHooker();
    ~TaskbarWheelHooker() override;
    inline static TaskbarWheelHooker* instance = nullptr;

signals:
    void tabWheelEvent(const QString& exePath, bool isUp, int windows); // 参数为引用问题也不大，貌似会自动拷贝（Qt::QueuedConnection情况下）
    void leaveTaskbar(); // 鼠标离开taskbar

private:
    HHOOK h_mouse = nullptr;
};


#endif //WIN_SWITCHER_TASKBARWHEELHOOKER_H
