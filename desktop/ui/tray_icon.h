#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

class MainWindow;

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(MainWindow* mainWindow, QObject* parent = nullptr);
    ~TrayIcon();

    void show();
    void setMainWindow(MainWindow* mainWindow);
    void updateIcon(bool connected);

signals:
    void connectionStatusChanged(bool connected);

private slots:
    void onShowApp();
    void onStopService();
    void onStartService();
    void onQuit();

private:
    MainWindow* m_mainWindow;
    QMenu* m_menu;
    QSystemTrayIcon* m_tray;
};