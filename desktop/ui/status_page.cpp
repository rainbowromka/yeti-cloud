#include "status_page.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QFont>

StatusPage::StatusPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    // --- Заголовок ---
    auto *title = new QLabel("Статус подключения");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2C3E50;");
    layout->addWidget(title);

    // --- Разделитель ---
    auto *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #DDD;");
    layout->addWidget(sep1);

    // --- Строка: иконка + статус ---
    auto *connRow = new QHBoxLayout();
    m_connectionIcon = new QLabel("●");
    m_connectionIcon->setStyleSheet("font-size: 20px; color: #999;");
    m_connectionLabel = new QLabel("Нет подключения");
    m_connectionLabel->setStyleSheet("font-size: 14px; color: #666;");
    connRow->addWidget(m_connectionIcon);
    connRow->addWidget(m_connectionLabel, 1);
    layout->addLayout(connRow);

    // --- Сервер ---
    m_serverLabel = new QLabel("Сервер: не указан");
    m_serverLabel->setStyleSheet("font-size: 13px; color: #888;");
    layout->addWidget(m_serverLabel);

    // --- Разделитель ---
    auto *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #DDD;");
    layout->addWidget(sep2);

    // --- Конфигурация ---
    auto *cfgTitle = new QLabel("Конфигурация");
    cfgTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #2C3E50;");
    layout->addWidget(cfgTitle);

    m_configLabel = new QLabel("Файл конфига: не найден");
    m_configLabel->setStyleSheet("font-size: 13px; color: #888;");
    layout->addWidget(m_configLabel);

    m_keyLabel = new QLabel("Ключ: —");
    m_keyLabel->setStyleSheet("font-size: 12px; color: #AAA; font-family: monospace;");
    m_keyLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_keyLabel);

    layout->addStretch();
}

void StatusPage::setServerAddress(const QString &address)
{
    m_serverLabel->setText("Сервер: " + address);
}

void StatusPage::setConnected(bool connected)
{
    if (connected) {
        m_connectionIcon->setStyleSheet("font-size: 20px; color: #4CAF50;");
        m_connectionLabel->setText("Подключено");
        m_connectionLabel->setStyleSheet("font-size: 14px; color: #4CAF50; font-weight: bold;");
    } else {
        m_connectionIcon->setStyleSheet("font-size: 20px; color: #E74C3C;");
        m_connectionLabel->setText("Нет подключения");
        m_connectionLabel->setStyleSheet("font-size: 14px; color: #E74C3C;");
    }
}

void StatusPage::setConfigPresent(bool present)
{
    if (present) {
        m_configLabel->setText("Файл конфига: найден");
        m_configLabel->setStyleSheet("font-size: 13px; color: #4CAF50;");
    } else {
        m_configLabel->setText("Файл конфига: не найден");
        m_configLabel->setStyleSheet("font-size: 13px; color: #E74C3C;");
    }
}

void StatusPage::setAdminKey(const QString &key)
{
    if (key.isEmpty()) {
        m_keyLabel->setText("Ключ: —");
    } else {
        m_keyLabel->setText("Ключ: " + key.left(16) + "...");
        m_keyLabel->setToolTip(key);
    }
}