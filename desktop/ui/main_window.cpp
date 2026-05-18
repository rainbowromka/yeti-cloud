#include "main_window.h"
#include "add_server_page.h"
#include "status_page.h"
#include "tray_icon.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMessageBox>

MainWindow::MainWindow(TrayIcon *trayIcon, QWidget *parent)
    : QWidget(parent), m_trayIcon(trayIcon)
{
    setWindowTitle("Yeti Cloud");
    setFixedSize(360, 600);
    setupUi();
}

void MainWindow::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- Заголовок ---
    auto *header = new QWidget();
    header->setFixedHeight(48);
    header->setStyleSheet("background-color: #2C3E50; color: white;");

    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(8, 4, 8, 4);

    m_homeBtn = new QPushButton("⌂");
    m_homeBtn->setFixedSize(32, 32);
    m_homeBtn->setStyleSheet("border: none; font-size: 18px; color: white;");
    connect(m_homeBtn, &QPushButton::clicked, this, [this]() { navigateTo(0); });

    m_titleLabel = new QLabel("Yeti Cloud");
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: white;");

    m_folderBtn = new QPushButton("📁");
    m_folderBtn->setFixedSize(32, 32);
    m_folderBtn->setStyleSheet("border: none; font-size: 16px; color: white;");
    m_folderBtn->setToolTip("Открыть папку облака");
    connect(m_folderBtn, &QPushButton::clicked, this, &MainWindow::onOpenFolder);

    m_menuBtn = new QPushButton("⋮");
    m_menuBtn->setFixedSize(32, 32);
    m_menuBtn->setStyleSheet("border: none; font-size: 18px; font-weight: bold; color: white;");
    connect(m_menuBtn, &QPushButton::clicked, this, &MainWindow::onMenuButton);

    headerLayout->addWidget(m_homeBtn);
    headerLayout->addWidget(m_titleLabel, 1);
    headerLayout->addWidget(m_folderBtn);
    headerLayout->addWidget(m_menuBtn);

    mainLayout->addWidget(header);

    // --- Контент (страницы) ---
    m_stack = new QStackedWidget();

    m_statusPage = new StatusPage();
    m_addServerPage = new AddServerPage();

    connect(m_addServerPage, &AddServerPage::serverAdded, this, &MainWindow::onServerAdded);

    m_stack->addWidget(m_statusPage);    // index 0 — статус
    m_stack->addWidget(m_addServerPage); // index 1 — добавить сервер

    mainLayout->addWidget(m_stack, 1);
}

void MainWindow::navigateTo(int index)
{
    m_stack->setCurrentIndex(index);
}

void MainWindow::onMenuButton()
{
    QMenu menu(this);

    QAction *statusAction = menu.addAction("Статус");
    connect(statusAction, &QAction::triggered, this, [this]() { navigateTo(0); });

    QAction *addAction = menu.addAction("Добавить сервер");
    connect(addAction, &QAction::triggered, this, &MainWindow::onAddServer);

    menu.exec(m_menuBtn->mapToGlobal(QPoint(0, m_menuBtn->height())));
}

void MainWindow::onAddServer()
{
    navigateTo(1);
}

void MainWindow::onOpenFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::homePath() + "/YetiCloud"));
}

void MainWindow::onServerAdded(const QString &host, const QString &user, const QString &password)
{
    Q_UNUSED(user)
    Q_UNUSED(password)

    QString url = "ws://" + host + ":8080/ws";
    m_trayIcon->connectToServer(url);
    navigateTo(0);
}