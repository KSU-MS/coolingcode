#include <Adafruit_GFX.h>
#include <Adafruit_MAX31855.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Metro.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

// Global things
String printname;
Metro timerFlush = Metro(500); // a timer to write to sd
Metro displayUp = Metro(500);  // a timer to update display
File logger;                   // a var to actually write to said sd
#define SCREEN_WIDTH 128       // OLED display width, in pixels
#define SCREEN_HEIGHT 64       // OLED display height, in pixels
#define OLED_RESET -1          // Reset pin # use -1 if unsure
#define SCREEN_ADDRESS 0x3c    // See datasheet for Address
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Thermo copples asdasdd2123
#define MAXDO 3
#define MAXCLK 5
int MAXCS1 = 6;
int MAXCS2 = 7;
int MAXCS3 = 8;
int MAXCS4 = 9;
int flow = 24;
Adafruit_MAX31855 RadIn(MAXCLK, MAXCS1, MAXDO);
Adafruit_MAX31855 RadOut(MAXCLK, MAXCS2, MAXDO);
Adafruit_MAX31855 EngIn(MAXCLK, MAXCS3, MAXDO);
Adafruit_MAX31855 EngOut(MAXCLK, MAXCS4, MAXDO);

void drawThing(String msg, String pos); // Draw something idk
void readSensors();                     // Read shit

// Run once on startup
void setup() {
  // Wait for Serial to start
  Serial.begin(9600);
  while (!Serial) {
  }

  // Wait for display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.display(); // You must call .display() after draw command to apply

  Serial.print("Initializing sensors...");
  if (!RadIn.begin()) {
    Serial.println("ERROR.");
    while (1)
      delay(10);
  }
  if (!RadOut.begin()) {
    Serial.println("ERROR.");
    while (1)
      delay(10);
  }
  if (!EngIn.begin()) {
    Serial.println("ERROR.");
    while (1)
      delay(10);
  }
  if (!EngOut.begin()) {
    Serial.println("ERROR.");
    while (1)
      delay(10);
  }

  // Wait for SD stuffs
  if (!SD.begin(BUILTIN_SDCARD)) { // Begin Arduino SD API
    Serial.println("SD card failed or not present");
  }
  delay(500);

  char filename[] = "THM_0000.CSV";
  for (uint8_t i = 0; i < 10000; i++) {
    filename[4] = i / 1000 + '0';
    filename[5] = i / 100 % 10 + '0';
    filename[6] = i / 10 % 10 + '0';
    filename[7] = i % 10 + '0';
    if (!SD.exists(filename)) { // Create and open file
      logger = SD.open(filename, (uint8_t)O_WRITE | (uint8_t)O_CREAT);
      break;
    }
    if (i == 9999) { // Print if it cant
      Serial.println("Ran out of names, clear SD");
    }
  }

  printname = filename;

  // Debug prints
  if (logger) {
    Serial.print("Successfully opened SD file: ");
    Serial.println(filename);
  } else {
    Serial.println("Failed to open SD file");
  }

  // Print guide at top of CSV
  logger.println("ID,value");
  logger.flush();

  // Do te ting
  Serial.println("Log start");
}

// Loops forever
void loop() {
  // Update display
  if (displayUp.check()) {
    display.clearDisplay();
    drawThing(RadIn.readInternal(), "1");
    drawThing(RadOut.readInternal(), "2");
    drawThing(EngIn.readInternal(), "3");
    drawThing(EngOut.readInternal(), "4");
    drawThing(analogRead(flow), "5");
  }

  // Flush if timer ticked
  if (timerFlush.check()) {
    logger.flush();
  }

  readSensors();
}

// It do in fact start reading shit
void readSensors() {
  logger.print("RadIn");
  logger.println(RadIn.readInternal());
  logger.print("RadOut");
  logger.println(RadOut.readInternal());
  logger.print("EngIn");
  logger.println(EngIn.readInternal());
  logger.print("EngOut");
  logger.println(EngOut.readInternal());
  logger.print("Flow");
  logger.println(analogRead(flow), 9);
}

// It take string in, and it puts shit onto display
void drawThing(String msg, String pos) {
  display.setTextSize(2);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);                 // Use full 256 char 'Code Page 437' font

  if (pos == "1") {
    display.setCursor(0, 0);
    display.print("RadIn: ");
    display.print(msg);
  } else if (pos == "2") {
    display.setCursor(16, 0);
    display.print("RadIn: ");
    display.print(msg);
  } else if (pos == "3") {
    display.setCursor(32, 0);
    display.print("RadOut: ");
    display.print(msg);
  } else if (pos == "4") {
    display.setCursor(48, 0);
    display.print("EngOut: ");
    display.print(msg);
  } else if (pos == "5") {
    display.setCursor(64, 0);
    display.print("Flow: ");
    display.print(msg);
  } else {
    return;
  }

  display.display();
}
