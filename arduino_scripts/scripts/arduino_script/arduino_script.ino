#include <ArduinoJson.h>
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <SoftwareSerial.h>

#define rx 10
#define tx 11

SoftwareSerial ESP(rx, tx);


#define DATA_PIN 5

int led_count = 45;

int led_resolution = 3;

int color_array_length = led_count / led_resolution;

#define BRIGHTNESS_PERCENT 255/100
int brightness = 10;

CRGB *leds = NULL;

long *color_hex = new long[1]{0xFFFFFF};

long global_color = 0x000000;

String wifi_ssid("Ledstripestest");
String wifi_password("");

void setup() {
  ESP.begin(9600);
  Serial.begin(9600);
  updateArrays();
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, led_count);
  handleESPOutgoingPacket("wificonfig");
  handleESPOutgoingPacket("stripeconfig");
  handleESPOutgoingPacket("colorconfig"); 
  updateCustomProperties();
}

void updateArrays()
{
  if(led_resolution < 1) led_resolution = 1;
  if(led_count < 1) led_count = 1;
  color_array_length = led_count / led_resolution;
  
  if(leds == NULL){
    leds = new CRGB[led_count];
  }
  if(color_hex != 0)
  {
    delete [] color_hex;
  }
  color_hex = new long[color_array_length];
  for(int i = 0; i < color_array_length; i++)
  {
        color_hex[i] = global_color;
  }
  
}


void updateColors(long globalColor, long* colors, int colorLength)
{
  global_color = globalColor;
  for(int i = 0; i < colorLength; i++)
  {
    color_hex[i] = colors[i] != 0 ? colors[i] : global_color;
  }
  calculateLEDColors();
  handleESPOutgoingPacket("colorconfig");
}

void updateCustomProperties()
{
  FastLED.setBrightness(getTranslatedBrightness());
  calculateLEDColors();
}

int getTranslatedBrightness()
{
  return (int) (BRIGHTNESS_PERCENT * brightness);
}

void loop() {
  onESPIncomingPacket();
  onIncomingPacket();
  FastLED.show();
}

void onIncomingPacket()
{
  while(Serial.available() > 0)
  {
    String line = Serial.readStringUntil('\n');
    ESP.println(line);
    if(line.startsWith("CHANGED "))
    {
      String command = line;
      command.replace("CHANGED ", "");
      
      String type = split(command, 0);
     
      String arguments = command;
      arguments.replace(type + " ", "");
      handleESPIncomingPacket(type, arguments);
    }else if(line.startsWith("REQUEST "))
    {
      String command = line;
      command.replace("REQUEST ", "");
      String type = split(command, 0);
      Serial.println(type);
      handleESPOutgoingPacket(type);
    }
  }
}

void onESPIncomingPacket()
{
  while(ESP.available() > 0)
  {
    String line = ESP.readStringUntil('\n');
    Serial.println(line);
    if(line.startsWith("CHANGED ") || line.indexOf("colorconfig") >= 0)
    {
      String command = line;
      if(line.startsWith("CHANGED "))
      command.replace("CHANGED ", "");
      else
      command = line.substring(line.indexOf("colorconfig"), line.length());
      String type = split(command, 0);
     
      String arguments = command;
      arguments.replace(type + " ", "");
      handleESPIncomingPacket(type, arguments);
    }else if(line.startsWith("REQUEST "))
    {
      String command = line;
      command.replace("REQUEST ", "");
      String type = split(command, 0);
      Serial.println(type);
      handleESPOutgoingPacket(type);
    }
  }
}


void handleESPOutgoingPacket(String type)
{
   if(type.equals("wificonfig"))
   {
      ESP.print("WIFICONFIG ");
      ESP.print(toPacketArgument(wifi_ssid));
      ESP.print(' ');
      ESP.println(toPacketArgument(wifi_password));

      Serial.print("WIFICONFIG ");
      Serial.print(toPacketArgument(wifi_ssid));
      Serial.print(' ');
      Serial.println(toPacketArgument(wifi_password));
   }
   else if(type.equals("stripeconfig"))
   {
      ESP.print("STRIPECONFIG ");
      ESP.print(DATA_PIN);
      ESP.print(' ');
      ESP.print(led_count);
      ESP.print(' ');
      ESP.println(String(led_resolution));

      Serial.print("STRIPECONFIG ");
      Serial.print(DATA_PIN);
      Serial.print(' ');
      Serial.print(led_count);
      Serial.print(' ');
      Serial.println(String(led_resolution));
   }
   else if(type.equals("colorconfig"))
   {
      /*Serial.print("COLORCONFIG ");
      Serial.print(toHexString(global_color));
      for(int i = 0; i < color_array_length - 1; i++)
      {
        //Serial.print(' ');        
        //Serial.print(toHexString(color_hex[i]));
      }
      //Serial.println(toHexString(color_hex[color_array_length - 1])); */
   } 
}

void handleESPIncomingPacket(String type, String arguments)
{
  if(type.equals("wificonfig"))
  {
    wifi_ssid = fromPacketArgument(split(arguments, 0));
    wifi_password = fromPacketArgument(split(arguments, 1));
  }
  else if(type.equals("stripeconfig"))
  {
    led_resolution = fromPacketArgument(split(arguments, 0)).toInt();
    updateArrays();
    calculateLEDColors();
  }
  else if(type.equals("colorconfig"))
  {
    brightness = fromPacketArgument(split(arguments, 0)).toInt();
    global_color = toHexLong(fromPacketArgument(split(arguments, 1)));
    long* added_values = new long[color_array_length];
    for(int i = 0; i < color_array_length; i++)
    {
      added_values[i] = toHexLong(fromPacketArgument(split(arguments, i + 2)));
    }
    updateColors(global_color, added_values, color_array_length);
    updateCustomProperties();
  }
}



void calculateLEDColors()
{
  color_array_length = led_count / led_resolution;
    for(int i = 0; i < color_array_length; i++)
    {
      int start = i * led_resolution;
      for(int row = 0; row < led_resolution; row++)
      {
        if(color_hex[i] == 0) color_hex[i] = global_color;
        leds[start + row] = color_hex[i];
      }
    }
}



//    UTIL CODE

String toHexString(long hexInt)
{
  char toReturn[6];  
  ltoa(hexInt,toReturn,16);
  return "#" + String(toReturn);
}

long toHexLong(String hexString)
{
  String hexStringClean = hexString;
  hexStringClean.replace("#", "");
  return strtol(hexStringClean.c_str(), NULL, 16);
}

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
