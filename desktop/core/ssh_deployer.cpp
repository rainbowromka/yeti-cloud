#include "ssh_deployer.h"
#include <iostream>

SshDeployer::SshDeployer(const Config &config)
    : m_config(config)
{
}

bool SshDeployer::deploy(ProgressCallback onProgress)
{
    onProgress("Подключение по SSH...");
    if (!connectSsh()) return false;

    onProgress("Загрузка серверной части...");
    if (!uploadBinary()) return false;

    onProgress("Запуск сервера...");
    if (!startService()) return false;

    onProgress("Готово. Сервер запущен.");
    return true;
}

std::string SshDeployer::lastError() const
{
    return m_error;
}

bool SshDeployer::connectSsh()
{
    // TODO: libssh2
    m_error = "SSH deploy not implemented yet";
    return false;
}

bool SshDeployer::uploadBinary()
{
    // TODO: SFTP
    return false;
}

bool SshDeployer::startService()
{
    // TODO: запуск systemd-сервиса
    return false;
}