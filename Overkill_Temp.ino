#include <Arduino.h>
#undef max
#undef min
#include <Wire.h>
#include <ArduinoJson.h>   //Version 5
#include <Adafruit_BMP085.h>
#include <stdint.h>
#include <DHT.h>
#include <DHT_U.h>
#include "SparkFunHTU21D.h"
#include <SPI.h>
#include "WiFiNINA.h"
#include <U8g2lib.h>
#include "ThingSpeak.h"
#include <Time.h>
WiFiClient  client;



String apiKey = "################";
//the city you want the weather for
String location = "Savannah,US";
String CityID = "4221552";
String result;
char server[] = "api.openweathermap.org";
String weatherDescription = "";
String weatherLocation = "";
String Country;
float Temperature;
float Humidity;
float Pressure;
//https://github.com/olikraus/u8g2/wiki/fntlistall
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

//U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

HTU21D myHumidity;

char ssid[] = "&&&&&&&&&&&&";        // your network SSID (name)
char pass[] = "&&&&&&&&&&&&";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                                // your network key Index number
int status = WL_IDLE_STATUS;                     // the Wifi radio's status

float tmp;
float tmp2;
float humd;
float humdtmp;
float avgtemp;
float avghumd;
int kPa;
float f;
float bmptemp;
float lastavgtemp;
float lastavghumd;


const int buttonPin = 7;
int buttonState;             // the current reading from the input pin
int lastButtonState = 0;


const char DEGREE_SYMBOL[] = { 0xB0, '\0' };
const char WIFI_SYMBOL[] = { 0xf8, '\0' };
const char UP_SYMBOL[] = { 0x53, '\0' };

//#define dht_apin A6
#define dht_apin 6
#define DHTTYPE    DHT11     // DHT 11
DHT_Unified dht(dht_apin, DHTTYPE);


Adafruit_BMP085 bmp;


unsigned long myChannelNumber = 991346;
const char * myWriteAPIKey = "&&&&&&&&&&&&&";

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  u8g.begin();
  u8g.clearBuffer();
  u8g.setFont(u8g2_font_crox5h_tr);
  u8g.setCursor(0, 30);
  u8g.print("Loading...");
  u8g.sendBuffer();

  Connect_To_Wifi();
  ThingSpeak.begin(client);  //Initialize ThingSpeak

  //Start Sensors
  dht.begin();        //DHT
  myHumidity.begin(); //HTU21D
  bmp.begin();        //BMP-085

  delay(1000);

  getWeather();
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void loop() {
  GetSensorData();
  lastavgtemp = avgtemp;
  lastavghumd = avghumd;

  delay(2000);

  GetSensorData();
  Display_Temp();
  delay(5000);
  long rssi = WiFi.RSSI();
  Serial.print("Status:");
  Serial.println(status);
  
  if ((status == WL_CONNECTION_LOST) || (status == WL_DISCONNECTED) || (status == WL_CONNECT_FAILED) || (rssi = 0))
  { 
      resetFunc();  //call reset
  }

  //Update ThingSpeak only when temp or humidity changes
  if ((round(avgtemp) - round(lastavgtemp) != 0) || (round(avghumd) - round(lastavghumd) != 0))
  {
    Display_Temp();
    TS_Send();
  }


}

////////////////////////FUNCTIONS////////////////////////////////
void Connect_To_Wifi()
{
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("Attempting to connect to WEP network, SSID: ");
    Serial.println(ssid);

    status = WiFi.begin(ssid, keyIndex, pass);

    // wait 10 seconds for connection:
    delay(1000);
  }
  // once you are connected :
  Serial.print("You're connected to the network");
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

}


void GetSensorData()
{
  //DHT Temp & Humidity
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  tmp = event.temperature;
  dht.humidity().getEvent(&event);
  humdtmp = event.relative_humidity;
  tmp = (tmp * 9 / 5) + 32;

  //HTU21D
  humd = myHumidity.readHumidity();
  tmp2 = (myHumidity.readTemperature() * 9 / 5) + 32;

  //BMP085
  bmptemp = bmp.readTemperature();
  f = (bmptemp * 9 / 5) + 32; //BMP

  //Calc Averages
  avghumd = (humdtmp + humd) / 2;
  avgtemp = (tmp + f + tmp2) / 3;


  Serial.print("DHT Humidity = ");
  Serial.print(humdtmp);
  Serial.println("% ");

  Serial.print("HTU21D Humidity ");
  Serial.print(humd);
  Serial.println("%  ");

  Serial.print("DHT Temp ");
  Serial.print(tmp);
  Serial.println("F  ");

  Serial.print("HTU21D Temp ");
  Serial.print(tmp2);
  Serial.println("F  ");

  Serial.print("BMP Temp ");
  Serial.print(f);
  Serial.println(" F");

  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");

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
}



