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
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#ifndef SSID
#define SSID "Ledstripes"
#endif

#ifndef PASSWORD
#define PASSWORD ""
#endif

#ifndef PID
#define PID "ledstripes"
#endif

const char *softAP_ssid = SSID;
const char *project_id = PID;
const char *softAP_password = PASSWORD;

// The access points IP address and net mask
// It uses the default Google DNS IP address 8.8.8.8 to capture all
// Android dns requests
IPAddress apIP(192, 169, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

AsyncWebServer server(80);

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;


void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println();
  Serial.println("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  // its an open WLAN access point without a password
  if(softAP_password && !softAP_password[0])
  WiFi.softAP(softAP_ssid);
  //its an open WLAN access point with password
  else
  WiFi.softAP(softAP_ssid, softAP_password);
  Serial.println("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Starting file system...");
  SPIFFS.begin();
  Serial.println("Starting DNS server...");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println("Registering MDNS connection for " + String(project_id) + "...");
  if(!MDNS.begin(project_id)) Serial.print("MDNS connection failed!");
  Serial.println("Registering web pages...");
  registerRootPage();
  //server.serveStatic("/checking_wifi", SPIFFS, "/web/basic/basic_message.html").setTemplateProcessor(checking_ssid_password_processor);  
  //server.serveStatic("/checking_wifi_result_ok", SPIFFS, "/web/basic/basic_message.html").setTemplateProcessor(result_ok_ssid_password_processor);
  //server.serveStatic("/checking_wifi_result_error", SPIFFS, "/web/basic/basic_message.html").setTemplateProcessor(result_bad_ssid_password_processor);
  server.serveStatic("/new_ledstripe", SPIFFS, "/web/forms/add_led_stripe.html"); 
  server.onNotFound([](AsyncWebServerRequest *request) {
        captivePortal(request);
  });
  Serial.println("Registering API pages...");
  registerBasicAPIPages();
  Serial.println("Starting server...");
  server.begin();
}

void registerRootPage()
{
  server.serveStatic("/res", SPIFFS, "/web/res");
  server.serveStatic("/captive_landing", SPIFFS, "/web/forms/wifi_ssid_pass.html");
}

void registerBasicAPIPages()
{
  server.on("/connected_to_ledstripes_network", HTTP_GET, [](AsyncWebServerRequest *request){
              AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "You are connected to an Ledstripes network");
              request->send(response);
       });

  server.on("/login_wifi", HTTP_GET, [](AsyncWebServerRequest *request){

              //Read incoming url /login_wifi?ssid=SSID&password=PASSWORD
              String ssid = "";
              String password;
              if(request->hasParam("ssid")) ssid = request->getParam("ssid")->value();
              if(request->hasParam("password")) password = request->getParam("password")->value();

              //Handle incoming url /login_wifi?ssid=SSID&password=PASSWORD
              Serial.println("SSID: '" + ssid + "' | password: '" + password + "'");
              //Redirect back to home page hence only api page
              request->send(SPIFFS, "/web/basic/basic_message.html", String(), false, checking_ssid_password_processor);

              bool pass_and_ssid_ok = false;

              //TODO: check if password and ssid are ok
              
              if(pass_and_ssid_ok) request->send(SPIFFS, "/web/basic/basic_message.html", String(), false, result_ok_ssid_password_processor);
              else request->send(SPIFFS, "/web/basic/basic_message.html", String(), false, result_bad_ssid_password_processor);
       });       
}


void registerCaptiveRedirectPage(char *domain)
{
      server.on(domain, HTTP_GET, [](AsyncWebServerRequest *request){
          captivePortal(request);
      });
}

boolean captivePortal(AsyncWebServerRequest *request)
{
if (request->url() != "/" && request->url() != "" && request->url() != NULL) {
    AsyncWebServerResponse *response = request->beginResponse(302);
    response->addHeader("Location", String("http://") + toStringIp(apIP) + "/captive_landing");
    request->send(response);
    return true;
  }
  return false;
}

void loop() {
  // handle all the DNS requests
  dnsServer.processNextRequest();
}

/** processors (replacing template texts for the basic webpage) */

String checking_ssid_password_processor(const String& var)
{
  if(var == "STATUS")
    return String("normal");
  if(var == "TITLE")
    return String("Checking...");
  if(var == "SUB_TITLE")
    return String("...If WiFi credentials are OK");  
  return String();
}

String result_bad_ssid_password_processor(const String& var)
{
  if(var == "STATUS")
    return String("error");
  if(var == "TITLE")
    return String("Bad configuration :c");
  if(var == "SUB_TITLE")
    return String("The WiFi can't take this configuration, since another WiFi exists with this name.");
  if(var == "SHOULD_SHOW")
    return String("true");
  if(var == "GO_BACK_LINK")
    return String("http://" + toStringIp(apIP) + "/captive_landing/");      
  return String();
}

String result_ok_ssid_password_processor(const String& var)
{
  if(var == "STATUS")
    return String("ok");
  if(var == "TITLE")
    return String("Change successful");
  if(var == "SUB_TITLE")
    return String("You have changed the networks name to " + String(SSID) + " and the password to " + String(PASSWORD) + ".");
  if(var == "SHOULD_SHOW")
    return String("true");
  if(var == "GO_BACK_LINK")
    return String("http://" + toStringIp(apIP) + "/captive_landing/");      
  return String();
}



/** Is this an IP? */
boolean isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
