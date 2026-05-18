#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QWebSocket>
#include <QTimer>

class MainWindow;

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(MainWindow *mainWindow, QObject *parent = nullptr);
    ~TrayIcon();

    void show();
    void setMainWindow(MainWindow *mainWindow);
    void connectToServer(const QString &url);

private slots:
    void onShowApp();
    void onStopService();
    void onStartService();
    void onQuit();
    void onConnected();
    void onDisconnected();
    void onStatusCheck();

private:
    void updateIcon(bool connected);

    QSystemTrayIcon *m_tray;
    QMenu *m_menu;
    MainWindow *m_mainWindow;

    QWebSocket *m_webSocket;
    QTimer *m_statusTimer;

    QString m_serverUrl;
    bool m_connected = false;
};