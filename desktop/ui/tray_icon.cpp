#include "tray_icon.h"
#include "main_window.h"

#include <QApplication>
#include <QStyle>

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
        if (reason == QSystemTrayIcon::Trigger) { // левый клик
            onShowApp();
        }
    });

    updateIcon(false);
}

TrayIcon::~TrayIcon()
{
    delete m_menu;
}

void TrayIcon::show()
{
    m_tray->show();
}

void TrayIcon::onShowApp()
{
    m_mainWindow->show();
    m_mainWindow->raise();
    m_mainWindow->activateWindow();
}

void TrayIcon::onStopService()
{
    // TODO: остановка сервиса синхронизации
    m_tray->showMessage("Yeti Cloud", "Сервис остановлен");
}

void TrayIcon::onStartService()
{
    // TODO: запуск сервиса синхронизации
    m_tray->showMessage("Yeti Cloud", "Сервис запущен");
}

void TrayIcon::onQuit()
{
    QApplication::quit();
}

void TrayIcon::updateIcon(bool connected)
{
    QIcon icon = QApplication::style()->standardIcon(
        connected ? QStyle::SP_ComputerIcon : QStyle::SP_MessageBoxWarning);
    m_tray->setIcon(icon);
}