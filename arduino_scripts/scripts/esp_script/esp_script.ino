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

void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println();
  
  //Initialize file system
  Serial.print("Starting file system... success: ");
  Serial.println(SPIFFS.begin());

  setupWebserver();
  setupAPI();
  setupJSON();
}

void loop() {
  onWebServer();
  callRightJSONPacketListeners();
}

//    JSON CODE

class JSONPacket {
  public:
    void send(String packetType, DynamicJsonDocument jsonContent)
    {
      DynamicJsonDocument sentDoc(1024);
      sentDoc["content"] = jsonContent;
      sentDoc["type"] = packetType;
      serializeJson(sentDoc, Serial);
    }
};

class JSONPacketListener {  
  public:
    JSONPacketListener() = default;
    JSONPacketListener(String packetType){
      _packetType = packetType;
    }
    virtual void onReceive(DynamicJsonDocument doc) = 0;
    String getPacketType() {
      return _packetType;
    }
  protected:
    String _packetType;
};

class LoginJSONPacketListener : public JSONPacketListener {
  public:
    void onReceive(DynamicJsonDocument doc) override
    {

    }
};

class JSONPacketVerifier 
{
  public:
    JSONPacketVerifier(JSONPacketListener* packetListener)
    {
        this->packetType = packetListener->getPacketType();
    } 
    DynamicJsonDocument unwrap(String content)
    {
        if(!content.startsWith("{")) return DynamicJsonDocument(16);
        DynamicJsonDocument wrapped(1024);
        deserializeJson(wrapped, content);
        if(wrapped["type"] != packetType)
        return DynamicJsonDocument(16);
        return wrapped["content"];
    }
  private:
    String packetType;    
};

class JSONStorage {
  public:
    void store(DynamicJsonDocument doc, char* fileName)
    {
      File file = SPIFFS.open(fileName, "w");
      String jsonContent = "";
      serializeJson(doc, jsonContent);
      file.write(jsonContent.c_str());
      file.close();
    }

    DynamicJsonDocument pull(char* fileName)
    {
      File file = SPIFFS.open(fileName, "r");
      String content = "";
      while(file.available())
      {
        content = content + file.readStringUntil('\n');
      }
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, content);
      file.close();
      return doc; 
    }
};

JSONStorage jsonStorage = JSONStorage();

const int AMOUNT_OF_LISTENER_TYPES = 5;
int listenerArrayIndex = 0;
JSONPacketListener *listenerArray[AMOUNT_OF_LISTENER_TYPES];

void setupJSON()
{

}

void registerJSONPacketListener(class JSONPacketListener* listener)
{
  if(listenerArrayIndex >= AMOUNT_OF_LISTENER_TYPES) return;
  listenerArray[listenerArrayIndex] = listener;
  listenerArrayIndex++;
}

void callRightJSONPacketListeners()
{
    String content = "";
    while(Serial.available())
    {
      content = content + Serial.readStringUntil('\n');
    }

    for(int i = 0; i < listenerArrayIndex; i++)
    {
      JSONPacketListener* listener = listenerArray[i];
      JSONPacketVerifier verifier = JSONPacketVerifier(listener);
      DynamicJsonDocument correctDocument = verifier.unwrap(content);
      if(!correctDocument.isNull())
      {
        listener->onReceive(correctDocument);
      }
    }
}


//    WEB CODE
class WifiCredentials {
  public:
    WifiCredentials(String ssid, String pid, String password)
    {
      _ssid = ssid;
      _pid = pid;
      _password = password;
    }
    String getSSID() { return _ssid; }
    String setSSID(String ssid) { _ssid = ssid; }
    String getPID() { return _pid; }
    String setPID(String pid) { _pid = pid; }
    String getPassword() { return _password; }
    String setPassword(String password) { _password = password; }
  private:
    String _ssid;
    String _pid;
    String _password;
};

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    DynamicJsonDocument captiveEnabledDoc = JSONStorage().pull("/cfg/ap_config.json");
                                             //Check if captive portal is enabled in config
    return request->url() != "/" && !captiveEnabledDoc.isNull() ? captiveEnabledDoc["captive_portal"] : true;                               
  }
  void handleRequest(AsyncWebServerRequest *request)
  {
    request->redirect("/");
  }
};

// The access points IP address and net mask
// It uses the default Google DNS IP address 8.8.8.8 to capture all
// Android dns requests

