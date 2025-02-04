#ifndef WIN_SWITCHER_CONFIGMANAGERBASE_H
#define WIN_SWITCHER_CONFIGMANAGERBASE_H

#include <QSettings>
#include <QProcess>

class ConfigManagerBase : public QObject {
    Q_OBJECT

protected:
    QSettings settings;
    QProcess proc_editor;

public:
    ConfigManagerBase(const ConfigManagerBase&) = delete;
    ConfigManagerBase& operator=(const ConfigManagerBase&) = delete;

    QVariant get(QAnyStringView key, const QVariant& defaultValue = QVariant()) {
        return settings.value(key, defaultValue);
    }

    void set(QAnyStringView key, const QVariant& value) {
        settings.setValue(key, value);
    }

    void remove(QAnyStringView key) {
        settings.remove(key);
    }

    /// Writes any unsaved changes to permanent storage, and reloads any settings that have been changed in the meantime by another application.
    void sync() {
        // This function is called automatically from QSettings's destructor and by the event loop at regular intervals,
        // so you normally don't need to call it yourself.
        settings.sync();
    }

    /// Edit config file with `notepad.exe` & emit `configEdited` signal when finished
    void editConfigFile() {
        if (proc_editor.state() == QProcess::Running) {
            qDebug() << "Editor is running";
            return;
        }

        qDebug() << "#Editing config file" << settings.fileName();
        // QProcess 只能同时连接一个进程
        proc_editor.start("notepad", {settings.fileName()});
    }

protected:
    explicit ConfigManagerBase(const QString& filename) : settings(filename, QSettings::IniFormat) {
        connect(&proc_editor, &QProcess::finished, this, [this] {
            qDebug() << "#Config file edit finished";
            sync(); // reload
            emit configEdited();
        });
    }

signals:
    void configEdited(); // edit via `Settings` action, not other ways
};


#endif //WIN_SWITCHER_CONFIGMANAGERBASE_H
