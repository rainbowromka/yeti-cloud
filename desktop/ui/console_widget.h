#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class ConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(QWidget* parent = nullptr);

public slots:
    void appendMessage(const QString& message);

private slots:
    void onClear();
    void onCopy();

private:
    QLabel* m_textLabel;
    QPushButton* m_clearBtn;
    QPushButton* m_copyBtn;
};