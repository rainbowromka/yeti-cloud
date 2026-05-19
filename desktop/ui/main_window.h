#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QMenu>

class AddServerPage;
class StatusPage;
class TrayIcon;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(TrayIcon *trayIcon, QWidget *parent = nullptr);

    void navigateTo(int index);

private slots:
    void onMenuButton();
    void onAddServer();
    void onOpenFolder();
    void onServerAdded(const QString &host, const QString &adminKey);
    void onConnectionStatusChanged(bool connected);

private:
    void setupUi();

    TrayIcon *m_trayIcon;
    QStackedWidget *m_stack;

    QLabel *m_titleLabel;
    QPushButton *m_homeBtn;
    QPushButton *m_folderBtn;
    QPushButton *m_menuBtn;

    AddServerPage *m_addServerPage;
    StatusPage *m_statusPage;
};