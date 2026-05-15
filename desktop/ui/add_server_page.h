#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class AddServerPage : public QWidget
{
    Q_OBJECT

public:
    explicit AddServerPage(QWidget *parent = nullptr);

signals:
    void serverAdded(const QString &host, const QString &user, const QString &password);

private slots:
    void onDeploy();

private:
    QLineEdit *m_hostEdit;
    QLineEdit *m_userEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_portEdit;
    QPushButton *m_deployBtn;
    QLabel *m_statusLabel;
};