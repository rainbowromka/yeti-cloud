#include "tray_icon.h"
#include "deploy_dialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyle>
#include <QJsonDocument>
#include <QJsonObject>

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
{
    m_menu = new QMenu();

    m_statusAction = m_menu->addAction("Сервер: проверка...");
    m_statusAction->setEnabled(false);

    m_deployAction = m_menu->addAction("Установить на сервер...");
    connect(m_deployAction, &QAction::triggered, this, &TrayIcon::onDeployAction);

    m_menu->addSeparator();

    m_exitAction = m_menu->addAction("Выход");
    connect(m_exitAction, &QAction::triggered, this, &TrayIcon::onExit);

    m_tray = new QSystemTrayIcon(this);
    m_tray->setContextMenu(m_menu);
    m_tray->setToolTip("Yeti Cloud");

    updateIcon(false);

    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(m_webSocket, &QWebSocket::connected, this, [this]() {
        m_connected = true;
        updateIcon(true);
        m_statusAction->setText("Сервер: онлайн");
    });
    connect(m_webSocket, &QWebSocket::disconnected, this, &TrayIcon::onDisconnected);

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &TrayIcon::onStatusCheck);
    m_statusTimer->start(10000);
}

TrayIcon::~TrayIcon()
{
    delete m_menu;
}

void TrayIcon::show()
{
    m_tray->show();
}

void TrayIcon::onDeployAction()
{
    DeployDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        QString host = dialog.host();
        QString user = dialog.user();
        QString password = dialog.password();

        m_tray->showMessage("Yeti Cloud", "Подключение к " + host + "...");

        m_serverUrl = "ws://" + host + ":8080/ws";
        connectToServer(m_serverUrl);
    }
}

void TrayIcon::onConnectToServer()
{
    if (!m_serverUrl.isEmpty()) {
        m_webSocket->open(QUrl(m_serverUrl));
    }
}

void TrayIcon::onDisconnected()
{
    m_connected = false;
    updateIcon(false);
    m_statusAction->setText("Сервер: оффлайн");
}

void TrayIcon::onStatusCheck()
{
    if (!m_serverUrl.isEmpty() && m_webSocket->state() == QAbstractSocket::UnconnectedState) {
        connectToServer(m_serverUrl);
    }

    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->ping();
    }
}

void TrayIcon::onExit()
{
    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }
    QApplication::quit();
}

void TrayIcon::updateIcon(bool connected)
{
    QIcon icon = QApplication::style()->standardIcon(
        connected ? QStyle::SP_ComputerIcon : QStyle::SP_MessageBoxWarning);
    m_tray->setIcon(icon);
}

void TrayIcon::connectToServer(const QString &url)
{
    m_webSocket->open(QUrl(url));
}