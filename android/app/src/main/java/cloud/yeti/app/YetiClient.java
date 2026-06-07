package cloud.yeti.app;

import android.os.Handler;
import android.os.Looper;

import org.json.JSONObject;

import java.io.File;
import java.util.concurrent.TimeUnit;

import cloud.yeti.app.config.ConfigManager;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.WebSocket;
import okhttp3.WebSocketListener;
import okhttp3.MediaType;

public class YetiClient {
    private static final MediaType JSON = MediaType.get("application/json; charset=utf-8");

    private final OkHttpClient httpClient;
    private final ConfigManager config;
    private WebSocket webSocket;
    private final Handler mainHandler;

    public interface Callback {
        void onLog(String message);
        void onConnected();
        void onDisconnected();
        void onError(String error);
    }

    private Callback callback;

    public YetiClient(ConfigManager config)
    {
        this.config = config;
        this.httpClient = new OkHttpClient.Builder()
                .connectTimeout(10, TimeUnit.SECONDS)
                .readTimeout(10, TimeUnit.SECONDS)
                .build();
        this.mainHandler = new Handler(Looper.getMainLooper());
    }

    public YetiClient setCallback(Callback callback) {
        this.callback = callback;
        return this;
    }

    public void connect() {
        if (!config.isLoaded()) {
            log("Server settings are missing.");
            return;
        }

        String deviceKey = config.getDeviceKey();
        String deviceId = config.getDeviceId();

        if (deviceKey != null && !deviceKey.isEmpty() && deviceId != null && !deviceId.isEmpty()) {
            // Уже зарегистрирован — сразу WebSocket
            log("Already registered, connecting WebSocket");
            connectWebSocket();
        } else {
            // Не зарегистрирован — HTTP-регистрация
            log("Starting HTTP registration");
            stepInvite();
        }
    }

    public void disconnect() {
        log("Disconnecting...");
        if (webSocket != null) {
            webSocket.close(1000, "User disconnect");
        }
    }

    // ─── Step 1: POST /api/invite ───

    private void stepInvite() {
        log("Step 1: Creating invite...");

        String url = "http://" + config.getServerHost() + ":" + config.getServerPort() + "/api/invite";
        Request request = new Request.Builder()
                .url(url)
                .header("Authorization", "Bearer " + config.getAdminKey())
                .post(RequestBody.create("", JSON))
                .build();

        httpClient.newCall(request).enqueue(new okhttp3.Callback() {
            @Override
            public void onFailure(okhttp3.Call call, java.io.IOException e) {
                log("ERROR: Invite failed - " + e.getMessage());
                error("Invite failed: " + e.getMessage());
            }

            @Override
            public void onResponse(okhttp3.Call call, Response response) throws java.io.IOException {
                if (!response.isSuccessful()) {
                    log("ERROR: Invite failed - " + response.message());
                    error("Invite failed: " + response.message());
                    response.close();
                    return;
                }

                try {
                    String body = response.body().string();
                    JSONObject json = new JSONObject(body);
                    String inviteToken = json.getString("invite_token");
                    log("Got invite token");
                    response.close();
                    stepRegister(inviteToken);
                } catch (Exception e) {
                    log("ERROR: Parse invite response - " + e.getMessage());
                    error("Parse error: " + e.getMessage());
                    response.close();
                }
            }
        });
    }

    // ─── Step 2: POST /api/register ───

    private void stepRegister(String inviteToken) {
        log("Step 2: Registering device...");

        String url = "http://" + config.getServerHost() + ":" + config.getServerPort()
            + "/api/register";

        try {
            JSONObject body = new JSONObject();
            body.put("invite_token", inviteToken);
            body.put("device_name", "Android Device");

            Request request = new Request.Builder()
                    .url(url)
                    .post(RequestBody.create(body.toString(), JSON))
                    .build();

            httpClient.newCall(request).enqueue(new okhttp3.Callback() {
                @Override
                public void onFailure(okhttp3.Call call, java.io.IOException e) {
                    log("ERROR: Register failed - " + e.getMessage());
                    error("Register failed: " + e.getMessage());
                }

                @Override
                public void onResponse(okhttp3.Call call, Response response) throws java.io.IOException {
                    if (!response.isSuccessful()) {
                        log("ERROR: Register failed - " + response.message());
                        error("Register failed: " + response.message());
                        response.close();
                        return;
                    }

                    try {
                        String body = response.body().string();
                        JSONObject json = new JSONObject(body);
                        config.setDeviceKey(json.getString("device_key"));
                        config.setDeviceId(json.getString("device_id"));
                        log("Device registered: " + config.getDeviceId());
                        config.save();
                        response.close();
                        connectWebSocket();
                    } catch (Exception e) {
                        log("ERROR: Parse register response - " + e.getMessage());
                        error("Parse error: " + e.getMessage());
                        response.close();
                    }
                }
            });
        } catch (Exception e) {
            log("ERROR: Register body - " + e.getMessage());
            error("Register error: " + e.getMessage());
        }
    }

    // ─── Step 3: WebSocket ───

    private void connectWebSocket() {
        String wsUrl = "ws://" + config.getServerHost() + ":" + config.getServerPort()
                + "/ws?token=" + config.getDeviceKey() + "&device_id=" + config.getDeviceId();
        log("Opening WebSocket: " + wsUrl);

        Request request = new Request.Builder().url(wsUrl).build();
        webSocket = httpClient.newWebSocket(request, new WebSocketListener() {
            @Override
            public void onOpen(WebSocket webSocket, Response response) {
                log("WebSocket connected");
                mainHandler.post(() -> {
                    if (callback != null) callback.onConnected();
                });
            }

            @Override
            public void onMessage(WebSocket webSocket, String text) {
                log("WS message: " + text);
            }

            @Override
            public void onClosing(WebSocket webSocket, int code, String reason) {
                log("WebSocket closing: " + reason);
                webSocket.close(1000, null);
            }

            @Override
            public void onClosed(WebSocket webSocket, int code, String reason) {
                log("WebSocket closed");
                mainHandler.post(() -> {
                    if (callback != null) callback.onDisconnected();
                });
            }

            @Override
            public void onFailure(WebSocket webSocket, Throwable t, Response response) {
                log("WebSocket error: " + t.getMessage());
                mainHandler.post(() -> {
                    if (callback != null) callback.onError(t.getMessage());
                });
            }
        });
    }

    // ─── Helpers ───

    private void log(String message) {
        android.util.Log.d("YetiClient", message);
        mainHandler.post(() -> {
            if (callback != null) callback.onLog(message);
        });
    }

    private void error(String message) {
        mainHandler.post(() -> {
            if (callback != null) callback.onError(message);
        });
    }
}