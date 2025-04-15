#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <DHT.h>

// LoRa pin configuration
#define ss 5
#define rst 14
#define dio0 2

// LED pin
#define LED_Pin 27

// Motor control pins for L298N
#define IN1 32
#define IN2 33
#define ENA 25

// DHT sensor configuration
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Variables for timeout
unsigned long lastPacketTime = 0;
const unsigned long timeoutInterval = 35000;

// Motor speed
int motorSpeed = 0;

// ESP32 PWM channel for motor
const int pwmChannel = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;

// Variables for DHT11 sensor readings
float temperature = 0.0;
float humidity = 0.0;

void setup() {
  Serial.begin(9600);

  // Initialize LED
  pinMode(LED_Pin, OUTPUT);
  digitalWrite(LED_Pin, LOW);

  // Initialize motor
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, motorSpeed);

  // Initialize LoRa
  Serial.println("Initializing LoRa...");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initialized OK!");

  // Record the initial time
  lastPacketTime = millis();

  // Initialize DHT11 sensor
  dht.begin();
}

void loop() {
  // Check for incoming LoRa messages
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Read the incoming message
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }
    Serial.print("Received message: ");
    Serial.println(receivedMessage);

    // Handle the LED
    if (receivedMessage.indexOf("ON") != -1) {
      digitalWrite(LED_Pin, HIGH);
    } else if (receivedMessage.indexOf("OFF") != -1) {
      digitalWrite(LED_Pin, LOW);
    }

    // Handle the motor speed
    if (receivedMessage.startsWith("Pot:")) {
      String potValueStr = receivedMessage.substring(4);
      int potValue = potValueStr.toInt();
      motorSpeed = map(potValue, 0, 4023, 255, 0);
      analogWrite(ENA, motorSpeed);
      Serial.print("Motor Speed: ");
      Serial.println(motorSpeed);
    }
  }

  // Read temperature and humidity
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    // Send DHT11 data via LoRa
    LoRa.beginPacket();
    LoRa.print("Temp:");
    LoRa.print(temperature);
    LoRa.print("C, Hum:");
    LoRa.print(humidity);
    LoRa.println("%");
    LoRa.endPacket();
    Serial.println("DHT11 data sent via LoRa");
  }

  delay(500); // Short delay to avoid rapid loop execution
}
