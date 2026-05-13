#pragma once

#include <string>
#include <functional>

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

    bool deploy(ProgressCallback onProgress);

    std::string lastError() const;

private:
    Config m_config;
    std::string m_error;

    bool connectSsh();
    bool uploadBinary();
    bool startService();
};