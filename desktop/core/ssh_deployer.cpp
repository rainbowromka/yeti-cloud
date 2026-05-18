#include "ssh_deployer.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <fstream>
#include <filesystem>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

// ========== Конструктор / Деструктор ==========

SshDeployer::SshDeployer(const Config &config)
    : m_config(config), m_session(nullptr)
{
}

SshDeployer::~SshDeployer()
{
    disconnect();
}

void SshDeployer::disconnect()
{
    if (m_session) {
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
    }
}

std::string SshDeployer::lastError() const
{
    return m_error;
}

// ========== Главный метод ==========

bool SshDeployer::deploy(ProgressCallback onProgress)
{
    onProgress("Подключение по SSH...");
    if (!connectSsh()) return false;

    // Проверка, что SSH живой
    onProgress("Проверка соединения...");
    std::string testOut;
    if (!execCommand("echo OK", testOut) || testOut.find("OK") == std::string::npos) {
        m_error = "SSH-соединение установлено, но команды не выполняются";
        return false;
    }

    onProgress("Загрузка серверной части...");
    if (!uploadFile("yeti-server", "/opt/yeti-server/yeti-server")) return false;

    onProgress("Запуск сервера...");
    if (!startService()) return false;

    onProgress("Готово. Сервер запущен.");
    disconnect();
    return true;
}

// ========== Подключение ==========

bool SshDeployer::connectSsh()
{
    m_session = ssh_new();
    if (!m_session) {
        m_error = "Не удалось создать SSH-сессию";
        return false;
    }

    ssh_options_set(m_session, SSH_OPTIONS_HOST, m_config.host.c_str());
    ssh_options_set(m_session, SSH_OPTIONS_PORT, &m_config.port);
    ssh_options_set(m_session, SSH_OPTIONS_USER, m_config.user.c_str());

    // Без логов libssh
    int verbosity = SSH_LOG_NOLOG;
    ssh_options_set(m_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

    int rc = ssh_connect(m_session);
    if (rc != SSH_OK) {
        m_error = "Не удалось подключиться: " + std::string(ssh_get_error(m_session));
        disconnect();
        return false;
    }

    // Пропускаем проверку ключа (для тестов — ОК, в проде надо проверять)
    ssh_options_set(m_session, SSH_OPTIONS_HOSTKEYS, "auto");
    ssh_options_set(m_session, SSH_OPTIONS_STRICTHOSTKEYCHECK, "no");

    rc = ssh_userauth_password(m_session, nullptr, m_config.password.c_str());
    if (rc != SSH_AUTH_SUCCESS) {
        m_error = "Аутентификация не удалась. Проверьте логин/пароль.";
        disconnect();
        return false;
    }

    return true;
}

// ========== Выполнение команды ==========

bool SshDeployer::execCommand(const std::string &cmd, std::string &output)
{
    ssh_channel channel = ssh_channel_new(m_session);
    if (!channel) {
        m_error = "Не удалось создать SSH-канал";
        return false;
    }

    int rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        ssh_channel_free(channel);
        m_error = "Не удалось открыть SSH-канал";
        return false;
    }

    rc = ssh_channel_request_exec(channel, cmd.c_str());
    if (rc != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        m_error = "Не удалось выполнить команду: " + cmd;
        return false;
    }

    // Читаем stdout
    char buffer[4096];
    int bytesRead;
    output.clear();

    bytesRead = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 0);
    while (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
        bytesRead = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 0);
    }

    // Читаем stderr (добавляем к output)
    bytesRead = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 1);
    while (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
        bytesRead = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 1);
    }

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);

    return true;
}

// ========== Загрузка файла через SCP ==========

bool SshDeployer::uploadFile(const std::string &localPath, const std::string &remotePath)
{
    // Определяем полный путь к бинарнику
    std::filesystem::path fullLocalPath;
#ifdef _WIN32
    char exeBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exeBuf, MAX_PATH);
    fullLocalPath = std::filesystem::path(exeBuf).parent_path() / localPath;
#else
    fullLocalPath = localPath;
#endif

    if (!std::filesystem::exists(fullLocalPath)) {
        m_error = "Бинарник не найден: " + fullLocalPath.string();
        return false;
    }

    // Создаём директорию на сервере
    std::filesystem::path remoteDir = std::filesystem::path(remotePath).parent_path();
    std::string mkdirCmd = "mkdir -p " + remoteDir.string() + " && chmod 755 " + remoteDir.string();
    std::string dummy;
    execCommand(mkdirCmd, dummy);

    std::string cleanupCmd = "pkill -9 yeti-server 2>/dev/null; sleep 1; rm -f " + remotePath + "; echo OK";
    std::string cleanup;
    execCommand(cleanupCmd, cleanup);

    // Открываем локальный файл
    std::ifstream file(fullLocalPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        m_error = "Не удалось открыть локальный файл: " + fullLocalPath.string();
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0);

    // SCP-сессия
    ssh_scp scp = ssh_scp_new(m_session, SSH_SCP_WRITE, remoteDir.string().c_str());
    if (!scp) {
        m_error = "Не удалось создать SCP-сессию: " + std::string(ssh_get_error(m_session));
        return false;
    }

    int rc = ssh_scp_init(scp);
    if (rc != SSH_OK) {
        ssh_scp_free(scp);
        m_error = "Не удалось инициализировать SCP: " + std::string(ssh_get_error(m_session));
        return false;
    }

    std::string fileName = std::filesystem::path(remotePath).filename().string();
    rc = ssh_scp_push_file(scp, fileName.c_str(), fileSize, 0755);
    if (rc != SSH_OK) {
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        m_error = "Не удалось создать файл на сервере: " + std::string(ssh_get_error(m_session));
        return false;
    }

    // Пишем чанками
    const size_t CHUNK = 16384;
    char buffer[CHUNK];
    while (file.read(buffer, CHUNK) || file.gcount() > 0) {
        rc = ssh_scp_write(scp, buffer, file.gcount());
        if (rc != SSH_OK) {
            ssh_scp_close(scp);
            ssh_scp_free(scp);
            m_error = "Ошибка при загрузке файла";
            return false;
        }
    }

    ssh_scp_close(scp);
    ssh_scp_free(scp);

    return true;
}

// ========== Запуск сервиса ==========

bool SshDeployer::startService()
{
    std::string output;

    // Убиваем старый процесс
    execCommand("pkill yeti-server 2>/dev/null; sleep 1; echo OK", output);

    // Запускаем новый в фоне
    std::string cmd =
        "chmod +x /opt/yeti-server/yeti-server && "
        "sh -c '(nohup /opt/yeti-server/yeti-server </dev/null >/opt/yeti-server/server.log 2>&1 &)' && echo OK";

    if (!execCommand(cmd, output)) return false;

    if (output.find("OK") == std::string::npos) {
        m_error = "Не удалось запустить сервер: " + output;
        return false;
    }

    return true;
}