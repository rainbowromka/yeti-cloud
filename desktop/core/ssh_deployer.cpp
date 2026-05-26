#include "ssh_deployer.h"
#include "log_buffer.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <random>

#ifdef _WIN32
#include <windows.h>
#endif

// ========== Генератор admin_key ==========

static std::string generateAdminKey()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);

    std::ostringstream oss;
    for (int i = 0; i < 64; ++i) {
        int val = dist(gen);
        oss << std::hex << val;
    }
    return oss.str();
}

// ========== Путь к папке с exe ==========

static std::filesystem::path exeDir()
{
#ifdef _WIN32
    char exeBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exeBuf, MAX_PATH);
    return std::filesystem::path(exeBuf).parent_path();
#else
    return std::filesystem::current_path();
#endif
}

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

// ========== Главный метод ==========

bool SshDeployer::deploy()
{
    m_adminKey = generateAdminKey();

    LogBuffer::instance()->add("Connecting via SSH...");
    if (!connectSsh()) return false;

    LogBuffer::instance()->add("Checking connection...");
    std::string testOut;
    if (!execCommand("echo OK", testOut) || testOut.find("OK") == std::string::npos) {
        LogBuffer::instance()->add("ERROR: SSH connected but commands not executed");
        return false;
    }

    LogBuffer::instance()->add("Uploading server binary...");
    if (!uploadFile("yeti-server", "/opt/yeti-server/yeti-server")) return false;

    LogBuffer::instance()->add("Uploading server config...");
    {
        std::filesystem::path localCfg = exeDir() / "yeti-server-config.json";
        std::ofstream cfgFile(localCfg);
        cfgFile << "{\n  \"admin_key\": \"" << m_adminKey << "\"\n}\n";
        cfgFile.close();

        bool ok = uploadConfigFile(localCfg.string(), "/opt/yeti-server/yeti-server-config.json");
        std::filesystem::remove(localCfg);
        if (!ok) return false;
    }

    LogBuffer::instance()->add("Saving client config...");
    {
        std::filesystem::path localCfg = exeDir() / "yeti-desktop-config.json";
        std::ofstream cfgFile(localCfg);
        cfgFile << "{\n"
                << "  \"admin_key\": \"" << m_adminKey << "\",\n"
                << "  \"server_host\": \"" << m_config.host << "\",\n"
                << "  \"server_port\": " << m_config.port << "\n"
                << "}\n";
        cfgFile.close();
    }

    LogBuffer::instance()->add("Starting server...");
    if (!startService()) return false;

    LogBuffer::instance()->add("Server deployed successfully");
    disconnect();
    return true;
}

// ========== Подключение ==========

bool SshDeployer::connectSsh()
{
    m_session = ssh_new();
    if (!m_session) {
        LogBuffer::instance()->add("ERROR: Failed to create SSH session");
        return false;
    }

    ssh_options_set(m_session, SSH_OPTIONS_HOST, m_config.host.c_str());
    ssh_options_set(m_session, SSH_OPTIONS_PORT, &m_config.port);
    ssh_options_set(m_session, SSH_OPTIONS_USER, m_config.user.c_str());

    int verbosity = SSH_LOG_NOLOG;
    ssh_options_set(m_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

    int rc = ssh_connect(m_session);
    if (rc != SSH_OK) {
        LogBuffer::instance()->add("ERROR: Connection failed: " + std::string(ssh_get_error(m_session)));
        disconnect();
        return false;
    }

    ssh_options_set(m_session, SSH_OPTIONS_HOSTKEYS, "auto");
    ssh_options_set(m_session, SSH_OPTIONS_STRICTHOSTKEYCHECK, "no");

    rc = ssh_userauth_password(m_session, nullptr, m_config.password.c_str());
    if (rc != SSH_AUTH_SUCCESS) {
        LogBuffer::instance()->add("ERROR: Authentication failed");
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
        LogBuffer::instance()->add("ERROR: Failed to create SSH channel");
        return false;
    }

    int rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        ssh_channel_free(channel);
        LogBuffer::instance()->add("ERROR: Failed to open SSH channel");
        return false;
    }

    rc = ssh_channel_request_exec(channel, cmd.c_str());
    if (rc != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        LogBuffer::instance()->add("ERROR: Failed to execute command: " + cmd);
        return false;
    }

    char buffer[4096];
    int bytesRead;
    output.clear();

    bytesRead = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 0);
    while (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
        bytesRead = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 0);
    }

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

// ========== Загрузка конфиг-файла через SCP ==========

