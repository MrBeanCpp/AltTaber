#ifndef WIN_SWITCHER_CONFIGMANAGERBASE_H
#define WIN_SWITCHER_CONFIGMANAGERBASE_H

#include <QSettings>

class ConfigManagerBase {
protected:
    QSettings settings;
public:
    ConfigManagerBase(const ConfigManagerBase&) = delete;
    ConfigManagerBase& operator=(const ConfigManagerBase&) = delete;

    virtual ~ConfigManagerBase() = default;

    QVariant get(QAnyStringView key, const QVariant& defaultValue = QVariant()) {
        return settings.value(key, defaultValue);
    }

    void set(QAnyStringView key, const QVariant& value) {
        settings.setValue(key, value);
    }

    void remove(QAnyStringView key) {
        settings.remove(key);
    }

    void sync() {
        // This function is called automatically from QSettings's destructor and by the event loop at regular intervals,
        // so you normally don't need to call it yourself.
        settings.sync();
    }

protected:
    explicit ConfigManagerBase(const QString& filename) : settings(filename, QSettings::IniFormat) {}

};


#endif //WIN_SWITCHER_CONFIGMANAGERBASE_H
