#pragma once

#include <QObject>
#include <QString>

class LogBuffer;
class HttpClient;
class ConfigManager;
class QWebSocket;
class QTimer;

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject* parent = nullptr);
    ~Client();

    bool isConnected() const;

public slots:
    void start();
    void connectToServer(const QString& host, int port,
                         const QString& adminKey, const QString& deviceName);
    void disconnectFromServer();

signals:
    void statusChanged(const QString& status);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

private slots:
    void onPingTimeout();
    void tryReconnect();

private:
    void connectWebSocket(const QString& host, int port,
                          const QString& deviceKey, const QString& deviceId);

    LogBuffer* m_log;
    ConfigManager* m_cfg;
    HttpClient* m_httpClient;
    QWebSocket* m_webSocket;
    QTimer* m_pingTimer;
    QTimer* m_reconnectTimer;

    QString m_host;
    int m_port = 0;
    QString m_deviceKey;
    QString m_deviceId;
    bool m_connecting = false;
    bool m_intentionalDisconnect = false;
    int m_reconnectAttempts = 0;
    static const int MAX_RECONNECT_ATTEMPTS = 5;
};