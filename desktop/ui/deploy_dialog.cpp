#include "deploy_dialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

DeployDialog::DeployDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Установка на сервер");
    setMinimumWidth(350);

    auto *layout = new QVBoxLayout(this);

    auto *info = new QLabel("Введите данные для подключения к VPS по SSH:");
    info->setWordWrap(true);
    layout->addWidget(info);

    auto *form = new QFormLayout();

    m_hostEdit = new QLineEdit();
    m_hostEdit->setPlaceholderText("45.67.89.123 или my-vps.com");
    form->addRow("Хост:", m_hostEdit);

    m_userEdit = new QLineEdit();
    m_userEdit->setPlaceholderText("root");
    form->addRow("Пользователь:", m_userEdit);

    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("••••••••");
    form->addRow("Пароль:", m_passwordEdit);

    layout->addLayout(form);

    auto *buttons = new QHBoxLayout();
    auto *deployBtn = new QPushButton("Установить");
    auto *cancelBtn = new QPushButton("Отмена");

    connect(deployBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    buttons->addStretch();
    buttons->addWidget(cancelBtn);
    buttons->addWidget(deployBtn);

    layout->addLayout(buttons);
}

QString DeployDialog::host() const { return m_hostEdit->text(); }
QString DeployDialog::user() const { return m_userEdit->text(); }
QString DeployDialog::password() const { return m_passwordEdit->text(); }