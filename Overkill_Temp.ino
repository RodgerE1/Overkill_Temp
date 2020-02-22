#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <stdint.h>
#include <DHT.h>
#include <DHT_U.h>
#include "SparkFunHTU21D.h"
#include <SPI.h>
#include "WiFiNINA.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include "ThingSpeak.h"

WiFiClient  client;

//#include "arduino_secrets.h" 
//https://github.com/olikraus/u8g2/wiki/fntlistall
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

//U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

HTU21D myHumidity;

char ssid[] = "&&&&&&&&&&";        // your network SSID (name)
char pass[] = "&&&&&&&&&&";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                                // your network key Index number
int status = WL_IDLE_STATUS;                     // the Wifi radio's status

float tmp;
float tmp2;
float humd;
float avgtemp;
float avghumd;
int kPa;
float f;


//#define dht_apin A6
#define dht_apin 6
#define DHTTYPE    DHT11     // DHT 11
DHT_Unified dht(dht_apin,DHTTYPE);


Adafruit_BMP085 bmp;

 
//const char* host = "api.thingspeak.com";
//const char* myWriteAPIKey = "&&&&&&&&&&&&&&";
//const int httpPort = 80;
unsigned long myChannelNumber = #######;
const char * myWriteAPIKey = "&&&&&&&&&&&&&&&&";
  
void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  ThingSpeak.begin(client);  //Initialize ThingSpeak
  u8g.begin();
  u8g.clearBuffer(); 
  u8g.setFont(u8g2_font_crox5h_tr);
  u8g.setCursor(10,30);
  u8g.print("Loading...");
  u8g.sendBuffer();
  dht.begin();
  myHumidity.begin();   
  
  Connect_To_Wifi();

  

    
  if (!bmp.begin()) {
	Serial.println("Could not find a valid BMP085 sensor, check wiring!");
	while (1) {}
  }
  delay(1000);


}

void Connect_To_Wifi()
{
    // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("Attempting to connect to WEP network, SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, keyIndex, pass);
    digitalWrite(LED_BUILTIN, HIGH);
    u8g.sendBuffer();
    // wait 10 seconds for connection:
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
      }
      // once you are connected :
      digitalWrite(LED_BUILTIN, LOW);
  Serial.print("You're connected to the network");
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  }





  
void loop() {
  u8g.clearBuffer();  

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    tmp = event.temperature;
    Serial.println(F("°C"));
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    avghumd = event.relative_humidity;
    Serial.println(F("%"));
  }

  
  humd = myHumidity.readHumidity();
  tmp2 = (myHumidity.readTemperature() * 9/5) + 32;
  tmp = (tmp * 9 / 5) + 32;
  f = (bmp.readTemperature() * 9/5) + 32;
  avghumd = (avghumd + humd)/2;
  avgtemp = (tmp + f + tmp2)/3;  
  

    Serial.print("DHT Humidity = ");
    Serial.print(avghumd);
    Serial.println("% ");

    Serial.print("HTU21D Humidity ");
    Serial.print(myHumidity.readHumidity());
    Serial.println("%  ");
    
    Serial.print("DHT Temp ");
    Serial.print(tmp);
    Serial.println("F  ");

    Serial.print("HTU21D Temp ");
    Serial.print(tmp2);
    Serial.println("F  ");

    Serial.print("BMP Temp ");
    Serial.print(f);
    Serial.println(" *F");
    
    Serial.print("Pressure = ");
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");
    
    // Calculate altitude assuming 'standard' barometric
    // pressure of 1013.25 millibar = 101325 Pascal
    Serial.print("Altitude = ");
    Serial.print(bmp.readAltitude());
    Serial.println(" meters");

    Serial.print("Pressure at sealevel (calculated) = ");
    Serial.print(bmp.readSealevelPressure());
    Serial.println(" Pa");

  
    Serial.print("Real altitude = ");
    Serial.print(bmp.readAltitude(101500));
    Serial.println(" meters");
    
    Serial.println();

 Display_Temp();


  ThingSpeak.setField(1, avgtemp);
  ThingSpeak.setField(2, round(avghumd));
  ThingSpeak.setField(3, kPa);
  ThingSpeak.setField(4, bmp.readAltitude(101500));
  ThingSpeak.setStatus("");
  // write to the ThingSpeak channel

  int x = ThingSpeak.writeFields(myChannelNumber,myWriteAPIKey);
  if(x == 200){
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
  digitalWrite(LED_BUILTIN, LOW);
  delay(45000);
}

void Display_Temp()
{
    u8g.firstPage();    
  do {
    //keep this the same as it pages through the data generating the screen
  
      u8g.setFont(u8g2_font_crox5h_tr);
      u8g.setCursor(0,20);
      u8g.print("T: ");
      u8g.print(float(avgtemp),1);
      u8g.print(" F");  

      u8g.setCursor(0,45);
      u8g.print("H: ");
      u8g.print(round(avghumd));
      u8g.print("%");

      u8g.setFont(u8g_font_7x13);
      kPa = bmp.readPressure()/1000;
   

      u8g.setCursor(0,64);
      u8g.print("Pressure: ");
      u8g.print(kPa);
      u8g.println(" kPa");
      
  } while ( u8g.nextPage() );
  u8g.sendBuffer();
}