bool SshDeployer::uploadConfigFile(const std::string &localPath, const std::string &remotePath)
{
    std::filesystem::path remoteDir = std::filesystem::path(remotePath).parent_path();
    std::string mkdirCmd = "mkdir -p " + remoteDir.string() + " && chmod 755 " + remoteDir.string();
    std::string dummy;
    execCommand(mkdirCmd, dummy);

    std::ifstream file(localPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LogBuffer::instance()->add("ERROR: Failed to open config: " + localPath);
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0);

    ssh_scp scp = ssh_scp_new(m_session, SSH_SCP_WRITE, remoteDir.string().c_str());
    if (!scp) {
        LogBuffer::instance()->add("ERROR: SCP: " + std::string(ssh_get_error(m_session)));
        return false;
    }

    int rc = ssh_scp_init(scp);
    if (rc != SSH_OK) {
        ssh_scp_free(scp);
        LogBuffer::instance()->add("ERROR: SCP init: " + std::string(ssh_get_error(m_session)));
        return false;
    }

    std::string fileName = std::filesystem::path(remotePath).filename().string();
    rc = ssh_scp_push_file(scp, fileName.c_str(), fileSize, 0644);
    if (rc != SSH_OK) {
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        LogBuffer::instance()->add("ERROR: SCP push: " + std::string(ssh_get_error(m_session)));
        return false;
    }

    const size_t CHUNK = 4096;
    char buffer[CHUNK];
    while (file.read(buffer, CHUNK) || file.gcount() > 0) {
        rc = ssh_scp_write(scp, buffer, file.gcount());
        if (rc != SSH_OK) {
            ssh_scp_close(scp);
            ssh_scp_free(scp);
            LogBuffer::instance()->add("ERROR: SCP write error");
            return false;
        }
    }

    ssh_scp_close(scp);
    ssh_scp_free(scp);
    return true;
}

// ========== Загрузка файла через SCP ==========

bool SshDeployer::uploadFile(const std::string &localPath, const std::string &remotePath)
{
    std::filesystem::path fullLocalPath;
#ifdef _WIN32
    char exeBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exeBuf, MAX_PATH);
    fullLocalPath = std::filesystem::path(exeBuf).parent_path() / localPath;
#else
    fullLocalPath = localPath;
#endif

    if (!std::filesystem::exists(fullLocalPath)) {
        LogBuffer::instance()->add("ERROR: Binary not found: " + fullLocalPath.string());
        return false;
    }

    std::filesystem::path remoteDir = std::filesystem::path(remotePath).parent_path();
    std::string mkdirCmd = "mkdir -p " + remoteDir.string() + " && chmod 755 " + remoteDir.string();
    std::string dummy;
    execCommand(mkdirCmd, dummy);

    std::string cleanupCmd = "pkill -9 yeti-server 2>/dev/null; sleep 1; rm -f " + remotePath + "; echo OK";
    std::string cleanup;
    execCommand(cleanupCmd, cleanup);

    std::ifstream file(fullLocalPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LogBuffer::instance()->add("ERROR: Failed to open local file: " + fullLocalPath.string());
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0);

    ssh_scp scp = ssh_scp_new(m_session, SSH_SCP_WRITE, remoteDir.string().c_str());
    if (!scp) {
        LogBuffer::instance()->add("ERROR: Failed to create SCP session: " + std::string(ssh_get_error(m_session)));
        return false;
    }

    int rc = ssh_scp_init(scp);
    if (rc != SSH_OK) {
        ssh_scp_free(scp);
        LogBuffer::instance()->add("ERROR: SCP init failed: " + std::string(ssh_get_error(m_session)));
        return false;
    }

    std::string fileName = std::filesystem::path(remotePath).filename().string();
    rc = ssh_scp_push_file(scp, fileName.c_str(), fileSize, 0755);
    if (rc != SSH_OK) {
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        LogBuffer::instance()->add("ERROR: Failed to create file on server: " + std::string(ssh_get_error(m_session)));
        return false;
    }

    const size_t CHUNK = 16384;
    char buffer[CHUNK];
    while (file.read(buffer, CHUNK) || file.gcount() > 0) {
        rc = ssh_scp_write(scp, buffer, file.gcount());
        if (rc != SSH_OK) {
            ssh_scp_close(scp);
            ssh_scp_free(scp);
            LogBuffer::instance()->add("ERROR: Upload error");
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

    execCommand("pkill yeti-server 2>/dev/null; sleep 1; echo OK", output);

    std::string cmd =
        "chmod +x /opt/yeti-server/yeti-server && "
        "sh -c '(nohup /opt/yeti-server/yeti-server </dev/null >/opt/yeti-server/server.log 2>&1 &)' && echo OK";

    if (!execCommand(cmd, output)) return false;

    if (output.find("OK") == std::string::npos) {
        LogBuffer::instance()->add("ERROR: Failed to start server: " + output);
        return false;
    }

    return true;
}