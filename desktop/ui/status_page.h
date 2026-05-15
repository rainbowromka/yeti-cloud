#pragma once

#include <QWidget>
#include <QLabel>

class StatusPage : public QWidget
{
    Q_OBJECT

public:
    explicit StatusPage(QWidget *parent = nullptr);

    void setStatus(const QString &text);

private:
    QLabel *m_statusLabel;
};