#define BLYNK_TEMPLATE_ID "TMPL3FkoFiTO_"
#define BLYNK_TEMPLATE_NAME "Lora"
#define BLYNK_AUTH_TOKEN "ogLzcWKcP2GdH61qE2z0bzMEQKPMJ6Vv"

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <DHT.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// WiFi credentials
char ssid[] = "Phoenix";
char pass[] = "12345678";

// LoRa pin configuration
#define ss 5
#define rst 14
#define dio0 2

// LED pin
#define LED_Pin 27

// Motor control pins
#define IN1 32
#define IN2 33
#define ENA 25

// DHT sensor configuration
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Variables
unsigned long lastPacketTime = 0;
const unsigned long timeoutInterval = 35000;

int motorSpeed = 0;
bool motorStatus = false;

float temperature = 0.0;
float humidity = 0.0;

bool ledStatus = false;
BlynkTimer timer;

// ---------------- BLYNK CONTROLS ----------------
BLYNK_WRITE(V0) {
  int value = param.asInt();
  ledStatus = value;
  digitalWrite(LED_Pin, ledStatus ? HIGH : LOW);
}

BLYNK_WRITE(V3) {
  int motorState = param.asInt();
  motorStatus = motorState;

  if (motorStatus) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, motorSpeed); // use last speed
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
  }

  Serial.println(motorStatus ? "Motor ON (Blynk)" : "Motor OFF (Blynk)");
}

BLYNK_WRITE(V4) {
  motorSpeed = param.asInt();
  if (motorStatus) {
    analogWrite(ENA, motorSpeed);
  }
  Serial.print("Motor Speed (from Blynk): ");
  Serial.println(motorSpeed);
}
// ------------------------------------------------

void setup() {
  Serial.begin(9600);

  pinMode(LED_Pin, OUTPUT);
  digitalWrite(LED_Pin, LOW);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  pinMode(ENA, OUTPUT);
  analogWrite(ENA, motorSpeed);

  Serial.println("Initializing LoRa...");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initialized OK!");

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();

  lastPacketTime = millis();
  timer.setInterval(3000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }
    Serial.print("Received message: ");
    Serial.println(receivedMessage);

    if (receivedMessage.indexOf("ON") != -1) {
      Blynk.virtualWrite(V0, 1);
      digitalWrite(LED_Pin, HIGH);
    } else if (receivedMessage.indexOf("OFF") != -1) {
      Blynk.virtualWrite(V0, 0);
      digitalWrite(LED_Pin, LOW);
    }

    if (receivedMessage.startsWith("Pot:")) {
      String potValueStr = receivedMessage.substring(4);
      int potValue = potValueStr.toInt();
      motorSpeed = map(potValue, 0, 4023, 255, 0);
      if (motorStatus) {
        analogWrite(ENA, motorSpeed);
      }
      Serial.print("Motor Speed (from LoRa): ");
      Serial.println(motorSpeed);
    }

    if (receivedMessage.indexOf("MotorON") != -1) {
      Blynk.virtualWrite(V3, 1);
      motorStatus = true;
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      analogWrite(ENA, motorSpeed);
    } else if (receivedMessage.indexOf("MotorOFF") != -1) {
      Blynk.virtualWrite(V3, 0);
      motorStatus = false;
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      analogWrite(ENA, 0);
    }
  }
}

void sendSensorData() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);

  LoRa.beginPacket();
  LoRa.print("Temp:");
  LoRa.print(temperature);
  LoRa.print("C, Hum:");
  LoRa.print(humidity);
  LoRa.println("%");
  LoRa.endPacket();

  Serial.println("DHT11 data sent via LoRa");
}
