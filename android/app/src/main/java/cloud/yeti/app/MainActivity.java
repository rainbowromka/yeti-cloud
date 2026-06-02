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

import cloud.yeti.app.config.ConfigManager;

public class MainActivity extends AppCompatActivity {
    private YetiClient client;
    private TextView logView;
    private ScrollView scrollView;
    private StatusFragment statusFragment;
    private DeployFragment deployFragment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Terminal.getInstance().init(this::appendLog);
//        ConfigManager.getInstance().init();


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
        client = new YetiClient();
        client.setCallback(new YetiClient.Callback() {
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

    public void connectWithKey(String adminKey) {
        client.connect("192.168.0.53", 8080, adminKey);
        showFragment(statusFragment);
        statusFragment.setServerAddress("192.168.0.53:8080");
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
            String key = "49b4e33a11239c3afc1956835837bccf9aa08d65d7895f251680ef34082eb62a";
            connectWithKey(key);
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