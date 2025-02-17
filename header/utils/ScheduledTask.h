#ifndef SCHEDULEDTASK_H
#define SCHEDULEDTASK_H
#include <QPair>


class ScheduledTask {
    static QPair<QString, QString> queryAuthorUserId();
    static QString createTaskXml(const QString& exePath, const QString& description, bool asAdmin = true, int priority = 5);

public:
    ScheduledTask() = delete;
    static bool createTask(const QString& taskName);
    static bool queryTask(const QString& taskName);
    static bool deleteTask(const QString& taskName);
};


#endif //SCHEDULEDTASK_H
