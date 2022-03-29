#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include "config.h"
    
#define DHTPIN        2
#define DHTTYPE       DHT22
#define RELAY         4


const char* resource  = "xxx"; 
const char* server    = "maker.ifttt.com";

AdafruitIO_Feed *klijalisteTemp = io.feed("klijalisteTemp");
AdafruitIO_Feed *klijalisteVlaga = io.feed("klijalisteVlaga");
AdafruitIO_Feed *onoff = io.feed("onoff");

int emailSent = 0;
int grijalica = 0;

unsigned long currentMillis = 0; 
unsigned long grijalicaTimer = 0; 


DHT dht(DHTPIN, DHTTYPE);


void setup()
{
  
  Serial.begin(9600);
  dht.begin();
  pinMode(4, OUTPUT);
  delay(3000);
  grijalica = 0;
  onoff->save(grijalica);
  digitalWrite(RELAY, LOW);

     // connecting to io.adafruit.com
    Serial.print("Connecting to Adafruit IO");
    io.connect();
  
    // wait for a connection
    while(io.status() < AIO_CONNECTED) {
      Serial.print(".");
      delay(500);
    }

    // connected to io.adafruit.com
    Serial.println();
    Serial.println(io.statusText());
}

void loop(){

  io.run();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print("\nGrijalica:");
  Serial.println(grijalica);
  Serial.println("\n");
  
  if (t < 9) {
    digitalWrite(RELAY, HIGH);
    if (grijalica == 0){
      makeIFTTTRequest();
      grijalica = 1;
      onoff->save(grijalica);
      grijalicaTimer = millis();
      delay(180000); //3 mins minimum heating
    }
    else {
      if ((millis() - grijalicaTimer) > 2400000){ // temp heater shutdown after 40min for 20mins cooling
        digitalWrite(RELAY, LOW);
        grijalica = 0;
        onoff->save(grijalica);
        Serial.println("Cooling the heating element.\n\n");
        makeIFTTTRequest();
        delay(1200000); //20mins cooling
        grijalicaTimer = millis();
      }
    }
  }
  else {
    digitalWrite(RELAY, LOW);
    if (grijalica == 1){
      makeIFTTTRequest();
      grijalica = 0;
      onoff->save(grijalica);
    }
  }

  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT22 sensor!");
    delay (1000);
    return;
  }


  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.println(" *C ");
  Serial.println();

  
  
  klijalisteTemp->save(t);
  delay(2000);
  klijalisteVlaga->save(h);   
  delay(10000);

  Serial.println ("*******");
  Serial.println (millis());
  Serial.println (currentMillis);
  Serial.println (emailSent);
  Serial.println ("*******");
  
  if (millis() - currentMillis > 7200000) {
    emailSent == 0;
  }
}

void makeIFTTTRequest() {
  Serial.print("Connecting to: "); 
  Serial.print(server);
  
  WiFiClient client;
  int retries = 5;
  while(!!!client.connect(server, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if(!!!client.connected()) {
    Serial.println("Failed to connect...");
  }
  
  Serial.print("Request resource: "); 
  Serial.println(resource);

  int grijalicaStatus;
  
  if ((digitalRead (RELAY)) == 1){
    grijalicaStatus = 1;
  }
  else {
    grijalicaStatus = 0;
  }
  
  String jsonObject = String("{\"value1\":\"") + dht.readTemperature() + "\",\"value2\":\"" + dht.readHumidity()
                      + "\",\"value3\":\"" + grijalicaStatus + "\"}";
                      
  client.println(String("POST ") + resource + " HTTP/1.1");
  client.println(String("Host: ") + server); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);
        
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
    Serial.println("No response...");
  }
  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\n... Closing Connection ...");
  Serial.println("_____________________");
  client.stop(); 

}

/*
void checkWiFi() {
  
  if (WiFi.status() == 3) {
      Serial.println("ESP8266 connected to:");
      Serial.println(ssid);
      Serial.println(io.statusText());
      Serial.println();
  }

  if (WiFi.status() == 6 || WiFi.status() == 1 || WiFi.status() == 4 || WiFi.status() == 5){
      makeIFTTTRequest();
      Serial.println("WIRELESS CONNECTION LOST!");
      //Serial.println("restarting device...!");
      //ESP.restart();
      Serial.println("trying to reconnect...!");
      WiFi.begin(ssid, password);
      delay(5000);
      Serial.print("Now reConnecting to Adafruit IO");
      io.connect();
    
      // wait for a connection
      while(io.status() < AIO_CONNECTED) {
        Serial.print(".");
        delay(500);
      }  
      // connected to io.adafruit.com
      Serial.println();
      Serial.println(io.statusText());
      delay(10000); // delay shouldn't be too small because WiFi doesn't have the time to reconnect!
      checkWiFi();
  }
}
*/
