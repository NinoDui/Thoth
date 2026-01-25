#include <thoth/TTSDownloader.h>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QFile>

TTSDownloader::TTSDownloader(QObject* parent)
    : QObject(parent), m_manager(new QNetworkAccessManager(this)) {
}

void TTSDownloader::download(const std::string& text, const std::filesystem::path& dst, DownloadCallback callback) {
    QUrl url("https://translate.google.com/translate_tts");
    QUrlQuery query;
    query.addQueryItem("ie", "UTF-8");
    query.addQueryItem("tl", "en");
    query.addQueryItem("client", "tw-ob");
    query.addQueryItem("q", QString::fromStdString(text));
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply* reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [reply, dst, callback]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            callback(false, dst);
            return;
        }

        QFile file(QString::fromStdString(dst.string()));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            callback(true, dst);
        } else {
            callback(false, dst);
        }
    });
}