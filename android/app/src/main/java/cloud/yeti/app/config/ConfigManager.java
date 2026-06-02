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

    private ConfigManager(Context context) {
        configFile = new File(context.getFilesDir(), "yeti-config.json");
    }

    public static synchronized ConfigManager getInstance(Context context) {
        if (instance == null) {
            instance = new ConfigManager(context.getApplicationContext());
        }
        return instance;
    }

    public boolean load() {
        if (!configFile.exists()) {
            return false;
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
            return true;
        } catch (Exception e) {
            return false;
        }
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