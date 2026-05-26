#include "add_server_page.h"
#include "ssh_deployer.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QIntValidator>
#include <QtConcurrent/QtConcurrent>

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
    m_hostEdit->setText("localhost");
    form->addRow("Хост:", m_hostEdit);

    m_portEdit = new QLineEdit();
    m_portEdit->setPlaceholderText("22");
    m_portEdit->setText("2222");
    m_portEdit->setValidator(new QIntValidator(1, 65535, this));
    form->addRow("Порт:", m_portEdit);

    m_userEdit = new QLineEdit();
    m_userEdit->setPlaceholderText("root");
    m_userEdit->setText("root");
    form->addRow("Пользователь:", m_userEdit);

    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("••••••••");
    m_passwordEdit->setText("test123");
    form->addRow("Пароль:", m_passwordEdit);

    layout->addWidget(formGroup);

    m_deployBtn = new QPushButton("Установить");
    m_deployBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px; "
                               "border: none; border-radius: 4px; font-size: 14px; } "
                               "QPushButton:hover { background-color: #45A049; }");
    connect(m_deployBtn, &QPushButton::clicked, this, &AddServerPage::onDeploy);
    layout->addWidget(m_deployBtn);

    layout->addStretch();
}

void AddServerPage::onDeploy()
{
    QString host = m_hostEdit->text();
    QString user = m_userEdit->text();
    QString password = m_passwordEdit->text();
    int port = m_portEdit->text().toInt();

    if (host.isEmpty() || user.isEmpty() || password.isEmpty()) {
        return;
    }

    m_deployBtn->setEnabled(false);

    auto config = new SshDeployer::Config;
    config->host = host.toStdString();
    config->user = user.toStdString();
    config->password = password.toStdString();
    config->port = port;

    auto *deployer = new SshDeployer(*config);

    auto *watcher = new QFutureWatcher<bool>(this);
    QObject::connect(watcher, &QFutureWatcher<bool>::finished, this, [this, deployer, watcher, config]() {
        bool ok = watcher->result();
        if (ok) {
            emit serverAdded(
                QString::fromStdString(config->host),
                QString::fromStdString(deployer->adminKey()));
        }
        m_deployBtn->setEnabled(true);
        delete deployer;
        delete config;
        watcher->deleteLater();
    });

    QFuture<bool> future = QtConcurrent::run([deployer]() {
        return deployer->deploy();
    });

    watcher->setFuture(future);
}