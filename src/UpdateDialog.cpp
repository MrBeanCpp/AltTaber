// You may need to build the project (run Qt uic code generator) to get "ui_UpdateDialog.h" resolved

#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"
#include <QDebug>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "utils/SystemTray.h"

UpdateDialog::UpdateDialog(QWidget* parent) : QDialog(parent), ui(new Ui::UpdateDialog) {
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowTitle("AltTaber Updater[GitHub]");
    qDebug() << QSslSocket::sslLibraryBuildVersionString() << QSslSocket::supportsSsl();

    connect(ui->btn_recheck, &QPushButton::clicked, this, &UpdateDialog::fetchGithubReleaseInfo);
    connect(ui->btn_update, &QPushButton::clicked, this, [this] {
        ui->btn_update->setEnabled(false);
        auto url = relInfo.downloadUrl;
        download(url, qApp->applicationDirPath() + "/" + QUrl(url).fileName());
    });
    connect(this, &UpdateDialog::downloadSucceed, this, [this](const QString& filePath) {
        qDebug() << "Download succeed" << filePath;
    });
}

UpdateDialog::~UpdateDialog() {
    delete ui;
}

void UpdateDialog::fetchGithubReleaseInfo() {
    ui->textBrowser->setMarkdown(R"(## Fetching...)");
    const QString ApiUrl = QString("https://api.github.com/repos/%1/%2/releases/latest").arg(Owner, Repo);
    QNetworkRequest request(ApiUrl);
    auto* reply = manager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Failed to fetch update info" << reply->errorString();
            sysTray.showMessage("Failed to fetch update info", reply->errorString());
            return;
        }
        const auto data = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const auto obj = doc.object();

        relInfo.ver = normalizeVersion(obj["tag_name"].toString());
        relInfo.description = obj["body"].toString();
        relInfo.publishTime = toLocalTime(obj["published_at"].toString());

        if (const auto assets = obj["assets"].toArray(); !assets.isEmpty())
            relInfo.downloadUrl = assets.first()["browser_download_url"].toString();

        qDebug() << "Update info fetched" << relInfo.ver << relInfo.downloadUrl;
        ui->label_newVer->setText(QString("New Version: v%1 (%2)").arg(relInfo.ver.toString(), relInfo.publishTime));
        ui->label_ver->setText(QString("Current Version: v%1").arg(version.toString()));

        bool needUpdate = relInfo.ver > version;
        // 如果要支持网络图片，必须重写`QTextBrowser::loadResource`，默认是作为本地文件处理
        ui->textBrowser->setMarkdown(needUpdate ? relInfo.description : "# ✅Everything is up-to-date");
        ui->btn_update->setEnabled(needUpdate);
    });
}

void UpdateDialog::download(const QString& url, const QString& savePath) {
    QNetworkRequest request(url);
    auto* reply = manager.get(request);
    ui->progressBar->show();
    ui->progressBar->setValue(0);

    QFile::remove(savePath);
    downloadStatus.file.setFileName(savePath);
    downloadStatus.file.open(QIODevice::WriteOnly | QIODevice::Append);
    downloadStatus.success = false;
    downloadStatus.reply = reply;

    connect(reply, &QNetworkReply::readyRead, this, [this, reply] {
        downloadStatus.file.write(reply->readAll());
    });

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesReceived == bytesTotal)
            downloadStatus.success = true;

        ui->progressBar->setMaximum(bytesTotal);
        ui->progressBar->setValue(bytesReceived);
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (!downloadStatus.success || reply->error() != QNetworkReply::NoError) {
            qWarning() << "Download failed" << reply->errorString();
            ui->textBrowser->setMarkdown("## Download failed\n" + reply->errorString());
            sysTray.showMessage("Download failed", reply->errorString());
        } else {
            ui->textBrowser->setMarkdown("## Download success✅");
            sysTray.showMessage("Download Status", "Success!");
            emit downloadSucceed(downloadStatus.file.fileName());
        }
        downloadStatus.reply = nullptr;
        downloadStatus.file.close();
        ui->progressBar->hide();
        ui->btn_update->setEnabled(true);
    });
}

QVersionNumber UpdateDialog::normalizeVersion(const QString& ver) {
    auto v = ver;
    if (v.startsWith('v')) v.removeFirst();
    while (v.count('.') > 2 && v.endsWith(".0")) // 移除多余的0，保留3位
        v.chop(2);
    return QVersionNumber::fromString(v);
}

QString UpdateDialog::toLocalTime(const QString& isoTime) {
    const auto dateTime = QDateTime::fromString(isoTime, Qt::ISODate);
    return dateTime.toLocalTime().toString("yyyy.MM.dd HH:mm");
}

void UpdateDialog::showEvent(QShowEvent*) {
    ui->progressBar->hide();
    fetchGithubReleaseInfo();
}
