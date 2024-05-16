#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define WIFI_SSID "FRITZ!Box 7530 SW"
#define WIFI_PASSWORD "PASSWORD"
#define INFLUXDB_URL "http://192.168.178.80:8086/"
#define INFLUXDB_USERNAME "home-assistant"
#define INFLUXDB_PASSWORD "thereturnnothing"
#define INFLUXDB_ORG "home-assistant"

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG);

Point sensor("Pflanze");

const double aridityPin = A0;
const int anyPin = 3;


void setup() {
  Serial.begin(115200);

  Serial.print("Connecting..");

  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_USERNAME, INFLUXDB_PASSWORD);

  pinMode(aridityPin, INPUT);
  pinMode(anyPin, INPUT);

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

}


void loop() {
  for (int i = 0; i < 5; i++){
    int aridityValue = analogRead(aridityPin);
    sensor.addField("Trockenheit", aridityValue);
    Serial.println(aridityValue);
    if (!client.writePoint(sensor)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }
    sensor.clearFields();
    delay(1000 * 6);
  }

  ESP.deepSleep(30e6);
}
