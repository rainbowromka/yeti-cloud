package cloud.yeti.app.core;

import com.jcraft.jsch.ChannelExec;
import com.jcraft.jsch.JSch;
import com.jcraft.jsch.JSchException;
import com.jcraft.jsch.Session;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.SecureRandom;

import com.yourapp.utils.Terminal;

import cloud.yeti.app.config.ConfigManager;

public abstract class DeploySsh
{
    private final Terminal terminal;
    private final String adminKey;

    private final String host;
    private final int port;
    private final String user;
    private final String password;
    private final File filesDir;

    private Session session;

    protected abstract void connectWithKey();
    protected abstract void handleEnableDeployButton();
    protected abstract InputStream getServerStream() throws Exception;

    public DeploySsh (
        String host,
        int port,
        String user,
        String password,
        File filesDir
    ) {
        this.host = host;
        this.port = port;
        this.user = user;
        this.password = password;
        this.filesDir = filesDir;

        this.terminal = Terminal.getInstance();
        this.adminKey = generateAdminKey();

    }

    static class DeployException extends Exception {
        public DeployException(String message) {
            super(message);
        }
    }

    public void deploy()
    throws Exception
    {
        session = connectSsh();

        try {
            checkingConnection();
            uploadServerBinary();
            createAndUploadConfig();
            saveClientConfig();
            startServer();

            log("Server deployed successfully");
            log("Admin key: " + adminKey);
            connectWithKey();
        } catch (DeployException e) {
            log(e.getMessage());
        } finally {
            session.disconnect();
            handleEnableDeployButton();
        }
    }

    private void startServer()
    throws Exception
    {
        log("Starting server...");
        execCommand(session,
            "pkill -9 yeti-server 2>/dev/null; sleep 1; echo OK");

        // Для nohup — не ждём ответа, отправляем и выходим
        String cmd = "chmod +x /opt/yeti-server/yeti-server && " +
            "chmod +x /opt/yeti-server/yeti-server && "
            + "sh -c '(nohup /opt/yeti-server/yeti-server </dev/null >/opt/yeti-server/server.log 2>&1 &)' && echo OK";

        String startOut = execCommandNoWait(session, cmd);
        if (!startOut.contains("OK")) {
            throw new DeployException("ERROR: Failed to start server: " + startOut);
        }
    }

    private String execCommandNoWait(Session session, String cmd) throws Exception {
        ChannelExec channel = (ChannelExec) session.openChannel("exec");
        channel.setCommand(cmd);

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        channel.setOutputStream(out);
        channel.connect(5000);

        // Ждём максимум 3 секунды
        int waited = 0;
        while (!channel.isClosed() && waited < 3000) {
            Thread.sleep(100);
            waited += 100;
        }
        if (!channel.isClosed()) {
            channel.disconnect();
        }

        return out.toString();
    }

    private void createAndUploadConfig()
    throws Exception
    {
        log("Uploading server config...");
        String configJson = "{\n  \"admin_key\": \"" + adminKey + "\"\n}\n";
        File tmpConfig = File.createTempFile("yeti-server-config", ".json");
        try (FileOutputStream fos = new FileOutputStream(tmpConfig)) {
            fos.write(configJson.getBytes());
        }
        uploadFile(session, tmpConfig.getAbsolutePath(),
                "/opt/yeti-server/yeti-server-config.json");
        tmpConfig.delete();
    }

    private void uploadServerBinary() throws Exception {
        log("Uploading server binary...");
        String serverBinPath = getServerBinaryPath();
        if (serverBinPath == null) {
            throw new DeployException("ERROR: Server binary not found");
        }
        uploadFile(session, serverBinPath, "/opt/yeti-server/yeti-server");
    }

    private void checkingConnection()
    throws Exception
    {
        log("Checking connection...");
        String testOut = execCommand(session, "echo OK");
        if (!testOut.contains("OK")) {
            throw new DeployException(
                "ERROR: SSH connected but commands not executed");
        }
    }

    private Session connectSsh()
    throws JSchException
    {
        log("Connecting via SSH...");
        JSch jsch = new JSch();
        Session session = jsch.getSession(user, host, port);
        session.setPassword(password);
        session.setConfig("StrictHostKeyChecking", "no");
        session.connect(10000);

        return session;
    }

    private static String generateAdminKey() {
        SecureRandom random = new SecureRandom();
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 64; i++) {
            sb.append(Integer.toHexString(random.nextInt(16)));
        }
        return sb.toString();
    }

    private String execCommand(Session session, String cmd)
    throws Exception
    {
        ChannelExec channel = (ChannelExec) session.openChannel("exec");
        channel.setCommand(cmd);

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        ByteArrayOutputStream err = new ByteArrayOutputStream();
        channel.setOutputStream(out);
        channel.setErrStream(err);
        channel.connect(5000);

        while (!channel.isClosed()) {
            Thread.sleep(100);
        }

        channel.disconnect();
        return out.toString() + err.toString();
    }

    private String getServerBinaryPath() {
        File bin = new File(filesDir, "yeti-server");
        if (bin.exists()) {
            return bin.getAbsolutePath();
        }

        // Copy from assets
        try {
            InputStream in = getServerStream();
            FileOutputStream out = new FileOutputStream(bin);
            byte[] buf = new byte[8192];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
            out.close();
            in.close();
            bin.setExecutable(true);
            log("Server binary extracted from assets");
            return bin.getAbsolutePath();
        } catch (Exception e) {
            log("ERROR extracting binary: " + e.getMessage());
            return null;
        }
    }

    private void uploadFile(Session session, String localPath,
                            String remotePath) throws Exception {
        execCommand(session, "mkdir -p /opt/yeti-server && chmod 755 /opt/yeti-server");

        File file = new File(localPath);
        byte[] data = new byte[(int) file.length()];
        try (FileInputStream fis = new FileInputStream(file)) {
            fis.read(data);
        }
        String base64 = android.util.Base64.encodeToString(data, android.util.Base64.NO_WRAP);

        String cmd = "base64 -d > " + remotePath + " && chmod 755 " + remotePath;
        execCommandWithInput(session, cmd, base64);
    }

    private String execCommandWithInput(Session session, String cmd, String input) throws Exception {
        ChannelExec channel = (ChannelExec) session.openChannel("exec");
        channel.setCommand(cmd);

        OutputStream stdin = channel.getOutputStream();
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        ByteArrayOutputStream err = new ByteArrayOutputStream();
        channel.setOutputStream(out);
        channel.setErrStream(err);
        channel.connect(5000);

        stdin.write(input.getBytes());
        stdin.flush();
        stdin.close();

        while (!channel.isClosed()) {
            Thread.sleep(100);
        }

        channel.disconnect();
        return out.toString() + err.toString();
    }

    private void saveClientConfig()
        throws DeployException
    {
        log("Saving client config...");

        try {
            ConfigManager config = ConfigManager.getInstance();

            config.setAdminKey(adminKey);
            config.setServerHost(host);
            config.setServerPort(8080);

            config.save();

            log("Config saved");
        } catch (Exception e) {
            throw new DeployException("ERROR saving config: " + e.getMessage());
        }
    }

    private void log(String text)
    {
        terminal.log(text);
    }
}
