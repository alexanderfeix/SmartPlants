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

#define WIFI_SSID "FRITZ!Box ..."
#define WIFI_PASSWORD "PASSWORD"
#define INFLUXDB_URL "http://IP-ADDRESS:8086/"
#define INFLUXDB_TOKEN "220382fec65c97c5de558038b46ef4d87bdb720032cc1908fa5891e5ec345733"
#define INFLUXDB_ORG "smart-plants"
#define INFLUXDB_BUCKET "plants"

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Point sensor("Monstera Deliciosa");

const double aridityPin = A0;
const int anyPin = 3;


void setup() {
  Serial.begin(115200);

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
  Serial.println();

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
}

void loop() {
  sensor.clearFields();

  int aridityValue = analogRead(aridityPin);
  Serial.println(aridityValue);
  
  sensor.addField("Trockenheit", aridityValue);

  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
 
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  delay(2000);
}
