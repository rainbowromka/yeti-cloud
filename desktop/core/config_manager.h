#pragma once

#include <QString>
#include <QJsonObject>

class ConfigManager
{
public:
    static ConfigManager& instance();

    // Загрузка и сохранение
    bool load();
    bool save();

    // Геттеры
    QString getAdminKey() const;
    QString getServerHost() const;
    int getServerPort() const;
    QString getDeviceKey() const;
    QString getDeviceId() const;

    // Сеттеры
    void setAdminKey(const QString& key);
    void setServerHost(const QString& host);
    void setServerPort(int port);
    void setDeviceKey(const QString& key);
    void setDeviceId(const QString& id);

    // Проверки
    bool isRegistered() const;      // Есть ли device_key и device_id
    bool hasConfig() const;         // Существует ли файл конфига

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Шифрование DPAPI
    QString encrypt(const QString& plainText) const;
    QString decrypt(const QString& cipherText) const;

    // Путь к конфигу
    QString getConfigPath() const;

private:
    QJsonObject m_data;
    bool m_loaded = false;
};