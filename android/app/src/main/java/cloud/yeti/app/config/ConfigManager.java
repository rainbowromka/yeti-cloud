package cloud.yeti.app.config;

import android.content.Context;

import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

public class ConfigManager {
    private static ConfigManager instance;
    private final File configFile;

    private String serverHost;
    private int serverPort = 8080;
    private String adminKey;
    private String deviceKey;
    private String deviceId;
    private boolean isLoaded = false;

    private ConfigManager(File configFile) {
        this.configFile = configFile;
    }

    public static synchronized ConfigManager getInstance(File configFile) {
        if (instance == null) {
            instance = new ConfigManager(new File(configFile,  "yeti-config.json"));
        }
        return instance;
    }

    public static synchronized ConfigManager getInstance()
    throws HasNoConfig
    {
        if (instance == null) {
            throw new HasNoConfig("Error open file configuration");
        }
        return instance;
    }

    public ConfigManager load()
    {
        if (!configFile.exists()) {
            isLoaded = false;
            return this;
        }
        try (FileInputStream fis = new FileInputStream(configFile)) {
            byte[] data = new byte[(int) configFile.length()];
            fis.read(data);
            JSONObject json = new JSONObject(new String(data, StandardCharsets.UTF_8));

            serverHost = json.optString("server_host", "");
            serverPort = json.optInt("server_port", 8080);
            adminKey = json.optString("admin_key", "");
            deviceKey = json.optString("device_key", "");
            deviceId = json.optString("device_id", "");
            isLoaded = true;
        } catch (Exception e) {
            isLoaded = false;
        }
        return this;
    }

    public void save() {
        try {
            JSONObject json = new JSONObject();
            json.put("server_host", serverHost != null ? serverHost : "");
            json.put("server_port", serverPort);
            json.put("admin_key", adminKey != null ? adminKey : "");
            json.put("device_key", deviceKey != null ? deviceKey : "");
            json.put("device_id", deviceId != null ? deviceId : "");

            try (FileOutputStream fos = new FileOutputStream(configFile)) {
                fos.write(json.toString(2).getBytes(StandardCharsets.UTF_8));
            }
            isLoaded = true;
        } catch (Exception e) {
            // ignore
        }
    }

    public String getServerHost() { return serverHost; }
    public void setServerHost(String host) { this.serverHost = host; }

    public int getServerPort() { return serverPort; }
    public void setServerPort(int port) { this.serverPort = port; }

    public String getAdminKey() { return adminKey; }
    public void setAdminKey(String key) { this.adminKey = key; }

    public String getDeviceKey() { return deviceKey; }
    public void setDeviceKey(String key) { this.deviceKey = key; }

    public String getDeviceId() { return deviceId; }
    public void setDeviceId(String id) { this.deviceId = id; }

    public boolean isLoaded() {return this.isLoaded; }

    public boolean hasConfig() {
        return configFile.exists();
    }

    public boolean hasAdminKey() {
        return adminKey != null && !adminKey.isEmpty();
    }

    public boolean hasDeviceCredentials() {
        return deviceKey != null && !deviceKey.isEmpty()
            && deviceId != null && !deviceId.isEmpty();
    }
}