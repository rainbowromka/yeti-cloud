#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QMenu>

class AddServerPage;
class StatusPage;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void navigateTo(int index);

private slots:
    void onMenuButton();
    void onAddServer();
    void onOpenFolder();

private:
    void setupUi();

    QStackedWidget *m_stack;

    QLabel *m_titleLabel;
    QPushButton *m_homeBtn;
    QPushButton *m_folderBtn;
    QPushButton *m_menuBtn;

    AddServerPage *m_addServerPage;
    StatusPage *m_statusPage;
};