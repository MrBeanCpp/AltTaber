#include "utils/ScheduledTask.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QMessageBox>
#include <QProcess>
#include <QString>

/// Query Author and User ID via `PowerShell` for schtasks.exe xml
/// <br> GetTokenInformation & LookupAccountSidW is too complex
QPair<QString, QString> ScheduledTask::queryAuthorUserId() {
    const QString Command = R"(
        $identity = [System.Security.Principal.WindowsIdentity]::GetCurrent()
        $author = $identity.Name
        $sid = $identity.User.Value
        Write-Output "$author`n$sid"
    )";
    QProcess process;
    process.start("powershell", QStringList() << "-Command" << Command);
    process.waitForFinished();

    const auto output = process.readAllStandardOutput();
    const auto list = QString(output).replace("\r\n", "\n").split('\n', Qt::SkipEmptyParts);
    Q_ASSERT(list.size() == 2);
    return {list.at(0), list.at(1)};
}

// ref: https://github.com/Snipaste/feedback/issues/498
// ref: https://learn.microsoft.com/zh-cn/windows/win32/taskschd/tasksettings-priority
// ref: https://learn.microsoft.com/zh-cn/windows/win32/taskschd/daily-trigger-example--xml-
/// Start when user logon
/// <br>注意：默认情况下，用命令行参数创建的任务进程优先级较低，只能采用xml修改优先级
QString ScheduledTask::createTaskXml(const QString& exePath, const QString& description, bool asAdmin, int priority) {
    const QString isoTime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    const auto [author, userId] = queryAuthorUserId();
    // Clion Nova bug: 必须把xml字符串的缩进也改成4个空格，否则整个cpp文件都会被格式化为2个空格
    return QString(R"xml(<?xml version="1.0" encoding="UTF-16"?>
<Task version="1.2" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task">
    <RegistrationInfo>
          <Date>%1</Date>
          <Author>%2</Author>
          <Description>%3</Description>
    </RegistrationInfo>
    <Triggers>
          <LogonTrigger>
                <Enabled>true</Enabled>
          </LogonTrigger>
    </Triggers>
    <Principals>
          <Principal id="Author">
                <UserId>%4</UserId>
                <LogonType>InteractiveToken</LogonType>
                <RunLevel>%5</RunLevel>
          </Principal>
    </Principals>
    <Settings>
        <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>
        <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>
        <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>
        <AllowHardTerminate>false</AllowHardTerminate>
        <StartWhenAvailable>false</StartWhenAvailable>
        <RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>
        <IdleSettings>
            <StopOnIdleEnd>false</StopOnIdleEnd>
            <RestartOnIdle>false</RestartOnIdle>
        </IdleSettings>
        <AllowStartOnDemand>true</AllowStartOnDemand>
        <Enabled>true</Enabled>
        <Hidden>false</Hidden>
        <RunOnlyIfIdle>false</RunOnlyIfIdle>
        <WakeToRun>false</WakeToRun>
        <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>
        <Priority>%6</Priority>
    </Settings>
    <Actions Context="Author">
        <Exec>
            <Command>%7</Command>
        </Exec>
    </Actions>
</Task>
)xml").arg(isoTime)
      .arg(author)
      .arg(description)
      .arg(userId)
      .arg(asAdmin ? "HighestAvailable" : "LeastPrivilege")
      .arg(priority) // 计划任务中默认是7（低于正常），为了改变这个值，只能通过xml
      .arg(QDir::toNativeSeparators(exePath));
}

bool ScheduledTask::createTask(const QString& taskName) {
    const auto xml = createTaskXml(qApp->applicationFilePath(), "AltTaber startup as Admin");
    QFile file(qApp->applicationDirPath() + "/schtasks.xml");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(xml.toUtf8());
        file.close();
    } else {
        qWarning() << "Failed to write schtasks.xml";
        return false;
    }

    QProcess process;
    process.start("schtasks",
                  QStringList() << "/create" << "/tn" << taskName << "/xml" << file.fileName() << "/f");
    process.waitForFinished();
    file.remove();
    bool isOk = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    if (!isOk) {
        qWarning() << "Failed to create schtasks:" << QString::fromLocal8Bit(process.readAllStandardError())
                << process.exitStatus() << process.exitCode();
    }
    return isOk;
}

bool ScheduledTask::queryTask(const QString& taskName) {
    QProcess process;
    // (Get-ScheduledTask -TaskName "taskName").Actions.Executable 方便但是巨慢（1000ms）
    process.start("schtasks", QStringList() << "/query" << "/tn" << taskName << "/FO" << "CSV" << "/NH" << "/V");
    process.waitForFinished();
    // Task not exist
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qDebug() << "Task not exist:" << taskName;
        return false;
    }

    auto output = QString::fromLocal8Bit(process.readAllStandardOutput());
    auto csvList = output.simplified().removeFirst().chopped(1).split("\",\""); // ","
    auto exePath = csvList.at(8).simplified();
    if (!exePath.endsWith(".exe", Qt::CaseInsensitive)) {
        qWarning() << "[queryTask] exePath(csv) parse error:" << exePath;
        QMessageBox::warning(nullptr, "Query ScheduledTask", "exePath(csv) parse error");
        return false;
    }
    qDebug() << "queryTask exePath:" << exePath;

    static const QString AppPath = QDir::toNativeSeparators(qApp->applicationFilePath()); // IMPORTANT
    if (exePath != AppPath) {
        qWarning() << "Task exists, but exePath mismatch:" << exePath << AppPath;
        return false;
    }

    return true;
}

bool ScheduledTask::deleteTask(const QString& taskName) {
    QProcess process;
    process.start("schtasks", QStringList() << "/delete" << "/tn" << taskName << "/f");
    process.waitForFinished();
    bool isOk = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    if (!isOk) {
        qWarning() << "Failed to create schtasks:" << QString::fromLocal8Bit(process.readAllStandardError())
                << process.exitStatus() << process.exitCode();
    }
    return isOk;
}
