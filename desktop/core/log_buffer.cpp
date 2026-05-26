#include "log_buffer.h"

LogBuffer* LogBuffer::instance()
{
    static LogBuffer inst;
    return &inst;
}

LogBuffer::LogBuffer(QObject* parent)
    : QObject(parent)
{
}

LogBuffer* LogBuffer::add(const QString& message)
{
    if (m_messages.size() >= MAX_LINES) {
        m_messages.removeFirst();
    }
    m_messages.append(message);
    emit newMessage(message);
    return this;
}

LogBuffer* LogBuffer::add(const std::string& message)
{
    return add(QString::fromStdString(message));
}

LogBuffer* LogBuffer::add(const char* message)
{
    return add(QString::fromUtf8(message));
}

QStringList LogBuffer::all() const
{
    return m_messages;
}

void LogBuffer::clear()
{
    m_messages.clear();
}

QString LogBuffer::toString() const
{
    return m_messages.join('\n');
}