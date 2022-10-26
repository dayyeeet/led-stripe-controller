package com.ledstripes.app;

import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.view.View;
import android.webkit.WebResourceRequest;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

public class MainActivity extends AppCompatActivity {

    WebView ledWebPage;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        ledWebPage = findViewById(R.id.webView);
        ledWebPage.getSettings().setDomStorageEnabled(true);
        ledWebPage.getSettings().setJavaScriptEnabled(true);    //Needed for essential generating of values
        //Set webview to handle redirects inside the app
        ledWebPage.setWebViewClient(new WebViewClient() {
            public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request){
                view.loadUrl(request.getUrl().toString());
                return false;
            }
        });
        setTheme(R.style.Theme_LEDStripes);
        //Start clock to update the connection
        //CheckIfNetworkTask networkTask = new CheckIfNetworkTask(this);
        //networkTask.run();

    }

    void updateConnection(boolean connected)
    {
        if(connected)
        {
            ledWebPage.loadUrl("http://ledstripes.local/site");
            findViewById(R.id.message).setVisibility(View.INVISIBLE);
            ledWebPage.setVisibility(View.VISIBLE);
        }
        else
        {

            ledWebPage.setVisibility(View.INVISIBLE);
            findViewById(R.id.message).setVisibility(View.VISIBLE);
        }
    }

    //Returns if the user is connected to the LEDStripeNetwork
    private boolean isLEDStripeNetwork() {
        int statusCode = -1;
        try {
            URL url = new URL("http://ledstripes.local/api/connected_to_ledstripes_network/");
            HttpURLConnection http = (HttpURLConnection) url.openConnection();
            http.setRequestMethod("GET");
            Toast.makeText(this, "alive", Toast.LENGTH_SHORT).show();
            statusCode = http.getResponseCode();

        //Ignore if webpage is not present, since it doesn't have to be
        }catch (IOException ignored) {}
        return statusCode == HttpURLConnection.HTTP_OK;
    }
}