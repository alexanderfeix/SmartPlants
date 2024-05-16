#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <Arduino.h>
#include <FastLED.h>

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define WIFI_SSID "FRITZ!Box ..."
#define WIFI_PASSWORD "PASSWORD"
#define INFLUXDB_URL "http://IP-ADDRESS:8086/"
#define INFLUXDB_USERNAME "admin"
#define INFLUXDB_PASSWORD "admin"
#define INFLUXDB_ORG "smart-plants"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

#define LED_PIN     5
#define NUM_LEDS    1

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG);
Point sensor("Bewegungsmelder");
String query = "SELECT mean(\"Trockenheit\") AS \"mean_Trockenheit\" FROM \"home-assistant\".\"autogen\".\"Pflanze\" WHERE time > now() - 5m AND \"device\"='ESP8266' GROUP BY time(1m) FILL(null) LIMIT 5";

void setup() {
  Serial.begin(115200);
  
  pinMode(2, INPUT);
  pinMode(5, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );

  Serial.print("Connecting..");

  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_USERNAME, INFLUXDB_PASSWORD);
  
  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  sensor.addTag("device", DEVICE);
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi disconnected");
  } else {
    Serial.println("Wifi connected");
  }
  Serial.println("Finished setup.");
}

void loop() {
  // put your main code here, to run repeatedly:
  //digitalWrite(LED_BUILTIN, digitalRead(4));
  Serial.println("Reading..");
  Serial.println(digitalRead(2));
  while (digitalRead(2) == HIGH) 
  {
    Serial.println("Bewegungsmelder aktiv.");
    FluxQueryResult result = client.query(query);
    if (result.next()) {
      long value = result.getValueByName("mean_Trockenheit").getLong();
      Serial.println(value);
      if (value < 350) {
        Serial.println("Pflanze hat genügend Wasser :)");
        leds[0] = CRGB::Green;
        FastLED.show();
      } else if (value < 450) {
        Serial.println("Pflanze hat noch genügend Wasser, demnächst gießen.");
        leds[0] = CRGB::Yellow;
        FastLED.show();
      } else {
        Serial.println("Pflanze hat zu wenig wasser :(, bitte gießen.");
        leds[0] = CRGB::Red;
        FastLED.show();
      }
      break;
    } else {
      Serial.println("Could not retrieve data from InfluxDB query.");
    }
    // Check if there was an error
    if(result.getError() != "") {
      Serial.print("Query result error: ");
      Serial.println(result.getError());
    }
    delay(1000 * 5);
  }
  
  leds[0] = CRGB::Blue;
  FastLED.show();
  
  delay(1000);
}
