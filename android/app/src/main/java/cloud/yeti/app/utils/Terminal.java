package com.yourapp.utils;

public class Terminal {
    private static Terminal instance;
    private LogCallback callback;

    private Terminal() {
    }

    public static synchronized Terminal getInstance() {
        if (instance == null) {
            instance = new Terminal();
        }
        return instance;
    }

    public void log(String text) {
        if (callback == null) return;
        callback.log(text);
    }

    public void init(LogCallback appendLog) {
        this.callback = appendLog;
    }

    public interface LogCallback {
        void log(String text);
    }
}