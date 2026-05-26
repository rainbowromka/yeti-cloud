#include "http_client.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

HttpClient::HttpClient(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_nam->setTransferTimeout(10000);
    qDebug() << "HttpClient created";
}

void HttpClient::connectDevice(const QString& serverHost, int serverPort,
                                const QString& adminKey, const QString& deviceName,
                                std::function<void(const QString&)> onStep,
                                std::function<void(const QString&, const QString&)> onDone,
                                std::function<void(const QString&)> onError)
{
    // Шаг 1: POST /api/invite
    onStep("Creating invite token...");

    QUrl url(QString("http://%1:%2/api/invite").arg(serverHost).arg(serverPort));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(adminKey).toUtf8());

    QNetworkReply* reply = m_nam->post(request, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            onError("Invite failed: " + reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QString inviteToken = doc.object().value("invite_token").toString();
        if (inviteToken.isEmpty()) {
            onError("Invite failed: no token in response");
            return;
        }

        // Шаг 2: POST /api/register
        onStep("Registering device...");

        QUrl url2(QString("http://%1:%2/api/register").arg(serverHost).arg(serverPort));
        QNetworkRequest request2(url2);
        request2.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject body;
        body["invite_token"] = inviteToken;
        body["device_name"] = deviceName;

        QNetworkReply* reply2 = m_nam->post(request2, QJsonDocument(body).toJson());

        connect(reply2, &QNetworkReply::finished, this, [=]() {
            reply2->deleteLater();

            if (reply2->error() != QNetworkReply::NoError) {
                onError("Register failed: " + reply2->errorString());
                return;
            }

            QJsonDocument doc2 = QJsonDocument::fromJson(reply2->readAll());
            QString deviceKey = doc2.object().value("device_key").toString();
            QString deviceId = doc2.object().value("device_id").toString();

            if (deviceKey.isEmpty() || deviceId.isEmpty()) {
                onError("Register failed: no device_key or device_id");
                return;
            }

            onDone(deviceKey, deviceId);
        });
    });
}