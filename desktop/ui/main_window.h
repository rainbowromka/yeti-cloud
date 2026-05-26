#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QMenu>

class AddServerPage;
class StatusPage;
class TrayIcon;
class Client;
class ConsoleWidget;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(Client* client, TrayIcon* trayIcon, QWidget* parent = nullptr);

    void navigateTo(int index);

private slots:
    void onMenuButton();
    void onAddServer();
    void onOpenFolder();
    void onServerAdded(const QString& host, const QString& adminKey);

private:
    void setupUi();

    Client* m_client;
    TrayIcon* m_trayIcon;
    QStackedWidget* m_stack;

    QLabel* m_titleLabel;
    QPushButton* m_homeBtn;
    QPushButton* m_folderBtn;
    QPushButton* m_menuBtn;

    AddServerPage* m_addServerPage;
    StatusPage* m_statusPage;
    ConsoleWidget* m_console;
};