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
  setupWebserver();
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
        DynamicJsonDocument wrapped(1024);
        deserializeJson(wrapped, content);
        if(wrapped["type"] != packetType)
        return DynamicJsonDocument(1024);
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

JSONStorage jsonStorage();

const int AMOUNT_OF_LISTENER_TYPES = 5;
int listenerArrayIndex = 0;
JSONPacketListener *listenerArray[AMOUNT_OF_LISTENER_TYPES];

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

    for(int i = 0; i < AMOUNT_OF_LISTENER_TYPES; i++)
    {
      JSONPacketListener* listener = listenerArray[i];
      JSONPacketVerifier verifier = JSONPacketVerifier(listener);
      DynamicJsonDocument correctDocument = verifier.unwrap(content);
      if(correctDocument != NULL)
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

// The access points IP address and net mask
// It uses the default Google DNS IP address 8.8.8.8 to capture all
// Android dns requests
IPAddress apIP(192, 169, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
WifiCredentials credentials("Ledstripes", "ledstripes", "");

AsyncWebServer server(80);

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

void setupWebserver()
{
  Serial.println("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  // its an open WLAN access point without a password
  if(!credentials.getPassword().equals(""))
  WiFi.softAP(credentials.getSSID());
  //its an open WLAN access point with password
  else
  WiFi.softAP(credentials.getSSID(), credentials.getPassword());
  Serial.println("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Starting file system...");
  SPIFFS.begin();
  Serial.println("Starting DNS server...");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println("Registering MDNS connection for " + String(credentials.getPID()) + "...");
  if(!MDNS.begin(credentials.getPID())) Serial.print("MDNS connection failed!");
  Serial.println("Registering web pages...");
  registerRootPage();
  server.onNotFound([](AsyncWebServerRequest *request) {
        captivePortal(request);
  });
  setupAPI();
  Serial.println("Starting server...");
  server.begin();
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
    response->addHeader("Location", String("http://") + toStringIp(apIP));
    request->send(response);
    return true;
  }
  return false;
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

//    API CODE

enum APIRequestType {LOGIN};

class APIRequest 
{
  public:
    APIRequest(APIRequestType requestType, AsyncWebServerRequest *request, String paramKeys[], int paramLength)
    {
      this->request = request;
      this->requestType = requestType;
      this->paramKeys = paramKeys;
      this->paramValues = new String[paramLength];
      this->paramLength = paramLength;
    }
    ~APIRequest()
    {
      delete[] this->paramValues;
      delete[] this->paramKeys;
    }
    String* getParamKeys()
    {
      return paramKeys;
    }
    bool isValidRequest()
    {
      for(int i = 0; i < paramLength; i++)
      {
        if(!request->hasParam(paramKeys[i]))
        {
          return false;
        }
      }
      return true;
    }
    void storeValues()
    {
      for(int i = 0; i < paramLength; i++)
      {
        this->paramValues[i] = request->getParam(this->paramKeys[i])->value();
      }
    }
    String getParamValue(String paramName)
    {
      for(int i = 0; i < sizeof(this->paramKeys); i++)
      {
        if(this->paramKeys[i].equals(paramName)) return this->paramValues[i];
      }
      return String();
    }
    String* getParamValues()
    {
      return paramValues;
    }
    APIRequestType getRequestType()
    {
      return requestType;
    }
  protected:
    AsyncWebServerRequest *request;
    String* paramKeys;
    String* paramValues;
    APIRequestType requestType;
    int paramLength;  
};

class LoginAPIRequest : public APIRequest
{
  public:
    using APIRequest::APIRequest;
    bool checkIfValidData()
    {
      //TODO: Check if can login to this certain wifi
      Serial.println("SSID: '" + this->getParamValue("ssid") + "' | password: '" + this->getParamValue("password") + "'");
      return true;
    }
};


void setupAPI()
{
  Serial.println("Registering API pages...");
  registerBasicAPIPages();
}

void onAPIRequest(class APIRequest* request)
{
  if(request->getRequestType() == APIRequestType::LOGIN)
  {
    LoginAPIRequest &loginRequst = *static_cast<LoginAPIRequest*>(request);

  }
}

void registerBasicAPIPages()
{
  server.on("/connected_to_ledstripes_network", HTTP_GET, [](AsyncWebServerRequest *request){
              AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "You are connected to an Ledstripes network");
              request->send(response);
       });

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

              
              
              
              
              
              
              
              
              
              
              
              
              
              /*String ssid = "";
              String password;
              if(request->hasParam("ssid")) ssid = request->getParam("ssid")->value();
              if(request->hasParam("password")) password = request->getParam("password")->value();

              //Handle incoming url /login_wifi?ssid=SSID&password=PASSWORD
              Serial.println("SSID: '" + ssid + "' | password: '" + password + "'");
              //Redirect back to home page hence only api page */

  });       
}


