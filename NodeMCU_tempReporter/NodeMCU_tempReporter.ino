#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <ArduinoHttpClient.h>
#include <TimeLib.h>
#include "secrets.h"

const char server[] { "displayhometemp.herokuapp.com" };

WiFiClient wifi;
HttpClient client = HttpClient(wifi, server, 80);
WiFiUDP Udp;
NTPClient timeClient(Udp, "europe.pool.ntp.org");
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
  Serial.print("epoch - ");
  Serial.println(epoch);
  
  static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

  //"Mon, 09 Mar 2020 08:13:24 GMT"
  snprintf(buff, size, "%s, %02d %s %04d %s GMT", 
         days[weekday(epoch)-1], day(epoch), months[month(epoch)-1], year(epoch), timeClient.getFormattedTime().c_str()
         );
}

void getAndSendData(int tries, unsigned long delayStep)
{
  Serial.println(WiFi.status());
  
  if (WiFi.status() != WL_CONNECTED)
    WiFi.reconnect();
  
  static char buff[30] = {0};
  toUtcString(buff, sizeof(buff));

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  
  char data[100] = {0};
  snprintf(data, sizeof(data), "{\"temperature\":%.1f,\"humidity\":%.1f,\"timestamp\":\"%s\"}",
           temperature, humidity, buff);

  Serial.println(data);
  
  unsigned long delaySumm = 0ull;
  for (int i = 0; i <= tries; i++) {
    client.beginRequest();
    client.post("/api/temperature");
    client.sendHeader("Content-Type", "application/json");
    client.sendHeader("Content-Length", strlen(data));
    client.beginBody();
    Serial.println(client.print(data));
    client.endRequest();
  
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();
  
    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);
  
    if (statusCode == 200) {
      return;
    }

    delay(delaySumm + delayStep);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(1000000);
  Serial.println();

  Serial.print(SSID);
  Serial.print(' ');
  Serial.println(PASS);

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
  dht.setup(13, DHTesp::DHT11);

  delay(dht.getMinimumSamplingPeriod());
}

void loop() {
  Serial.println("--");
  
  getAndSendData(4, 5ull * 1000);
  
  Serial.printf("free heap: %d | frag: %d | max block: %d\n\n\n",
    ESP.getFreeHeap(),
    ESP.getHeapFragmentation(),
    ESP.getMaxFreeBlockSize()
    );
  
  delay(60*60*1000);
}
