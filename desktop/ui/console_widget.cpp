#include "console_widget.h"
#include "log_buffer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QClipboard>
#include <QApplication>

ConsoleWidget::ConsoleWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Кнопки
    auto* btnLayout = new QHBoxLayout();
    m_clearBtn = new QPushButton("Clear");
    m_copyBtn = new QPushButton("Copy");
    m_clearBtn->setFixedSize(50, 22);
    m_copyBtn->setFixedSize(50, 22);
    m_clearBtn->setStyleSheet("font-size: 10px; background: #444; color: white; border: none;");
    m_copyBtn->setStyleSheet("font-size: 10px; background: #444; color: white; border: none;");
    btnLayout->addWidget(m_clearBtn);
    btnLayout->addWidget(m_copyBtn);
    btnLayout->addStretch();

    // Текст
    m_textLabel = new QLabel();
    m_textLabel->setStyleSheet(
        "background: #000; color: #0F0; font-family: monospace; font-size: 10px;"
        "padding: 4px; border: 1px solid #333;");
    m_textLabel->setFixedHeight(80);
    m_textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(m_textLabel);

    connect(m_clearBtn, &QPushButton::clicked, this, &ConsoleWidget::onClear);
    connect(m_copyBtn, &QPushButton::clicked, this, &ConsoleWidget::onCopy);
}

void ConsoleWidget::appendMessage(const QString& message)
{
    QStringList lines = LogBuffer::instance()->all();
    // Показываем только последние 4 строки
    QStringList visible;
    int start = qMax(0, lines.size() - 4);
    for (int i = start; i < lines.size(); ++i) {
        visible.append(lines[i]);
    }
    m_textLabel->setText(visible.join('\n'));
}

void ConsoleWidget::onClear()
{
    LogBuffer::instance()->clear();
    m_textLabel->clear();
}

void ConsoleWidget::onCopy()
{
    QApplication::clipboard()->setText(LogBuffer::instance()->toString());
}