#include "client.h"
#include "log_buffer.h"
#include "http_client.h"
#include "config_manager.h"

#include <QWebSocket>
#include <QTimer>
#include <QUrl>

Client::Client(QObject* parent)
    : QObject(parent)
    , m_log(LogBuffer::instance())
    , m_cfg(&ConfigManager::instance())
    , m_httpClient(new HttpClient(this))
    , m_webSocket(nullptr)
    , m_pingTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
{
    m_pingTimer->setInterval(30000);
    connect(m_pingTimer, &QTimer::timeout, this, &Client::onPingTimeout);

    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &Client::tryReconnect);

    m_log->add("Client created");
}

Client::~Client()
{
    m_log->add("Client destroyed");
}

bool Client::isConnected() const
{
    return m_webSocket && m_webSocket->state() == QAbstractSocket::ConnectedState;
}

// ─── Слоты ───

void Client::start()
{
    m_log->add("Client starting...");
    m_cfg->load();

    if (!m_cfg->hasConfig()) {
        m_log->add("No config found, waiting for UI");
        return;
    }

    QString host = m_cfg->getServerHost();
    int port = m_cfg->getServerPort();
    QString adminKey = m_cfg->getAdminKey();
    QString deviceKey = m_cfg->getDeviceKey();
    QString deviceId = m_cfg->getDeviceId();

    if (!deviceKey.isEmpty() && !deviceId.isEmpty()) {
        m_host = host;
        m_port = port;
        m_deviceKey = deviceKey;
        m_deviceId = deviceId;
        m_log->add("Found device credentials, connecting WebSocket");
        connectWebSocket(host, port, deviceKey, deviceId);
    } else if (!adminKey.isEmpty()) {
        m_log->add("Found admin key, starting HTTP registration");
        connectToServer(host, port, adminKey, "Windows Desktop");
    } else {
        m_log->add("Config found but no credentials");
    }
}

void Client::connectToServer(const QString& host, int port,
                              const QString& adminKey, const QString& deviceName)
{
    if (m_connecting) {
        m_log->add("Already connecting, ignoring");
        return;
    }
    m_connecting = true;
    m_intentionalDisconnect = false;

    m_host = host;
    m_port = port;

    m_log->add("Connecting to " + host.toStdString() + ":" + std::to_string(port));

    m_httpClient->connectDevice(host, port, adminKey, deviceName,
        [this](const QString& step) {
            m_log->add(step.toStdString());
            emit statusChanged(step);
        },
        [this](const QString& deviceKey, const QString& deviceId) {
            m_deviceKey = deviceKey;
            m_deviceId = deviceId;

            m_log->add("Device registered: " + deviceId.toStdString());

            m_cfg->setDeviceKey(deviceKey);
            m_cfg->setDeviceId(deviceId);
            m_cfg->save();

            connectWebSocket(m_host, m_port, deviceKey, deviceId);
        },
        [this](const QString& error) {
            m_log->add("ERROR: " + error.toStdString());
            m_connecting = false;
            emit errorOccurred(error);
        }
    );
}

void Client::disconnectFromServer()
{
    m_log->add("Disconnecting...");
    m_intentionalDisconnect = true;
    m_connecting = false;
    m_reconnectTimer->stop();
    m_pingTimer->stop();
    m_reconnectAttempts = 0;

    if (m_webSocket) {
        if (m_webSocket->state() == QAbstractSocket::ConnectedState ||
            m_webSocket->state() == QAbstractSocket::ConnectingState) {
            m_webSocket->close();
        }
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    emit disconnected();
}

// ─── Private ───

void Client::connectWebSocket(const QString& host, int port,
                               const QString& deviceKey, const QString& deviceId)
{
    if (m_webSocket) {
        m_webSocket->deleteLater();
    }

    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    connect(m_webSocket, &QWebSocket::connected, this, [this]() {
        m_log->add("WebSocket connected");
        m_connecting = false;
        m_reconnectAttempts = 0;
        m_pingTimer->start();
        emit connected();
    });

    connect(m_webSocket, &QWebSocket::disconnected, this, [this]() {
        m_log->add("WebSocket disconnected");
        m_pingTimer->stop();

        if (!m_intentionalDisconnect && m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
            int delay = (m_reconnectAttempts == 0) ? 5000 : 10000;
            m_log->add("Reconnecting in " + std::to_string(delay / 1000) + "s...");
            m_reconnectTimer->start(delay);
        } else if (!m_intentionalDisconnect) {
            m_log->add("Max reconnect attempts reached");
            m_connecting = false;
            emit errorOccurred("Connection lost");
        }

        emit disconnected();
    });

    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this](QAbstractSocket::SocketError) {
        m_log->add("WebSocket error: " + m_webSocket->errorString().toStdString());
    });

    connect(m_webSocket, &QWebSocket::textMessageReceived, this, [this](const QString& msg) {
        m_log->add("WS message: " + msg.toStdString());
    });

    connect(m_webSocket, &QWebSocket::pong, this, [](quint64, const QByteArray&) {
        // pong received — connection alive
    });

    QString url = QString("ws://%1:%2/ws?token=%3&device_id=%4")
                  .arg(host).arg(port).arg(deviceKey).arg(deviceId);
    m_log->add("Opening WebSocket: " + url.toStdString());
    m_webSocket->open(QUrl(url));
}

void Client::onPingTimeout()
{
    if (m_webSocket && m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->ping();
    }
}

void Client::tryReconnect()
{
    if (m_intentionalDisconnect) {
        return;
    }

    m_reconnectAttempts++;
    m_log->add("Reconnect attempt " + std::to_string(m_reconnectAttempts));

    if (!m_deviceKey.isEmpty() && !m_deviceId.isEmpty()) {
        connectWebSocket(m_host, m_port, m_deviceKey, m_deviceId);
    } else {
        m_log->add("No device credentials, cannot reconnect");
        m_connecting = false;
    }
}