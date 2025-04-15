#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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
unsigned long lastPacketTime = 0;         // Last time a LoRa message was received
const unsigned long timeoutInterval = 35000; // Timeout interval in milliseconds

// Motor speed
int motorSpeed = 0;

// ESP32 PWM channel for motor
const int pwmChannel = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;

// Variables for DHT11 sensor readings
float temperature = 0.0;
float humidity = 0.0; // New variable for humidity

void setup() {
  Serial.begin(9600);
  
  // Initialize OLED
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();

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
  Serial.print(packetSize);
  if (packetSize)
   {
    // Reset the timeout timer
   // lastPacketTime = millis();

    // Read the incoming message
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
         
    }
     Serial.print("Received message: ");
    Serial.println(receivedMessage);

    // Debug: Display the received message


    // Handle the LED
    if (receivedMessage.indexOf("ON") != -1) {
      digitalWrite(LED_Pin, HIGH);
    } else if (receivedMessage.indexOf("OFF") != -1) {
      digitalWrite(LED_Pin, LOW);
    }

    // Handle the motor speed
    if (receivedMessage.startsWith("Pot:")) {
      String potValueStr = receivedMessage.substring(4); // Extract the value
      int potValue = potValueStr.toInt();
      motorSpeed = map(potValue, 0, 4023, 255, 0);
      analogWrite(ENA, motorSpeed);
      Serial.print("Motor Speed: ");
      Serial.println(motorSpeed);
    }
  
  }
/*
  // Check for timeout
  if (millis() - lastPacketTime > 15000) {
    // Timeout occurred
    Serial.println("LoRa disconnected! Turning off LED and motor.");
    digitalWrite(LED_Pin, LOW);  // Turn off LED
    motorSpeed = 0;              // Turn off motor
    analogWrite(ENA, motorSpeed);
  }
*/
  // Read temperature and humidity
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    temperature = 0.0; // Set to 0 if reading fails
    humidity = 0.0;    // Set to 0 if reading fails
  }

  // Update OLED display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LoRa Receiver");

  // Display LoRa connection status
  display.setCursor(0, 10);
  display.print("Status: ");
  //display.println((millis() - lastPacketTime > timeoutInterval) ? "Disconnected" : "Connected");

  // Display LED status
  display.setCursor(0, 20);
  display.print("LED: ");
  display.println(digitalRead(LED_Pin) == HIGH ? "ON" : "OFF");

  // Display motor speed
  display.setCursor(0, 30);
  display.print("Motor Speed: ");
  display.println(motorSpeed);

  // Display temperature
  display.setCursor(0, 40);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  // Display humidity
  display.setCursor(0, 50);
  display.print("Hum: ");
  display.print(humidity);
  display.println(" %");

  display.display();
  delay(500); // Short delay to avoid rapid loop execution
}
