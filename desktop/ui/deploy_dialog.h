#pragma once

#include <QDialog>
#include <QLineEdit>

class DeployDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeployDialog(QWidget *parent = nullptr);

    QString host() const;
    QString user() const;
    QString password() const;

private:
    QLineEdit *m_hostEdit;
    QLineEdit *m_userEdit;
    QLineEdit *m_passwordEdit;
};