#include "config_manager.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#pragma comment(lib, "crypt32.lib")
#endif

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

QString ConfigManager::getConfigPath() const
{
    return QCoreApplication::applicationDirPath() + "/yeti-desktop-config.json";
}

bool ConfigManager::load()
{
    QString path = getConfigPath();
    QFileInfo info(path);

    if (!info.exists()) {
        m_loaded = false;
        m_data = QJsonObject();
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file:" << path;
        m_loaded = false;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse config JSON:" << error.errorString();
        m_loaded = false;
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Config file is not a JSON object";
        m_loaded = false;
        return false;
    }

    m_data = doc.object();
    m_loaded = true;
    return true;
}

bool ConfigManager::save()
{
    QString path = getConfigPath();
    QJsonDocument doc(m_data);
    QByteArray data = doc.toJson(QJsonDocument::Indented);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to write config file:" << path;
        return false;
    }

    file.write(data);
    file.close();
    return true;
}

QString ConfigManager::getAdminKey() const
{
    if (!m_loaded) {
        return QString();
    }
    // Сначала пробуем зашифрованный
    QString encrypted = m_data.value("admin_key_enc").toString();
    if (!encrypted.isEmpty()) {
        return decrypt(encrypted);
    }
    // Fallback: открытый ключ (тестовая среда)
    return m_data.value("admin_key").toString();
}

QString ConfigManager::getDeviceKey() const
{
    if (!m_loaded) {
        return QString();
    }
    // Сначала пробуем зашифрованный
    QString encrypted = m_data.value("device_key_enc").toString();
    if (!encrypted.isEmpty()) {
        return decrypt(encrypted);
    }
    // Fallback: открытый ключ (тестовая среда)
    return m_data.value("device_key").toString();
}

QString ConfigManager::getServerHost() const
{
    if (!m_loaded) {
        return QString();
    }
    return m_data.value("server_host").toString();
}

int ConfigManager::getServerPort() const
{
    if (!m_loaded) {
        return 8080;
    }
    return m_data.value("server_port").toInt(8080);
}

QString ConfigManager::getDeviceId() const
{
    if (!m_loaded) {
        return QString();
    }
    return m_data.value("device_id").toString();
}

void ConfigManager::setAdminKey(const QString& key)
{
    if (key.isEmpty()) {
        m_data.remove("admin_key_enc");
    } else {
        m_data["admin_key_enc"] = encrypt(key);
    }
}

void ConfigManager::setServerHost(const QString& host)
{
    m_data["server_host"] = host;
}

void ConfigManager::setServerPort(int port)
{
    m_data["server_port"] = port;
}

void ConfigManager::setDeviceKey(const QString& key)
{
    if (key.isEmpty()) {
        m_data.remove("device_key_enc");
    } else {
        m_data["device_key_enc"] = encrypt(key);
    }
}

void ConfigManager::setDeviceId(const QString& id)
{
    m_data["device_id"] = id;
}

bool ConfigManager::isRegistered() const
{
    return !getDeviceKey().isEmpty() && !getDeviceId().isEmpty();
}

bool ConfigManager::hasConfig() const
{
    QFileInfo info(getConfigPath());
    return info.exists();
}

// DPAPI Encryption (Windows only)
QString ConfigManager::encrypt(const QString& plainText) const
{
#ifdef Q_OS_WIN
    if (plainText.isEmpty()) {
        return QString();
    }

    QByteArray plainData = plainText.toUtf8();
    DATA_BLOB plainBlob;
    plainBlob.pbData = reinterpret_cast<BYTE*>(plainData.data());
    plainBlob.cbData = plainData.size();

    DATA_BLOB cipherBlob = {0, nullptr};

    if (!CryptProtectData(&plainBlob, L"YetiCloudKey", nullptr, nullptr, nullptr, 0, &cipherBlob)) {
        qWarning() << "CryptProtectData failed:" << GetLastError();
        return QString();
    }

    QByteArray cipherData(reinterpret_cast<char*>(cipherBlob.pbData), cipherBlob.cbData);
    LocalFree(cipherBlob.pbData);

    return QString::fromLatin1(cipherData.toBase64());
#else
    // Non-Windows: store as plain text (fallback)
    return plainText;
#endif
}

QString ConfigManager::decrypt(const QString& cipherText) const
{
#ifdef Q_OS_WIN
    if (cipherText.isEmpty()) {
        return QString();
    }

    QByteArray cipherData = QByteArray::fromBase64(cipherText.toLatin1());

    DATA_BLOB cipherBlob;
    cipherBlob.pbData = reinterpret_cast<BYTE*>(cipherData.data());
    cipherBlob.cbData = cipherData.size();

    DATA_BLOB plainBlob = {0, nullptr};

    if (!CryptUnprotectData(&cipherBlob, nullptr, nullptr, nullptr, nullptr, 0, &plainBlob)) {
        qWarning() << "CryptUnprotectData failed:" << GetLastError();
        return QString();
    }

    QString result = QString::fromUtf8(reinterpret_cast<char*>(plainBlob.pbData), plainBlob.cbData);
    LocalFree(plainBlob.pbData);

    return result;
#else
    // Non-Windows: assume plain text
    return cipherText;
#endif
}