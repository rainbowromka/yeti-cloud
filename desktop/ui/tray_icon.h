#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QWebSocket>
#include <QTimer>

class DeployDialog;

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon();

    void show();

private slots:
    void onDeployAction();
    void onConnectToServer();
    void onDisconnected();
    void onStatusCheck();
    void onExit();

private:
    void updateIcon(bool connected);
    void connectToServer(const QString &url);

    QSystemTrayIcon *m_tray;
    QMenu *m_menu;
    QAction *m_deployAction;
    QAction *m_statusAction;
    QAction *m_exitAction;

    QWebSocket *m_webSocket;
    QTimer *m_statusTimer;

    QString m_serverUrl;
    bool m_connected = false;
};