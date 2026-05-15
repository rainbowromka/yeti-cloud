#include "status_page.h"

#include <QVBoxLayout>

StatusPage::StatusPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel("Статус синхронизации");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 16px; color: #666;");
    layout->addWidget(m_statusLabel);
}

void StatusPage::setStatus(const QString &text)
{
    m_statusLabel->setText(text);
}