#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_SSD1306.h>

// --- Hardware config ---
constexpr int TFT_CS = 5;
constexpr int TFT_RST = 17;
constexpr int TFT_DC = 16;
constexpr int TFT_SDA = 11;
constexpr int TFT_SCK = 12;
constexpr int TFT_LED = 4;

constexpr int SD_CS = 10;
constexpr int SD_MISO = 13;

constexpr int OLED_SDA = 8;
constexpr int OLED_SCL = 9;
constexpr int OLED_W = 128;
constexpr int OLED_H = 64;

constexpr int BUTTON_UP = 1;
constexpr int BUTTON_DOWN = 2;
constexpr int BUTTON_LEFT = 3;
constexpr int BUTTON_RIGHT = 6;
constexpr int BUTTON_A = 7;
constexpr int BUTTON_B = 15;
constexpr int BUTTON_C = 46;
constexpr int BUTTON_D = 45;
constexpr int BUTTON_VOL = 42;

const int buttonPins[] = {
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_LEFT,
  BUTTON_RIGHT,
  BUTTON_A,
  BUTTON_B,
  BUTTON_C,
  BUTTON_D,
  BUTTON_VOL
};

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);

// --- DANDY / NES ROM menu state ---
String romFiles[32];
int romCount = 0;
int selectedRom = 0;
bool sdReady = false;
bool dandyFolderFound = false;

bool previousButtons[9] = {false};

void drawOLEDStatus(const String& line1, const String& line2 = "") {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("DANDY NES");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setCursor(0, 18);
  oled.println(line1);
  oled.setCursor(0, 34);
  oled.println(line2);
  oled.display();
}

bool pressed(uint8_t index) {
  bool current = digitalRead(buttonPins[index]) == LOW;
  bool result = current && !previousButtons[index];
  previousButtons[index] = current;
  return result;
}

void refreshDandyList() {
  romCount = 0;
  selectedRom = 0;
  dandyFolderFound = false;

  if (!sdReady || !SD.exists("/DANDY")) {
    drawOLEDStatus("SD not ready", "/DANDY missing");
    return;
  }

  dandyFolderFound = true;
  File dir = SD.open("/DANDY");
  if (!dir || !dir.isDirectory()) {
    drawOLEDStatus("Folder error", "/DANDY");
    return;
  }

  File entry = dir.openNextFile();
  while (entry && romCount < 32) {
    String name = entry.name();
    if (!entry.isDirectory() && name.endsWith(".nes")) {
      romFiles[romCount++] = name;
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();

  if (romCount == 0) {
    drawOLEDStatus("No ROMs found", "/DANDY/*.nes");
  } else {
    drawOLEDStatus("ROMs found", String(romCount) + " game(s)");
  }
}

void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.setCursor(18, 8);
  tft.println("DANDY");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.setCursor(10, 34);
  if (!sdReady) {
    tft.println("SD init failed");
    tft.setCursor(10, 50);
    tft.println("Check wiring");
    return;
  }

  if (!dandyFolderFound) {
    tft.println("Folder /DANDY");
    tft.setCursor(10, 50);
    tft.println("not found");
    return;
  }

  if (romCount == 0) {
    tft.println("No .nes files");
    tft.setCursor(10, 50);
    tft.println("add ROMs");
    return;
  }

  int startIndex = selectedRom;
  int shown = 0;
  for (int i = 0; i < 6 && startIndex + i < romCount; ++i) {
    int y = 28 + i * 18;
    if (startIndex + i == selectedRom) {
      tft.setTextColor(ST77XX_YELLOW);
      tft.fillRoundRect(6, y - 2, 116, 14, 2, ST77XX_BLUE);
      tft.setTextColor(ST77XX_WHITE);
    } else {
      tft.setTextColor(ST77XX_GREEN);
    }
    tft.setCursor(10, y);
    tft.println(romFiles[startIndex + i]);
    shown++;
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(8, 145);
  tft.print("Sel:");
  tft.print(String(selectedRom + 1));
  tft.print("/");
  tft.print(String(romCount));
}

void launchSelectedROM() {
  if (!sdReady || !dandyFolderFound || romCount == 0) {
    drawOLEDStatus("No valid ROM", "select a game");
    return;
  }

  String romName = romFiles[selectedRom];
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(12, 40);
  tft.println("LOADING");
  tft.setTextSize(1);
  tft.setCursor(10, 70);
  tft.println(romName);
  drawOLEDStatus("Launching", romName);

  // Full NES emulation is a large project and requires a dedicated core.
  // This launcher is the platform layer for SD scanning + menu + rotation.
  delay(1200);
  draw OLED status after loading with a message stays stable
}

void setup() {
  Serial.begin(115200);

  for (int pin : buttonPins) {
    pinMode(pin, INPUT_PULLUP);
  }

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  SPI.begin(TFT_SCK, SD_MISO, TFT_SDA);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);  // 180 degrees

  Wire.begin(OLED_SDA, OLED_SCL);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  drawOLEDStatus("Booting", "DANDY loader");

  sdReady = SD.begin(SD_CS, SPI);
  if (sdReady) {
    Serial.println("SD ready");
  } else {
    Serial.println("SD init failed");
  }

  refreshDandyList();
  drawMenu();
}

void loop() {
  if (pressed(0)) {
    if (romCount > 0) {
      if (selectedRom > 0) --selectedRom;
      drawMenu();
    }
  }

  if (pressed(1)) {
    if (romCount > 0) {
      if (selectedRom < romCount - 1) ++selectedRom;
      drawMenu();
    }
  }

  if (pressed(4)) {
    // A -> launch selected ROM
    launchSelectedROM();
  }

  if (pressed(5)) {
    // B -> rescan SD card
    refreshDandyList();
    drawMenu();
  }

  if (pressed(8)) {
    drawOLEDStatus("DANDY", String(romCount) + " ROM(s)");
    delay(700);
    drawMenu();
  }

  delay(25);
}
