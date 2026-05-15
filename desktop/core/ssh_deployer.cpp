#include "ssh_deployer.h"

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <sstream>
#include <fstream>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

static bool g_wsInitialized = false;

static void initWinsock()
{
    if (!g_wsInitialized) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        g_wsInitialized = true;
    }
}

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
    initWinsock();

    // --- 1. TCP-сокет ---
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(m_config.port);
    int rc = getaddrinfo(m_config.host.c_str(), portStr.c_str(), &hints, &result);
    if (rc != 0) {
        m_error = "DNS: не удалось разрешить " + m_config.host;
        return false;
    }

    SOCKET sock = WSASocket(result->ai_family, result->ai_socktype,
                            result->ai_protocol, nullptr, 0, 0);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(result);
        m_error = "Не удалось создать сокет";
        return false;
    }

    rc = WSAConnect(sock, result->ai_addr, (int)result->ai_addrlen, nullptr, nullptr, nullptr, nullptr);
    freeaddrinfo(result);
    if (rc != 0) {
        closesocket(sock);
        m_error = "TCP: не удалось подключиться к " + m_config.host + ":" + portStr;
        return false;
    }

    // --- 2. SSH-сессия ---
    LIBSSH2_SESSION *session = libssh2_session_init();
    if (!session) {
        closesocket(sock);
        m_error = "Не удалось создать SSH-сессию";
        return false;
    }

    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    rc = libssh2_session_handshake(session, (libssh2_socket_t)sock);
    if (rc) {
        libssh2_session_free(session);
        closesocket(sock);
        m_error = "SSH handshake не удался: " + std::to_string(rc);
        return false;
    }

    // --- 3. Аутентификация по паролю ---
    const char *fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA256);
    if (fingerprint) {
        (void)fingerprint;
    }

    rc = libssh2_userauth_password(session, m_config.user.c_str(), m_config.password.c_str());
    if (rc != 0) {
        libssh2_session_free(session);
        closesocket(sock);
        m_error = "Аутентификация не удалась. Проверьте логин/пароль.";
        return false;
    }

    // --- 4. Проверка и подготовка сервера ---
    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(session);
    if (!channel) {
        libssh2_session_free(session);
        closesocket(sock);
        m_error = "Не удалось открыть SSH-канал";
        return false;
    }

    std::string cmd = "mkdir -p /opt/yeti-server 2>&1 && echo OK || echo FAIL";
    // std::string cmd = "sudo mkdir -p /opt/yeti-server 2>&1 && echo OK || echo FAIL";
    libssh2_channel_exec(channel, cmd.c_str());

    std::string output;
    char buf[256];
    int bytesRead;
    while ((bytesRead = libssh2_channel_read(channel, buf, sizeof(buf) - 1)) > 0) {
        buf[bytesRead] = '\0';
        output += buf;
    }
    libssh2_channel_close(channel);
    libssh2_channel_free(channel);

    if (output.find("OK") == std::string::npos) {
        libssh2_session_free(session);
        closesocket(sock);
        m_error = "Не удалось создать /opt/yeti-server: " + output;
        return false;
    }

    libssh2_session_free(session);
    closesocket(sock);
    return true;
}

bool SshDeployer::uploadBinary()
{
    m_error = "Загрузка бинарника пока не реализована";
    return false;
}

bool SshDeployer::startService()
{
    m_error = "Запуск сервиса пока не реализован";
    return false;
}