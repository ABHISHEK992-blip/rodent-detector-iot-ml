#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin definitions
#define TRIG_PIN D5
#define ECHO_PIN D6
#define IR_PIN D7
#define GAS_PIN A0
#define DHTPIN D4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address is usually 0x27 or 0x3F

// WiFi + MQTT
const char *ssid = "Ashu";
const char *password = "12345678";
const char *mqtt_server = "broker.hivemq.com"; // Or your broker

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;

void setup_wifi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void reconnect()
{
  while (!client.connected())
  {
    if (client.connect("ESP8266ClientRodent"))
    {
      Serial.println("MQTT connected!");
    }
    else
    {
      delay(5000);
    }
  }
}

float getUltrasonic()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH); // FIXED: use unsigned long
  return duration * 0.034 / 2;
}

void setup()
{
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);
  dht.begin();

  // LCD init
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Rodent System");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;

    // Collect sensor data
    float distance = getUltrasonic();
    int ir = digitalRead(IR_PIN);
    int gas = analogRead(GAS_PIN);
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Build JSON
    StaticJsonDocument<256> doc;
    doc["ultrasonic"] = distance;
    doc["ir"] = ir;
    doc["gas"] = gas;
    doc["humidity"] = h;
    doc["temperature"] = t;

    char buffer[256];
    serializeJson(doc, buffer);

    // Publish to MQTT
    client.publish("rodent/data", buffer);
    Serial.println(buffer);

    // Show data on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gas:");
    lcd.print(gas);
    lcd.print(" IR:");
    lcd.print(ir);

    lcd.setCursor(0, 1);
    lcd.print("T:");
    lcd.print(t, 1);
    lcd.print("C H:");
    lcd.print(h, 0);
    lcd.print("%");
  }
}
