#pragma once

#include <QWidget>
#include <QLabel>

class StatusPage : public QWidget
{
    Q_OBJECT

public:
    explicit StatusPage(QWidget *parent = nullptr);

    void setServerAddress(const QString &address);
    void setConnected(bool connected);
    void setConfigPresent(bool present);
    void setAdminKey(const QString &key);
    void setStatusText(const QString &text);

private:
    QLabel *m_connectionIcon;
    QLabel *m_connectionLabel;
    QLabel *m_serverLabel;
    QLabel *m_configLabel;
    QLabel *m_keyLabel;
    QLabel *m_statusLabel;
};