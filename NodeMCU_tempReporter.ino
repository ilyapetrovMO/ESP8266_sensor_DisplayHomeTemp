#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <ArduinoHttpClient.h>
#include <TimeLib.h>
#include "secrets.h"

const char server[] { "" };

WiFiClient wifi;
HttpClient client = HttpClient(wifi, server, 80);
WiFiUDP Udp;
NTPClient timeClient(Udp);
DHTesp dht;

void blink(int a)
{
  for (int i = 0; i <= a; i++)
  {
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
  }
}

void toUtcString(char* buff, size_t size)
{
  timeClient.update();
  unsigned long epoch = timeClient.getEpochTime();
  
  static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

  //"Mon, 09 Mar 2020 08:13:24 GMT"
  snprintf(buff, size, "%s, %02d %s %04d %s GMT", 
         days[weekday(epoch)-1], day(epoch), months[month(epoch)-1], year(epoch), timeClient.getFormattedTime().c_str()
         );
}

void getAndSendData()
{
  static char buff[30] = {0};
  toUtcString(buff, sizeof(buff));

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  
  humidity = humidity > 99.f ? 99.f : humidity;
  temperature = temperature > 99.f ? 99.f : temperature;
  
  char data[82] = {0};
  snprintf(data, sizeof(data), "{\"temperature\":%4.1f,\"humidity\":%4.1f,\"timestamp\":\"%s\"}",
           temperature, humidity, buff);

  Serial.println(data);

  client.beginRequest();
  client.post("/api/temperature");
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", sizeof(data)/sizeof(char)-2); //-2 is something I dont understand
  client.beginBody();
  client.print(data);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  Serial.println();

  WiFi.begin(SSID, PASS);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  blink(3);

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  dht.setup(2, DHTesp::DHT11);
}

void loop() {
   getAndSendData();
   delay(60*60*1000);
}
