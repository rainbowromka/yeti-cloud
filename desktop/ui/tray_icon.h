#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class MainWindow;

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(MainWindow *mainWindow, QObject *parent = nullptr);
    ~TrayIcon();

    void show();

private slots:
    void onShowApp();
    void onStopService();
    void onStartService();
    void onQuit();

private:
    void updateIcon(bool connected);

    QSystemTrayIcon *m_tray;
    QMenu *m_menu;
    MainWindow *m_mainWindow;

    bool m_connected = false;
};