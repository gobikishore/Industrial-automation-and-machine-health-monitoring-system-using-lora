#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WebServer.h>

// WiFi Credentials
const char* ssid = "Phoenix";
const char* password = "12345678";

WebServer server(80);

// Button pins
#define Button_1_Pin  13 // D13 (for LED control)
#define Button_2_Pin  12 // D12 (for potentiometer reading)

// LED pin
#define LED_Pin       27 // Pin for the LED (update as needed)

// Potentiometer pin
#define POT_PIN       34 // Analog pin for potentiometer

// OLED display parameters
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // No reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// LoRa pin configuration
#define ss 5   // CS pin
#define rst 14 // Reset pin
#define dio0 2 // IRQ pin

bool previousButton1State = HIGH;
bool previousButton2State = HIGH;
unsigned long lastPotSendTime = 0;
String receivedData = "Waiting for data...";
String temperature = "--";
String humidity = "--";
String ledState = "OFF";
String potValue = "--";
String motorSpeed = "0";
String loraStatus = "Connected";

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(Button_1_Pin, INPUT_PULLUP);
  pinMode(Button_2_Pin, INPUT_PULLUP);
  pinMode(LED_Pin, OUTPUT);
  digitalWrite(LED_Pin, LOW);
  pinMode(POT_PIN, INPUT);

  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("LoRa Sender");
  display.display();

  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    loraStatus = "Disconnected";
    while (1);
  }

  Serial.println("LoRa Initializing OK!");
  display.setCursor(0, 10);
  display.print("LoRa Initializing OK!");
  display.display();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    String html = "<html><head><style>"
                   "body { font-family: Arial, sans-serif; text-align: center; background: linear-gradient(to right, #0062E6, #33AEFF); color: white; padding: 20px; }"
                   "h1 { font-size: 26px; margin-bottom: 20px; }"
                   "div { background: rgba(255, 255, 255, 0.2); padding: 15px; margin: 10px auto; border-radius: 10px; width: 300px; box-shadow: 2px 2px 10px rgba(0,0,0,0.3); }"
                   "h3 { margin: 0; font-size: 20px; }"
                   "</style></head><body>"
                   "<h1>LoRa Data Monitor</h1>"
                   "<div><h3>🌡 Temperature: " + temperature + " °C</h3></div>"
                   "<div><h3>💧 Humidity: " + humidity + " %</h3></div>"
                   "<div><h3>💡 LED State: " + ledState + "</h3></div>"
                   "<div><h3>🎛 Potentiometer: " + potValue + "</h3></div>"
                   "<div><h3>⚙ Motor Speed: " + motorSpeed + " %</h3></div>"
                   "<div><h3>📡 LoRa Status: " + loraStatus + "</h3></div>"
                   "</body></html>";
    server.send(200, "text/html", html);
  });
  server.begin();
}

void loop() {
  server.handleClient();

  bool button1State = digitalRead(Button_1_Pin) == LOW;
  bool button2State = digitalRead(Button_2_Pin) == LOW;

  if (button1State != previousButton1State) {
    previousButton1State = button1State;
    ledState = button1State ? "ON" : "OFF";
    digitalWrite(LED_Pin, button1State ? HIGH : LOW);
    String ledMessage = "LED: " + ledState;
    Serial.print("Sending message: ");
    Serial.println(ledMessage);
    LoRa.beginPacket();
    LoRa.print(ledMessage);
    LoRa.endPacket();
  }

  digitalWrite(LED_Pin, digitalRead(Button_1_Pin) == LOW ? HIGH : LOW);

  if (button2State) {
    unsigned long currentTime = millis();
    if (currentTime - lastPotSendTime >= 2000) {
      lastPotSendTime = currentTime;
      int potRawValue = analogRead(POT_PIN);
      potValue = String(potRawValue);
      motorSpeed = String(map(potRawValue, 0, 4095, 0, 100));
      String potMessage = "Pot: " + potValue + " Motor: " + motorSpeed + "%";
      Serial.print("Sending message: ");
      Serial.println(potMessage);
      LoRa.beginPacket();
      LoRa.print(potMessage);
      LoRa.endPacket();
    }
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }
    Serial.print("Received: ");
    Serial.println(receivedData);

    if (receivedData.startsWith("Temp:")) temperature = receivedData.substring(5);
    else if (receivedData.startsWith("Hum:")) humidity = receivedData.substring(4);
    else if (receivedData.startsWith("Motor:")) motorSpeed = receivedData.substring(6);
  }
  delay(100);
}
