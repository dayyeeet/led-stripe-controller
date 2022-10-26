#include <ArduinoJson.h>

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

#include <AsyncEventSource.h>
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

  Serial.begin(9600);
  Serial.setTimeout(200); //Little timeout so not blocking esp
  delay(1000);
  Serial.println();

  //Initialize file system
  Serial.print("Starting file system... success: ");
  Serial.println(SPIFFS.begin());

  setupWebserver();
  setupAPI();
}

void loop() {
  onWebServer();
  //Always change things if arduino requests a change
  delay(1000);
  evaluateArduinoAnswers(false);
}



String ssid("Ledstripes");
String password("");
int resolution(3);
int brightness(50);
int gpio(12);
int ledcount(45);
bool captivePortal(true);

String globalColor = "#DEDEDE";
class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request) {
      return !(request->url().startsWith("/api") || request->url().startsWith("/site") || request->url().startsWith("/res")) && captivePortal;
    }
    void handleRequest(AsyncWebServerRequest *request)
    {
      request->redirect("http://ledstripes.local/site");
    }
};

class SiteRequestHandler : public AsyncWebHandler {
  public:
    SiteRequestHandler(){}
    virtual ~SiteRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request) {
      return request->url().startsWith("/site") || request->url().startsWith("/res");
    }

    void handleRequest(AsyncWebServerRequest *request)
    {
      if(request->url().startsWith("/site"))
      {  
        request->send(SPIFFS, "/web/index/index.html");
      }
      //Handle all resources
      else if(request->url().startsWith("/res/js"))
      {
        request->send(SPIFFS, "/web" + request->url(), String(), false, placeholder_processor);
      }
      else if(request->url().startsWith("/res"))
      {
        request->send(SPIFFS, "/web" + request->url());
      }
      
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

void setupWebserver()
{
  Serial.println("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  Serial.println("Configuring Wifi...");
  delay(0);
  //  ONLY index the wificonfig not instantly change the wifi
  //updateConfig(String("wificonfig"), false);
  if (password.equals("")) {
    WiFi.softAP(ssid);
  } else {
    WiFi.softAP(ssid, password);
  }

  Serial.println("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("Starting webserver...");
  server.begin();
  delay(0);
  //Connect to DNS and MDNS according to config
  Serial.println("Starting DNS server...");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.print("Registering MDNS connection for " + String("ledstripes") + "... success: ");
  Serial.println(MDNS.begin("ledstripes"));

  //Register Web Pages and captive portal
  Serial.println("Registering web pages...");
  server.addHandler(new CaptiveRequestHandler()); //Register Captive Handler
  server.addHandler(new SiteRequestHandler()); //Register webpage
  delay(0);
  Serial.println("REQUEST stripeconfig");
  Serial.println("REQUEST wificonfig");

}

void onWebServer()
{
  // handle all the DNS requests
  dnsServer.processNextRequest();
}

String placeholder_processor(const String& var)
{
  delay(0);
  if (var.equals("WIFI_SSID"))
    return ssid;
  else if (var.equals("BRIGHTNESS"))
    return String(brightness);
  else if (var.equals("RESOLUTION"))
    return String(resolution);
  else if (var.equals("LED_GPIO"))
    return String(gpio);
  else if (var.equals("LED_COUNT"))
    return String(ledcount);

  //replace color values of js
  else return color_processor(var);


}

String color_processor(const String& var)
{
  delay(0);
  if(var.equals("COLOR_GLOBAL")) return globalColor;
  return String();
}


//    MEMORY CONFIG CODE


void updateConfig(String requestedConfig, boolean changeValue)
{
  sendConfigRequest(requestedConfig);
  //Needed to always receive the answer
  delay(100);
  evaluateArduinoAnswers(changeValue);
}

void updateConfigs(boolean changeValue)
{
  sendConfigRequests();
  //Needed to always receive the answer
  delay(100);
  evaluateArduinoAnswers(changeValue);
}

//Gets executed first, then evaluateArduinoAnswers will review the requests that came in
void sendConfigRequests()
{
  Serial.println("REQUEST wificonfig");
}

void sendConfigRequest(String requestedConfig)
{
  requestedConfig.toLowerCase();
  Serial.println("REQUEST " + requestedConfig);
}

//Review everything the Serial sent and evaluate it
void evaluateArduinoAnswers(boolean changeValues)
{
  while (Serial.available() > 0)
  {
    delay(0);
    String line = Serial.readStringUntil('\n');
    delay(0);
    //Send CONFIRM packet to let arduino know it got through
    String subject = split(line, 0);
    subject.toLowerCase();
    Serial.println("CONFIRM " + subject);
    String arguments = line;
    //Change the WIFICONFIG if answer targets the WIFICONFIG
    if (line.startsWith("WIFICONFIG "))
    {
      arguments.replace("WIFICONFIG ", "");
      configureWifi(arguments, changeValues);
    }

    else if (line.startsWith("STRIPECONFIG "))
    {
      arguments.replace("STRIPECONFIG ", "");
      configureStripe(arguments, changeValues);
    }
  }
}

void configureWifi(String arguments, boolean changeWifi)
{

  //Packet in format: WIFICONFIG <SSID> <PASSWORD>
  //           index:            0      1
  String new_ssid = fromPacketArgument(split(arguments, 0));
  String new_password = fromPacketArgument(split(arguments, 1));
  if(!new_ssid.equals(ssid) || !new_password.equals(password))
  {
    password = new_password;
    ssid = new_ssid;
    if (password.equals(""))
       WiFi.softAP((char*)ssid.c_str(), NULL, 5, 0);
    else
       WiFi.softAP((char*)ssid.c_str(), (char*)password.c_str(), 5, 0);
  }
  //Important since we don't want to change the wifi all the time
  if (changeWifi)
  {
    //Send packet to Arduino that WIFICONFIG changed
    Serial.println("CHANGED wificonfig " + arguments);
  }
}

void configureStripe(String arguments, boolean changeStripe)
{
  gpio = fromPacketArgument(split(arguments, 0)).toInt();
  ledcount = fromPacketArgument(split(arguments, 1)).toInt();
  resolution = fromPacketArgument(split(arguments, 2)).toInt();

  if (changeStripe)
  {
    Serial.println("CHANGED stripeconfig " + arguments);
  }
}


//    API CODE

void setupAPI()
{
  Serial.println("Registering API pages...");
  registerAppAPIPages();
}


void registerAppAPIPages()
{
  server.on("/api/connected_to_ledstripes_network", HTTP_ANY, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "You are connected to an Ledstripes network");
    //request->send(response);
    delay(0);
    request->redirect("/site");
  });
  server.on("/api/login_wifi", HTTP_ANY, [](AsyncWebServerRequest * request) {
    //Redirect to wifi section
    if (request->hasParam("ssid"))
      if (request->hasParam("password"))
        configureWifi(toPacketArgument(request->getParam("ssid")->value()) + " " + toPacketArgument(request->getParam("password")->value()), true);
    //request->send(200, "text/plain", "success");
    delay(0);
    request->redirect("/site#wifi");
  });
  server.on("/api/stripe_cfg", HTTP_ANY, [](AsyncWebServerRequest * request) {
    //Redirect to wifi section
    if (request->hasParam("gpio"))
      if (request->hasParam("ledcount"))
        if (request->hasParam("resolution"))
          configureStripe(toPacketArgument(request->getParam("gpio")->value()) + " " + toPacketArgument(request->getParam("ledcount")->value()) + " " + toPacketArgument(request->getParam("resolution")->value()), true);
    //request->send(200, "text/plain", "success");
    delay(0);
    request->redirect("/site#led_settings");
  });
  server.on("/api/stripe_color", HTTP_ANY, [](AsyncWebServerRequest * request) {
    //Redirect to wifi section
    delay(10);
    Serial.print("CHANGED colorconfig "); 
    delay(10);
    if(request->hasParam("brightness")) Serial.print(request->getParam("brightness")->value());
    else
    Serial.print(0);
    delay(10);
    Serial.print(' ');
    delay(10);
    if(request->hasParam("global-color")) Serial.print(request->getParam("global-color")->value());
    else
    Serial.print("#ffffff");
    delay(10);
    //if(request->hasParam("brightness")) Serial.println(request->getParam("brightness")->value() + " Brightness");
    //if(request->hasParam("global-color")) Serial.println(request->getParam("global-color")->value() + " Global color");
    for(int i = 0; i < ledcount/resolution; i++)
    {
      delay(10);
      if(request->hasParam("color-" + String(i + 1))){  
      Serial.print(' ');
      delay(10);
      Serial.print(request->getParam("color-" + String(i+ 1))->value());
      }
      //if(request->hasParam("color-" + String(i + 1))) Serial.println(request->getParam("color-" + String(i+ 1))->value() + " Color-" + String(i + 1));
    }
    //request->send(200, "text/plain", "success");
    Serial.println();
    delay(2000);
    request->redirect("/site#color_settings");
  });
}


//    UTIL CODE

String toPacketArgument(String content)
{
  String temp = content;
  temp.replace(" ", "%");
  return temp;
}

String fromPacketArgument(String content)
{
  String temp = content;
  temp.replace("%", " ");
  return temp;
}

String split(String toSplit, int index)
{
  int currentIndex = 0;
  int stringStart = -1;
  int stringEnd = -1;
  for (int i = 0; i <= toSplit.length(); i++)
  {
    if (index == currentIndex)
    {
      if (stringStart == -1)
        stringStart = i;
      if (stringEnd == -1)
        if (toSplit.charAt(i) == ' ' || i == toSplit.length())
        {
          stringEnd = i;
          continue;
        }
    }

    if (toSplit.charAt(i) == ' ')
      currentIndex++;
  }

  if(stringStart == -1 && stringEnd == -1) return String("");

  String toReturn = toSplit.substring(stringStart, stringEnd);
  return toReturn;
}
