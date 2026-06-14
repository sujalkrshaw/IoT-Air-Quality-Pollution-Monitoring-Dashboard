#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ================= WiFi =================
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ================= MQTT =================
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
SCREEN_WIDTH,
SCREEN_HEIGHT,
&Wire,
-1
);

// ================= DHT22 =================
#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// ================= Pins =================
#define MQ135_PIN 34
#define LED_PIN 26
#define BUZZER_PIN 27

String airStatus;

// ================= WiFi =================
void setupWiFi() {

Serial.print("Connecting WiFi");

WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}

Serial.println();
Serial.println("WiFi Connected");
}

// ================= MQTT =================
void reconnectMQTT() {

while (!client.connected()) {

Serial.print("Connecting MQTT...");

String clientId =
  "ESP32-AQI-" +
  String(random(1000, 9999));

if (client.connect(clientId.c_str())) {

  Serial.println("Connected");

} else {

  Serial.print("Failed, rc=");
  Serial.println(client.state());

  delay(3000);
}

}
}

void setup() {

Serial.begin(115200);

pinMode(LED_PIN, OUTPUT);
pinMode(BUZZER_PIN, OUTPUT);

digitalWrite(LED_PIN, LOW);

dht.begin();

Wire.begin(21, 22);

if (!display.begin(
SSD1306_SWITCHCAPVCC,
0x3C)) {

Serial.println("OLED NOT FOUND");

while (true);

}

display.clearDisplay();
display.setTextColor(WHITE);

display.setTextSize(2);
display.setCursor(20, 20);
display.println("AQI");

display.display();

setupWiFi();

client.setServer(
mqtt_server,
1883
);

delay(2000);
}

void loop() {

if (!client.connected()) {
reconnectMQTT();
}

client.loop();

int airValue = analogRead(MQ135_PIN);

float temperature =
dht.readTemperature();

float humidity =
dht.readHumidity();

if (isnan(temperature) ||
isnan(humidity)) {


Serial.println("DHT Error");
return;


}

// AQI Classification

if (airValue < 1000) {
airStatus = "GOOD";

} else if (airValue < 2000) {


airStatus = "MODERATE";


} else if (airValue < 3000) {


airStatus = "POOR";


} else {


airStatus = "HAZARDOUS";


}

// Alert

if (airStatus == "HAZARDOUS") {


digitalWrite(LED_PIN, HIGH);
tone(BUZZER_PIN, 1000);


} else {


digitalWrite(LED_PIN, LOW);
noTone(BUZZER_PIN);


}

// MQTT JSON Payload

StaticJsonDocument<256> doc;

doc["aqi"] = airValue;
doc["temperature"] = temperature;
doc["humidity"] = humidity;
doc["status"] = airStatus;

char buffer[256];

serializeJson(doc, buffer);

client.publish(
"airquality/data",
buffer
);

// Serial Monitor

Serial.println("==============");
Serial.print("AQI: ");
Serial.println(airValue);

Serial.print("Temp: ");
Serial.println(temperature);

Serial.print("Humidity: ");
Serial.println(humidity);

Serial.print("Status: ");
Serial.println(airStatus);

Serial.println("MQTT Sent");

// OLED

display.clearDisplay();

display.setTextSize(1);

display.setCursor(0, 0);
display.print("AQI:");
display.print(airValue);

display.setCursor(0, 15);
display.print("Temp:");
display.print(temperature);

display.setCursor(0, 30);
display.print("Hum:");
display.print(humidity);

display.setCursor(0, 45);
display.print("Status:");

display.setCursor(0, 55);
display.print(airStatus);

display.display();

delay(3000);
}
