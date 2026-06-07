package cloud.yeti.app;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import androidx.fragment.app.Fragment;

import java.io.InputStream;

import cloud.yeti.app.core.DeploySsh;
import com.yourapp.utils.Terminal;

public class DeployFragment extends Fragment {
    private EditText hostInput, portInput, userInput, passwordInput;
    private Button deployBtn;
    private Terminal terminal;

    @Override
    public View onCreateView(
        LayoutInflater inflater,
        ViewGroup container,
        Bundle savedInstanceState)
    {
        terminal = Terminal.getInstance();

        View view = inflater.inflate(R.layout.fragment_deploy, container, false);

        hostInput = view.findViewById(R.id.hostInput);
        portInput = view.findViewById(R.id.portInput);
        userInput = view.findViewById(R.id.userInput);
        passwordInput = view.findViewById(R.id.passwordInput);
        deployBtn = view.findViewById(R.id.deployBtn);

        deployBtn.setOnClickListener(v -> startDeploy());

        return view;
    }

    private void startDeploy() {
        deployBtn.setEnabled(false);

        DeploySsh deploySsh = new DeploySsh(
                hostInput.getText().toString(),
                Integer.parseInt(portInput.getText().toString()),
                userInput.getText().toString(),
                passwordInput.getText().toString(),
                requireActivity().getFilesDir())
        {
            @Override
            protected void connectWithKey() {
                requireActivity().runOnUiThread(() -> {
                    if (getActivity() instanceof MainActivity) {
                        ((MainActivity) getActivity()).connectWithKey();
                    }
                });
            }

            @Override
            protected void handleEnableDeployButton() {
                enableDeployButton();
            }

            @Override
            protected InputStream getServerStream() throws Exception{
                return requireActivity().getAssets().open("yeti-server");
            }
        };

        new Thread(() -> {
            try {
                deploySsh.deploy();
            } catch (Exception e) {
                log("ERROR: " + e.getMessage());
                enableDeployButton();
            }
        }).start();
    }

    private void log(String message) {
        terminal.log(message);
    }

    private void enableDeployButton() {
        requireActivity().runOnUiThread(() -> deployBtn.setEnabled(true));
    }
}