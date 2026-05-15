#include "add_server_page.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

AddServerPage::AddServerPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    auto *title = new QLabel("Добавить сервер");
    title->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 8px;");
    layout->addWidget(title);

    auto *desc = new QLabel("Введите данные VPS для установки серверной части по SSH.");
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #666; margin-bottom: 16px;");
    layout->addWidget(desc);

    auto *formGroup = new QGroupBox();
    auto *form = new QFormLayout(formGroup);

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

    layout->addWidget(formGroup);

    m_deployBtn = new QPushButton("Установить");
    m_deployBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px; "
                               "border: none; border-radius: 4px; font-size: 14px; } "
                               "QPushButton:hover { background-color: #45A049; }");
    connect(m_deployBtn, &QPushButton::clicked, this, &AddServerPage::onDeploy);
    layout->addWidget(m_deployBtn);

    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #999; margin-top: 8px;");
    layout->addWidget(m_statusLabel);

    layout->addStretch();
}

void AddServerPage::onDeploy()
{
    QString host = m_hostEdit->text();
    QString user = m_userEdit->text();
    QString password = m_passwordEdit->text();

    if (host.isEmpty() || user.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("Заполните все поля.");
        return;
    }

    m_statusLabel->setText("Подключение к " + host + "...");
    m_deployBtn->setEnabled(false);

    // TODO: вызов SshDeployer из yeti-core
    emit serverAdded(host, user, password);
}