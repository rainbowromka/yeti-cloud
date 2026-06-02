package cloud.yeti.app;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.fragment.app.Fragment;

public class StatusFragment extends Fragment {
    private TextView connectionIcon;
    private TextView connectionText;
    private TextView serverAddress;
    private TextView deviceInfo;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_status, container, false);

        connectionIcon = view.findViewById(R.id.connectionIcon);
        connectionText = view.findViewById(R.id.connectionText);
        serverAddress = view.findViewById(R.id.serverAddress);
        serverAddress.setText("Server: -");
        deviceInfo = view.findViewById(R.id.deviceInfo);

        return view;
    }

    public void setConnected(boolean connected) {
        if (connected) {
            connectionIcon.setTextColor(0xFF4CAF50);
            connectionText.setText("Connected");
            connectionText.setTextColor(0xFF4CAF50);
        } else {
            connectionIcon.setTextColor(0xFFE74C3C);
            connectionText.setText("Disconnected");
            connectionText.setTextColor(0xFFE74C3C);
        }
    }

    public void setServerAddress(String address) {
        serverAddress.setText("Server: " + address);
    }

    public void setDeviceInfo(String info) {
        deviceInfo.setText("Device: " + info);
    }
}