void Display_Temp()
{
  u8g.firstPage();
  do {
    //keep this the same as it pages through the data generating the screen

    u8g.setFont(u8g2_font_crox5h_tr);  //Pixel Height 17
    u8g.setCursor(0, 20);
    u8g.print("T: ");
    u8g.print(float(avgtemp), 1);
    u8g.setFont(u8g2_font_fur17_tf);
    u8g.print(DEGREE_SYMBOL);
    u8g.setFont(u8g2_font_crox5h_tr);
    u8g.print(" F");

    u8g.setCursor(0, 45);
    u8g.print("H: ");
    u8g.print(round(avghumd));
    u8g.print("%");


    kPa = bmp.readPressure() / 1000;
    long rssi = WiFi.RSSI();
    u8g.setFont(u8g_font_7x13);
    u8g.setCursor(92, 64);
    u8g.print(rssi);
    u8g.print("dB");

    u8g.setFont(u8g2_font_open_iconic_all_1x_t);
    u8g.setCursor(0, 64);
    u8g.print(WIFI_SYMBOL);


  } while ( u8g.nextPage() );
  u8g.sendBuffer();
}

void getWeather()

{
  if (client.connect(server, 80))
  { //starts client connection, checks for connection
    client.println("GET /data/2.5/weather?id=" + CityID + "&units=metric&APPID=" + apiKey);
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
  }
  else {
    Serial.println("connection failed");        //error message if no client connect
    Serial.println();
  }

  while (client.connected() && !client.available())
    delay(1);                                          //waits for data
  while (client.connected() || client.available())
  { //connected or data available
    char c = client.read();                     //gets byte from ethernet buffer
    result = result + c;
  }

  client.stop();                                      //stop client
  result.replace('[', ' ');
  result.replace(']', ' ');
  Serial.println(result);
  char jsonArray [result.length() + 1];
  result.toCharArray(jsonArray, sizeof(jsonArray));
  jsonArray[result.length() + 1] = '\0';
  StaticJsonBuffer<1024> json_buf;
  JsonObject &root = json_buf.parseObject(jsonArray);
  Serial.print(jsonArray);
  if (!root.success())
  {
    Serial.println("parseObject() failed");
  }

  String location = root["name"];
  String country = root["sys"]["country"];
  float temperature = root["main"]["temp"];
  float humidity = root["main"]["humidity"];
  String weather = root["weather"]["main"];
  String description = root["weather"]["description"];
  float pressure = root["main"]["pressure"];
  temperature = (temperature * 9 / 5) + 32;
  Serial.println(temperature);
  Serial.println(humidity);
  Serial.println(weather);
  Serial.println(description);
  Serial.println(pressure);
  Serial.println(location);
  Serial.println(country);
}

void TS_Send()
{
  long rssi = WiFi.RSSI();
  ThingSpeak.setField(1, avgtemp);
  ThingSpeak.setField(2, round(avghumd));
  ThingSpeak.setField(3, rssi);
  ThingSpeak.setField(4, bmp.readAltitude(101500));
  ThingSpeak.setField(5, round(tmp));
  ThingSpeak.setField(6, round(tmp2));
  ThingSpeak.setField(7, round(f));


  ThingSpeak.setStatus("");
  // write to the ThingSpeak channel

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("Channel update successful.");
    u8g.setFont(u8g2_font_open_iconic_all_1x_t);
    u8g.setCursor(10, 64);
    u8g.print(UP_SYMBOL);
    u8g.sendBuffer();
  }
  else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
    Serial.print(String(x));
    Connect_To_Wifi;
  }
  delay(1000);
  u8g.setFont(u8g2_font_open_iconic_all_1x_t);
  u8g.setCursor(10, 64);
  u8g.print("*");
  u8g.sendBuffer();

}
