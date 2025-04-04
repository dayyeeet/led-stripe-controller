#include <AsyncPrinter.h>
#include <async_config.h>
#include <DebugPrintMacros.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncTCPbuffer.h>
#include <SyncClient.h>
#include <tcp_axtls.h>
#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <AsyncWebSocket.h>
#include <AsyncWebSynchronization.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <StringArray.h>
#include <WebAuthentication.h>
#include <WebHandlerImpl.h>
#include <WebResponseImpl.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println();
  delay(1000);
  //Initialize file system
  Serial.print("Starting file system... success: ");
  Serial.println(SPIFFS.begin());

  setupWebserver();
  setupAPI();
}

void loop() {
  onWebServer();
}


String ssid("");
String password("");
int resolution(1);
int brightness(100);
int gpio(12);
int ledcount(1);
bool captivePortal(true);

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request) {
    return request->url() != "/" && captivePortal;
  }
  void handleRequest(AsyncWebServerRequest *request) {
    request->redirect("/");
  }
};

// The access points IP address and net mask
// It uses the default Google DNS IP address 8.8.8.8 to capture all
// Android dns requests

IPAddress apIP(192, 169, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
AsyncWebServer server(80);

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

void setupWebserver() {
  Serial.println("Waiting for WiFi credentials...");
  configureWifi();
  Serial.println("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  if (password.equals(""))
    WiFi.softAP(ssid);
  else
    WiFi.softAP(ssid, password);
  Serial.println("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  //Connect to DNS and MDNS according to config
  Serial.println("Starting DNS server...");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.print("Registering MDNS connection for " + String("ledstripes") + "...");
  if (!MDNS.begin("ledstripes")) Serial.println("MDNS connection failed!");

  //Register Web Pages and captive portal
  Serial.println("Registering web pages...");
  registerRootPage();
  server.addHandler(new CaptiveRequestHandler());  //Register Captive Handler
}

void configureWifi() {
  while (ssid.equals("")) {
    while (Serial.available() > 0) {
      String line = Serial.readStringUntil('\n');
      if (line.startsWith("WIFICONFIG ")) {
        //Packet in format: WIFICONFIG <SSID> <PASSWORD>
        //           index: 0          1      2
        ssid = split(line, 1);
        password = split(line, 2);
      }
    }
  }
}

void onWebServer() {
  // handle all the DNS requests
  dnsServer.processNextRequest();
}

void registerRootPage() {
  server.serveStatic("/res/css", SPIFFS, "/web/res/css");
  server.serveStatic("/res/js", SPIFFS, "/web/res/js").setDefaultFile("index.html").setTemplateProcessor(placeholder_processor);
  server.serveStatic("/", SPIFFS, "/web/index");
}

String placeholder_processor(const String &var) {
  if (var == "WIFI_SSID")
    return ssid;
  return String();
}


//    API CODE

void setupAPI() {
  Serial.println("Registering API pages...");
  registerAppAPIPages();
}


void registerAppAPIPages() {
  server.on("/connected_to_ledstripes_network", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "You are connected to an Ledstripes network");
    request->send(response);
  });
}


//    UTIL CODE

String split(String toSplit, int index) {
  int length = toSplit.length();
  int spaceIndex;
  int stringEndIndex = 0;
  for (int i = 0; i < length; i++) {
    if (spaceIndex == index) {
      if (stringEndIndex == 0) {
        if (toSplit.charAt(i) == ' ') {
          stringEndIndex = i - 1;
        } else if (i == length) {
          stringEndIndex = i;
        }
      } else {
        return toSplit.substring(i, stringEndIndex);
      }
    } else if (toSplit.charAt(i) == ' ') {
      spaceIndex++;
    }
  }
  return "";
}