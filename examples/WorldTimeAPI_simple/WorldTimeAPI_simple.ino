#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "This board is not supported."
#endif

#include "DateTime.h"
#include "WorldTimeAPI.h"

WorldTimeAPIResult res;
void setup() {
  Serial.begin(115200);

  Serial.println();

  WiFi.begin("ssid", "password"); //Set SSID and password of your WiFi

  //Connecting to WiFi
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  WorldTimeAPI api;
  uint32_t t1 = micros();
  res = api.getByIP();
  t1 = micros() - t1;
  
  if(res.hasError()){
    //Some error happened
    Serial.print("Error occured: ");
    Serial.println((int)res.httpCode);
  }
  else{
    //Operation was successful
    Serial.print("Took: ");
    Serial.print(t1);
    Serial.println("us");
    
    Serial.print("Timezone: ");
    Serial.println(res.timezone);
    
    Serial.print("Abbreviation: ");
    Serial.println(res.abbreviation);
    
    Serial.print("Client IP: ");
    Serial.println(res.client_ip.toString());
    
    //Printing time every 1 second
    //Time may not be accurate - to obtain accurate time, use ntp server.
    while(true){
      Serial.println(res.datetime.toString()); //res.datetime also contains time zone and DST offsets
      delay(1000);
    }
  }
}