IPAddress apIP(192, 169, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
WifiCredentials apCredentials("Ledstripes", "ledstripes", "");
WifiCredentials wifiCredentials("SSID", "", "PASSWORD");
AsyncWebServer server(80);

void updateWifiCredentials(class WifiCredentials toUpdate, char* source)
{
  DynamicJsonDocument doc = jsonStorage.pull(source);
  if(doc.isNull()) return;
  toUpdate.setPassword(doc["password"]);
  if(doc["ssid"] != NULL)
  toUpdate.setSSID(doc["ssid"]);
  if(doc["pid"] != NULL)
  toUpdate.setPID(doc["pid"]);
}

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

void setupWebserver()
{
  //Try to connect to online WiFi
  updateWifiCredentials(wifiCredentials, "/cfg/wifi_config.json"); //Update WiFi ("Internet") credentials
  Serial.print("Connecting wifi. Success: ");
  bool success = false;
  if(wifiCredentials.getPassword().equals(""))
  success = WiFi.begin(wifiCredentials.getSSID());
  else
  success = WiFi.begin(wifiCredentials.getSSID());
  Serial.println(success);

  //Connect to Access point according to config
  updateWifiCredentials(apCredentials, "/cfg/ap_config.json"); //Update AP mode credentials
  Serial.println("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  if(!apCredentials.getPassword().equals(""))
  WiFi.softAP(apCredentials.getSSID());
  else
  WiFi.softAP(apCredentials.getSSID(), apCredentials.getPassword());
  Serial.println("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  //Connect to DNS and MDNS according to config
  Serial.println("Starting DNS server...");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println("Registering MDNS connection for " + String(apCredentials.getPID()) + "...");
  if(!MDNS.begin(apCredentials.getPID())) Serial.print("MDNS connection failed!");

  //Register Web Pages and captive portal
  Serial.println("Registering web pages...");
  registerRootPage();
  server.addHandler(new CaptiveRequestHandler()); //Register Captive Handler

}

void onWebServer()
{
    // handle all the DNS requests
  dnsServer.processNextRequest();
}

void registerRootPage()
{
  server.serveStatic("/res", SPIFFS, "/web/res");
  server.serveStatic("/", SPIFFS, "/web/index");
}

//    API CODE


class APIRequest : public AsyncWebHandler
{
  public:
    APIRequest(String url, String paramKeys[], int paramLength)
    {
      this->paramKeys = paramKeys;
      this->paramLength = paramLength;
      this->url = url;
    }
    ~APIRequest()
    {
      delete[] this->paramKeys;
    }
    String* getParamKeys()
    {
      return paramKeys;
    }
    bool canHandle(AsyncWebServerRequest *request)
    {
      Serial.println(request->url());
      if(!request->url().equals(url)) return false;
      for(int i = 0; i < paramLength; i++)
      {
        if(!request->hasParam(paramKeys[i]))
        {
          return false;
        }
      }
      return true;
    }
    virtual void handleRequest(AsyncWebServerRequest *request) {}
  protected:
    String* paramKeys;
    String url;
    int paramLength;  
};

class LoginAPIRequest : public APIRequest
{
  public:
    LoginAPIRequest() : APIRequest("/login_wifi", new String[2]{"ssid", "password"}, 2)
    {
      Serial.println("init loginAPIRequest");
    }
    void handleRequest(AsyncWebServerRequest *request)
    {
      Serial.println("loginrequest");
    }
};


void setupAPI()
{
  Serial.println("Registering API pages...");
  registerAppAPIPages();
  server.addHandler(new LoginAPIRequest());
}


void registerAppAPIPages()
{
  server.on("/connected_to_ledstripes_network", HTTP_GET, [](AsyncWebServerRequest *request){
              AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "You are connected to an Ledstripes network");
              request->send(response);
       });

 /*
 server.on("/login_wifi", HTTP_GET, [](AsyncWebServerRequest *request){

              //Read incoming url /login_wifi?ssid=SSID&password=PASSWORD
              String* keys = new String[2]{"ssid", "password"};
              LoginAPIRequest loginRequest = LoginAPIRequest(APIRequestType::LOGIN, request, keys, 2);
              if(!loginRequest.isValidRequest()){
                //TODO: send to invalid request screen
                return;
              } 
              loginRequest.storeValues();
              onAPIRequest(&loginRequest);

              
              
              
              
              
              
              
              
              
              
              
              
              
              String ssid = "";
              String password;
              if(request->hasParam("ssid")) ssid = request->getParam("ssid")->value();
              if(request->hasParam("password")) password = request->getParam("password")->value();

              //Handle incoming url /login_wifi?ssid=SSID&password=PASSWORD
              Serial.println("SSID: '" + ssid + "' | password: '" + password + "'");
              //Redirect back to home page hence only api page 

  }); */
    
}


