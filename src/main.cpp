#include <Adafruit_GFX.h>
#include <Adafruit_MAX31855.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Metro.h>
#include <SD.h>
#include <SPI.h>
#include <TimeLib.h>
#include <Wire.h>

// Global things
uint64_t global_ms_offset;     // Time calc things
uint64_t last_sec_epoch;       // Time calc things 2
String printname;              // Its the string for to display filename
Metro timerFlush = Metro(500); // a timer to write to sd
Metro displayUp = Metro(10);  // a timer to update display
File logger;                   // a var to actually write to said sd
#define SCREEN_WIDTH 128       // OLED display width, in pixels
#define SCREEN_HEIGHT 64       // OLED display height, in pixels
#define OLED_RESET -1          // Reset pin # use -1 if unsure
#define SCREEN_ADDRESS 0x3c    // See datasheet for Address
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// EMA variables for smoothing the reads
double AvgRadIn = 0; // starting EMA value
double AvgRadOut = 0; // starting EMA value
double AvgEngIn = 0; // starting EMA value
double AvgEngOut = 0; // starting EMA value
double smoothing = 0.9; 

// Thermo copples asdasdd2123
#define MAXDO 12
#define MAXCLK 13
Adafruit_MAX31855 RadIn(MAXCLK, 7, MAXDO);
Adafruit_MAX31855 RadOut(MAXCLK, 6, MAXDO);
Adafruit_MAX31855 EngIn(MAXCLK, 5, MAXDO);
Adafruit_MAX31855 EngOut(MAXCLK, 2, MAXDO);
int flowMeter = 24;

void drawThing(String msg, String pos); // Draw something idk
void readSensors();                     // Read shit
void ReadAvgSensors();                  // Read avg shit
double ema(double newValue, double previousEma, double smoothingFactor);                         // EMA shit

// Run once on startup
void setup() {
  // Wait for Serial to start
  Serial.begin(9600);
  // while (!Serial)
  //   delay(1);

  // COMMENT OUT THIS LINE AND PUSH ONCE RTC HAS BEEN SET!!!!
  // Teensy3Clock.set(1660351622); // set time (epoch) at powerup
  if (timeStatus() != timeSet) {
    Serial.println("RTC not set up, call Teensy3Clock.set(epoch)");
  } else {
    Serial.println("System date/time set to: ");
    Serial.print(Teensy3Clock.get());
  }
  last_sec_epoch = Teensy3Clock.get();

  // Wait for display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.display(); // You must call .display() after draw command to apply

  Serial.print("Initializing sensors...");
  if (!RadIn.begin() || !RadOut.begin() || !EngIn.begin() || !EngOut.begin()) {
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
  logger.println("time,name,value,");
  logger.flush();

  // Do te ting
  Serial.println("Log start");
}

// Loops forever
void loop() {
  // Update display
  // if (displayUp.check()) {
  //   display.clearDisplay();
  //   drawThing(RadIn.readCelsius(), "1");
  //   Serial.println(RadIn.readCelsius());
  //   drawThing(RadOut.readCelsius(), "2");
  //   Serial.println(RadOut.readCelsius());
  //   drawThing(EngIn.readCelsius(), "3");
  //   Serial.println(EngIn.readCelsius());
  //   drawThing(EngOut.readCelsius(), "4");
  //   Serial.println(EngOut.readCelsius());
  //   // drawThing(analogRead(flow), "5");
  // }

  // Update display Averages
  if (displayUp.check()) {
    display.clearDisplay();
    drawThing(AvgRadIn, "1");
    Serial.println(AvgRadIn);
    drawThing(AvgRadOut, "2");
    Serial.println(AvgRadOut);
    drawThing(AvgEngIn, "3");
    Serial.println(AvgEngIn);
    drawThing(AvgEngOut, "4");
    Serial.println(AvgEngOut);
    // drawThing(analogRead(flow), "5");
  }

  // Flush if timer ticked
  if (timerFlush.check()) {
    logger.flush();
  }

  ReadAvgSensors();
  // readSensors();
}

double ema(double newValue, double previousEma, double smoothingFactor) {
  return smoothingFactor * newValue + (1 - smoothingFactor) * previousEma;
}

void ReadAvgSensors() {
  AvgEngIn = ema(EngIn.readCelsius(), AvgEngIn, smoothing); // new EMA incorporating latest value
  AvgEngOut = ema(EngOut.readCelsius(), AvgEngOut, smoothing); // update EMA again
  AvgRadIn = ema(RadIn.readCelsius(), AvgRadIn, smoothing); // update EMA again
  AvgRadOut = ema(RadOut.readCelsius(), AvgRadOut, smoothing); // update EMA again
}
 
// It do in fact start reading shit
void readSensors() {

  // Calculate Time
  uint64_t sec_epoch = Teensy3Clock.get();
  if (sec_epoch != last_sec_epoch) {
    global_ms_offset = millis() % 1000;
    last_sec_epoch = sec_epoch;
  }
  uint64_t current_time =
      sec_epoch * 1000 + (millis() - global_ms_offset) % 1000;

  logger.print(current_time);
  logger.print(",RadIn,");
  logger.println(RadIn.readFahrenheit());
  logger.print(current_time);
  logger.print(",RadOut,");
  logger.println(RadOut.readFahrenheit());
  logger.print(current_time);
  logger.print(",EngIn,");
  logger.println(EngIn.readFahrenheit());
  logger.print(current_time);
  logger.print(",EngOut,");
  logger.println(EngOut.readFahrenheit());
  logger.print(current_time);
  logger.print(",Flow,");
  logger.println(analogRead(flowMeter), 9);
}

// It take string in, and it puts shit onto display
void drawThing(String msg, String pos) {
  display.setTextSize(1);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);                 // Use full 256 char 'Code Page 437' font

  if (pos == "1") {
    display.setCursor(0, 0);
    display.print("RadIn: ");
    display.print(msg);
  } else if (pos == "2") {
    display.setCursor(0, 16);
    display.print("RadOut: ");
    display.print(msg);
  } else if (pos == "3") {
    display.setCursor(0, 32);
    display.print("EngIn: ");
    display.print(msg);
  } else if (pos == "4") {
    display.setCursor(0, 48);
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
