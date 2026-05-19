#include "tray_icon.h"
#include "main_window.h"

#include <QApplication>
#include <QStyle>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>

TrayIcon::TrayIcon(MainWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    m_menu = new QMenu();

    QAction *showAction = m_menu->addAction("Показать приложение");
    connect(showAction, &QAction::triggered, this, &TrayIcon::onShowApp);

    m_menu->addSeparator();

    QAction *stopAction = m_menu->addAction("Остановить сервис");
    connect(stopAction, &QAction::triggered, this, &TrayIcon::onStopService);

    QAction *startAction = m_menu->addAction("Запустить сервис");
    connect(startAction, &QAction::triggered, this, &TrayIcon::onStartService);

    m_menu->addSeparator();

    QAction *quitAction = m_menu->addAction("Закрыть приложение");
    connect(quitAction, &QAction::triggered, this, &TrayIcon::onQuit);

    m_tray = new QSystemTrayIcon(this);
    m_tray->setContextMenu(m_menu);
    m_tray->setToolTip("Yeti Cloud");

    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            onShowApp();
        }
    });

    updateIcon(false);

    // WebSocket для проверки статуса
    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(m_webSocket, &QWebSocket::connected, this, &TrayIcon::onConnected);
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

void TrayIcon::setMainWindow(MainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
}

void TrayIcon::connectToServer(const QString &url)
{
    m_serverUrl = url;
    m_webSocket->open(QUrl(m_serverUrl));
}

void TrayIcon::onShowApp()
{
    if (m_mainWindow) {
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
}

void TrayIcon::onStopService()
{
    m_tray->showMessage("Yeti Cloud", "Сервис остановлен");
}

void TrayIcon::onStartService()
{
    m_tray->showMessage("Yeti Cloud", "Сервис запущен");
}

void TrayIcon::onQuit()
{
    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }
    QApplication::quit();
}

void TrayIcon::onConnected()
{
    m_connected = true;
    updateIcon(true);
    m_tray->setToolTip("Yeti Cloud — онлайн");
    m_tray->showMessage("Yeti Cloud", "Подключено к серверу");
    emit connectionStatusChanged(true);
}

void TrayIcon::onDisconnected()
{
    m_connected = false;
    updateIcon(false);
    m_tray->setToolTip("Yeti Cloud — оффлайн");
    emit connectionStatusChanged(false);
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

void TrayIcon::updateIcon(bool connected)
{
    QIcon icon = QApplication::style()->standardIcon(
        connected ? QStyle::SP_ComputerIcon : QStyle::SP_MessageBoxWarning);
    m_tray->setIcon(icon);
}