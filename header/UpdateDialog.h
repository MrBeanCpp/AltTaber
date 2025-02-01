#ifndef WIN_SWITCHER_UPDATEDIALOG_H
#define WIN_SWITCHER_UPDATEDIALOG_H

#include <QApplication>
#include <QDialog>
#include <QFile>
#include <QNetworkAccessManager>
#include <QVersionNumber>

QT_BEGIN_NAMESPACE

namespace Ui {
    class UpdateDialog;
}

QT_END_NAMESPACE

class UpdateDialog final : public QDialog {
    Q_OBJECT

public:
    explicit UpdateDialog(QWidget* parent = nullptr);
    ~UpdateDialog() override;

private:
    void fetchGithubReleaseInfo();
    void download(const QString& url, const QString& savePath);
    static QVersionNumber normalizeVersion(const QString& ver);
    static QString toLocalTime(const QString& isoTime);

signals:
    void downloadSucceed(const QString& filePath);

protected:
    void showEvent(QShowEvent*) override;

private:
    Ui::UpdateDialog* ui;
    QNetworkAccessManager manager;
    static constexpr auto Owner = "MrBeanCpp";
    static constexpr auto Repo = "AltTaber";
    const QVersionNumber version = normalizeVersion(QApplication::applicationVersion());

    struct ReleaseInfo {
        QVersionNumber ver;
        QString description;
        QString downloadUrl;
        QString publishTime;
    } relInfo;

    struct {
        bool success = false;
        QNetworkReply* reply = nullptr;
        QFile file;
    } downloadStatus;
};


#endif //WIN_SWITCHER_UPDATEDIALOG_H
