#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

bool previousButton1State = HIGH; // Previous state of Button 1
bool previousButton2State = HIGH; // Previous state of Button 2
unsigned long lastPotSendTime = 0; // Timestamp for last potentiometer reading

void setup() {
  // Initialize Serial for debug output
  Serial.begin(9600);
  while (!Serial);

  // Initialize button pins
  pinMode(Button_1_Pin, INPUT_PULLUP);
  pinMode(Button_2_Pin, INPUT_PULLUP);

  // Initialize LED pin
  pinMode(LED_Pin, OUTPUT);
  digitalWrite(LED_Pin, LOW); // Ensure LED is off initially

  // Initialize potentiometer pin
  pinMode(POT_PIN, INPUT);

  // Initialize OLED display
  Wire.begin(); // No parameters needed for ESP32
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("LoRa Sender");
  display.display();

  // Initialize LoRa
  Serial.println("LoRa Sender");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {  // Set frequency to 433 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa Initializing OK!");
  display.setCursor(0, 10);
  display.print("LoRa Initializing OK!");
  display.display();
}

void loop() {
  // Read button states
  bool button1State = digitalRead(Button_1_Pin) == LOW; // LOW when pressed
  bool button2State = digitalRead(Button_2_Pin) == LOW; // LOW when pressed

  // Handle Button 1 for LED control
  //if (button1State != previousButton1State) 
  { // If button state changes
    previousButton1State = button1State; // Update the previous state

    String ledMessage;
    if (button1State) {
      digitalWrite(LED_Pin, HIGH); // Turn LED ON
      ledMessage = "LED: ON";
    } else {
      digitalWrite(LED_Pin, LOW); // Turn LED OFF
      ledMessage = "LED: OFF";
    }

    // Send message via LoRa
    Serial.print("Sending message: ");
    Serial.println(ledMessage);
    LoRa.beginPacket();
    LoRa.print(ledMessage);
    LoRa.endPacket();

    // Update OLED display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("LoRa Sender");
    display.setCursor(0, 10);
    display.print("Sent: ");
    display.setCursor(0, 20);
    display.println(ledMessage);
    display.display();

  }

  // Handle Button 2 for Potentiometer reading
  if (button2State) {
    unsigned long currentTime = millis(); // Current time in milliseconds
    if (currentTime - lastPotSendTime >= 2000) { // Check if 2 second has passed
      lastPotSendTime = currentTime; // Update the timestamp

      int potValue = analogRead(POT_PIN); // Read potentiometer value (0-4095)
      String potMessage = "Pot: " + String(potValue);

      // Send potentiometer value
      Serial.print("Sending message: ");
      Serial.println(potMessage);
      LoRa.beginPacket();
      LoRa.print(potMessage);
      LoRa.endPacket();

      // Update OLED display
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.println("LoRa Sender");
      display.setCursor(0, 10);
      display.print("Pot Reading: ");
      display.setCursor(0, 20);
      display.println(potMessage);
      display.display();
    }
  } else if (!button2State && previousButton2State) {
    // If Button 2 is released, send "Pot: OFF"
    previousButton2State = button2State;

    String potMessage = "Pot: OFF";

    // Send message via LoRa
    Serial.print("Sending message: ");
    Serial.println(potMessage);
    LoRa.beginPacket();
    LoRa.print(potMessage);
    LoRa.endPacket();

    // Update OLED display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("LoRa Sender");
    display.setCursor(0, 10);
    display.print("Pot Status: ");
    display.setCursor(0, 20);
    display.println(potMessage);
    display.display();
  }

  // Update previous button state for Button 2
  previousButton2State = button2State;

  delay(100); // Main loop delay
}
