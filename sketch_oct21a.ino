#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <SensirionI2cScd4x.h>
#include <Wire.h>

// ===== WiFi CONFIG =====
const char* ssid = "jing";
const char* wifi_password = "jingmcse";

// ===== MQTT CONFIG =====
const char* mqtt_server = "19908b6cb7054952800839917a2701c3.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "sensorco2";
const char* mqtt_pass = "IDontknow123";
const char* mqtt_topic = "school/room1/co2";

// ===== SENSOR =====
SensirionI2cScd4x scd4x;

// ===== OBJECTS =====
WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

void connectWiFi() {
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to HiveMQ Cloud...");
    if (client.connect("ESP32S3_Client", mqtt_user, mqtt_pass)) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

#ifdef PIN_I2C_POWER
  pinMode(PIN_I2C_POWER, OUTPUT);
  digitalWrite(PIN_I2C_POWER, HIGH);
#endif

  Wire.begin();
  scd4x.begin(Wire, 0x62);

  uint16_t error;
  error = scd4x.stopPeriodicMeasurement();
  delay(500);
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.println("SCD4x start failed!");
  } else {
    Serial.println("SCD4x measurement started.");
  }

  connectWiFi();
  wifiClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) connectMQTT();
  client.loop();

  bool ready = false;
  if (scd4x.getDataReadyStatus(ready) == 0 && ready) {
    uint16_t co2;
    float temp, rh;
    if (scd4x.readMeasurement(co2, temp, rh) == 0 && co2 != 0) {
      char payload[128];
      snprintf(payload, sizeof(payload),
               "{\"co2\":%u,\"temp\":%.2f,\"rh\":%.2f}", co2, temp, rh);

      client.publish(mqtt_topic, payload);
      Serial.printf("Published: %s\n", payload);
    }
  }
  delay(5000);
}
