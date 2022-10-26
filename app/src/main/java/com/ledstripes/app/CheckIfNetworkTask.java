package com.ledstripes.app;

import android.widget.Toast;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;

public class CheckIfNetworkTask implements Runnable {

    MainActivity parent;
    boolean running = true;

    public CheckIfNetworkTask(MainActivity parent)
    {
        this.parent = parent;
    }

    @Override
    public void run() {
        int statusCode = -1;
        try {
            URL url = new URL("http://ledstripes.local/api/connected_to_ledstripes_network/");
            HttpURLConnection http = (HttpURLConnection) url.openConnection();
            http.setRequestMethod("GET");
            http.setConnectTimeout(100);
            statusCode = isHttpConnected(http) ? http.getResponseCode() : -1;

            //Ignore if webpage is not present, since it doesn't have to be
        }catch (IOException ignored) {}
        parent.updateConnection(statusCode == HttpURLConnection.HTTP_OK);

        try {
            Thread.sleep(3000);
            if(running) run();
        } catch (InterruptedException ignored) {}
    }

    private boolean isHttpConnected(HttpURLConnection con)
    {
        try {
            con.setDoOutput(con.getDoOutput()); // throws IllegalStateException if connected
            return false;
        } catch (IllegalStateException e) {
            return true;
        }
    }

    public void stop()
    {
        running = false;
    }
}
