#include <thoth/TTSDownloader.h>

#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

TTSDownloader::TTSDownloader(QObject* parent)
    : QObject(parent), m_manager(new QNetworkAccessManager(this)) {}

void TTSDownloader::download(const std::string& text, DownloadCallback callback) {
    QUrl url("https://translate.google.com/translate_tts");
    QUrlQuery query;
    query.addQueryItem("ie", "UTF-8");
    query.addQueryItem("tl", "en");
    query.addQueryItem("client", "tw-ob");
    query.addQueryItem("q", QString::fromStdString(text));
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply* reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            callback(true, std::move(data));
        } else {
            callback(false, QByteArray());
        }
    });
}