package cloud.yeti.app;

import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.ScrollView;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.Fragment;

import com.yourapp.utils.Terminal;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

import cloud.yeti.app.config.ConfigManager;

public class MainActivity extends AppCompatActivity {
    private YetiClient client;
    private TextView logView;
    private ScrollView scrollView;
    private StatusFragment statusFragment;
    private DeployFragment deployFragment;

    private ConfigManager config;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Terminal.getInstance().init(this::appendLog);

        copyTestConfigIfExists();

        config = ConfigManager.getInstance(getFilesDir()).load();

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        logView = findViewById(R.id.logView);
        scrollView = findViewById(R.id.scrollView);

        // Кнопка папки
        ImageView folderBtn = findViewById(R.id.folderBtn);
        folderBtn.setOnClickListener(v -> appendLog("Sync folder clicked"));

        // Кнопка меню
        ImageView menuBtn = findViewById(R.id.menuBtn);
        menuBtn.setOnClickListener(v -> toolbar.showOverflowMenu());

        // Фрагменты
        statusFragment = new StatusFragment();
        deployFragment = new DeployFragment();

        // Показать статус по умолчанию
        showFragment(statusFragment);

        // Клиент
        client = new YetiClient(config).setCallback(new YetiClient.Callback()
        {
            @Override
            public void onLog(String message) {
                appendLog(message);
            }

            @Override
            public void onConnected() {
                appendLog("*** CONNECTED ***");
                statusFragment.setConnected(true);
            }

            @Override
            public void onDisconnected() {
                appendLog("*** DISCONNECTED ***");
                statusFragment.setConnected(false);
            }

            @Override
            public void onError(String error) {
                appendLog("*** ERROR: " + error + " ***");
            }
        });
    }

    private void copyTestConfigIfExists() {
        try {
            String[] assets = getAssets().list("");
            boolean hasConfig = false;
            if (assets != null) {
                for (String name : assets) {
                    if ("yeti-config.json".equals(name)) {
                        hasConfig = true;
                        break;
                    }
                }
            }

            if (!hasConfig) return;

            File configFile = new File(getFilesDir(), "yeti-config.json");
            // Всегда заменяем при наличии в assets
            try (InputStream is = getAssets().open("yeti-config.json");
                 OutputStream os = new FileOutputStream(configFile, false)) {
                byte[] buffer = new byte[8192];
                int length;
                while ((length = is.read(buffer)) > 0) {
                    os.write(buffer, 0, length);
                }
            }
        } catch (Exception e) {
            // Если нет assets — продолжаем с обычным конфигом
        }
    }

    public void connectWithKey() {
        client.connect();
        showFragment(statusFragment);
        statusFragment.setServerAddress(config.getServerHost());
    }

    private void showFragment(Fragment fragment) {
        getSupportFragmentManager().beginTransaction()
            .replace(R.id.fragmentContainer, fragment)
            .commit();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_status) {
            showFragment(statusFragment);
            return true;
        } else if (id == R.id.action_deploy) {
            showFragment(deployFragment);
            return true;
        } else if (id == R.id.action_connect) {
            connectWithKey();
            return true;
        } else if (id == R.id.action_disconnect) {
            client.disconnect();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    public void appendLog(String text) {
        runOnUiThread(() -> {
            logView.append(text + "\n");
            scrollView.post(() -> scrollView.fullScroll(ScrollView.FOCUS_DOWN));
        });
    }
}