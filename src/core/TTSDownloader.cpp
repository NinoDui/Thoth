#include <qdebug.h>
#include <thoth/Q_TTSDownloader.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <iostream>

#include "Entity.h"

TTSDownloader::TTSDownloader(QObject* parent)
    : QObject(parent), m_qNetworkManager(new QNetworkAccessManager(this)) {}

TTSDownloader::TTSDownloader(QNetworkAccessManager* qNetworkManager, QObject* parent)
    : QObject(parent), m_qNetworkManager(qNetworkManager) {}

void TTSDownloader::download(const std::string& text, DownloadCallback callback) {
    // TODO: separate code v.s. config, use a config file to store the URL
    QUrl url("https://texttospeech.googleapis.com/v1/text:synthesize");
    QUrlQuery query;
    query.addQueryItem("key", "AIzaSyDVIeM6NWJUfaSpBMuK0vGFaVP1mWGfgoA");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QTTSRequest reqBody(QString::fromStdString(text));

    QNetworkReply* reply = m_qNetworkManager->post(request, reqBody.toPayload());
    std::cout << "reply->error(): " << reply->error() << std::endl;
    std::cout << "reply->errorString(): " << reply->errorString().toStdString() << std::endl;

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