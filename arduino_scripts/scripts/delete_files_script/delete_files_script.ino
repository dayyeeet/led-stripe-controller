#include <FS.h>

void setup()
{
    delay(1000);
    Serial.begin(9600);
    Serial.println();
    Serial.println("---------------------------------");
    SPIFFS.begin();
    Dir root = SPIFFS.openDir("/");
    int totalSize = 0;
    while(root.next()){
        Serial.println(root.fileName() + " | " + root.fileSize());
        totalSize = root.fileSize();
    }
    Serial.println("---------------------------------");
    Serial.println("TOTAL FILE SIZE: " + totalSize);
}

void loop()
{
  Serial.println("test");
}
