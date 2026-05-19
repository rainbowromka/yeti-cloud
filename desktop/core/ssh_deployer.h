#pragma once

#include <string>
#include <functional>
#include <memory>

// forward declare (не тянем libssh в заголовок)
struct ssh_session_struct;
typedef struct ssh_session_struct* ssh_session;

class SshDeployer
{
public:
    struct Config {
        std::string host;
        std::string user;
        std::string password;
        int port = 22;
    };

    using ProgressCallback = std::function<void(const std::string &step)>;

    explicit SshDeployer(const Config &config);
    ~SshDeployer();

    bool deploy(ProgressCallback onProgress);
    std::string lastError() const;
    const Config& config() const { return m_config; }
    const std::string& adminKey() const { return m_adminKey; }

private:
    Config m_config;
    std::string m_error;
    std::string m_adminKey;
    ssh_session m_session = nullptr;

    void disconnect();
    bool uploadConfigFile(const std::string &localPath, const std::string &remotePath);

    bool connectSsh();
    bool execCommand(const std::string &cmd, std::string &output);
    bool uploadFile(const std::string &localPath, const std::string &remotePath);
    bool startService();
};