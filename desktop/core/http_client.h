#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <QObject>
#include <QString>
#include <functional>

class QNetworkAccessManager;

class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QObject* parent = nullptr);

    void connectDevice(const QString& serverHost, int serverPort,
                       const QString& adminKey, const QString& deviceName,
                       std::function<void(const QString& step)> onStep,
                       std::function<void(const QString& deviceKey, const QString& deviceId)> onDone,
                       std::function<void(const QString& error)> onError);

private:
    QNetworkAccessManager* m_nam;
};

#endif // HTTP_CLIENT_H