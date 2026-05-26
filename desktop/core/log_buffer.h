#pragma once

#include <QObject>
#include <QStringList>
#include <string>

class LogBuffer : public QObject
{
    Q_OBJECT

public:
    static LogBuffer* instance();

    LogBuffer* add(const QString& message);
    LogBuffer* add(const std::string& message);
    LogBuffer* add(const char* message);
    QStringList all() const;
    void clear();
    QString toString() const;

signals:
    void newMessage(const QString& message);

private:
    explicit LogBuffer(QObject* parent = nullptr);
    QStringList m_messages;
    static const int MAX_LINES = 50;
};