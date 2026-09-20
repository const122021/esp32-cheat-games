#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_SSD1306.h>

// ============================
// Пины устройства
// ============================
constexpr uint8_t TFT_CS   = 5;
constexpr uint8_t TFT_RST  = 17;
constexpr uint8_t TFT_DC   = 16;
constexpr uint8_t TFT_MOSI = 11;
constexpr uint8_t TFT_SCK  = 12;
constexpr uint8_t TFT_LED  = 4;

constexpr uint8_t SD_CS   = 10;
constexpr uint8_t SD_MISO = 13;

constexpr uint8_t OLED_SDA = 8;
constexpr uint8_t OLED_SCL = 9;

constexpr uint8_t BUZZER = 18;

constexpr uint8_t BTN_UP    = 1;
constexpr uint8_t BTN_DOWN  = 2;
constexpr uint8_t BTN_LEFT  = 3;
constexpr uint8_t BTN_RIGHT = 6;
constexpr uint8_t BTN_A     = 7;
constexpr uint8_t BTN_B     = 15;
constexpr uint8_t BTN_C     = 46;
constexpr uint8_t BTN_D     = 45;
constexpr uint8_t BTN_VOL   = 42;

const uint8_t buttons[] = {
  BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT,
  BTN_A, BTN_B, BTN_C, BTN_D, BTN_VOL
};

// ============================
// Цвета старого дизайна RGB565
// ============================
constexpr uint16_t COL_BG       = 0x0000;
constexpr uint16_t COL_RED      = 0xF800;
constexpr uint16_t COL_WHITE    = 0xFFFF;
constexpr uint16_t COL_GRAY     = 0x8410;
constexpr uint16_t COL_LIGHT    = 0xC618;
constexpr uint16_t COL_ITEM_BG  = 0x2104;
constexpr uint16_t COL_DARK     = 0x4208;

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ВАЖНО: rotation 3 — как в исходном коде пользователя.
// При такой ориентации интерфейс использует область 160x128.
constexpr uint8_t DISPLAY_ROTATION = 3;

enum Screen {
  SCREEN_MAIN,
  SCREEN_GAMES,
  SCREEN_SETTINGS
};

Screen screen = SCREEN_MAIN;

const char* mainItems[] = {"GAMES", "VOLUME", "SETTING"};
const char* settingItems[] = {"SOUND", "VOLUME", "BACK"};
const char* gameNames[] = {
  "SNAKE", "TETRIS", "NONE", "NONE", "NONE",
  "NONE", "NONE", "NONE", "NONE", "NONE"
};

bool gameExists[10] = {
  true, true, false, false, false,
  false, false, false, false, false
};

int mainSelected = 0;
int gamesSelected = 0;
int settingsSelected = 0;

bool lastButtonState[9] = {false};
uint8_t volumeLevel = 3;
bool soundEnabled = true;

// ============================
// Ввод и звук
// ============================
bool pressed(uint8_t index) {
  bool state = digitalRead(buttons[index]) == LOW;
  bool event = state && !lastButtonState[index];
  lastButtonState[index] = state;
  return event;
}

void beep(uint16_t frequency, uint16_t duration = 60) {
  if (!soundEnabled || volumeLevel == 0) return;

  uint8_t duty = map(volumeLevel, 0, 5, 0, 255);
  ledcWrite(0, duty);
  ledcWriteTone(0, frequency);
  delay(duration);
  ledcWriteTone(0, 0);
  ledcWrite(0, 0);
}

void showOLED(const String& line1, const String& line2 = "") {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("ESP32 GAMEBOX");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setCursor(0, 20);
  oled.println(line1);
  oled.setCursor(0, 36);
  oled.println(line2);
  oled.display();
}

// ============================
// Главное меню — старый дизайн
// ============================
void drawMainMenu() {
  tft.fillScreen(COL_BG);
  tft.fillRect(0, 0, 160, 3, COL_RED);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_RED, COL_BG);
  tft.drawString("TERMINAL // M-03", 4, 6, 1);

  tft.setTextColor(COL_WHITE, COL_BG);
  tft.drawString("// SELECT", 4, 18, 1);
  tft.setTextColor(COL_RED, COL_BG);
  tft.drawString("PROTOCOL", 58, 18, 1);

  tft.setTextColor(COL_WHITE, COL_BG);
  tft.drawString("MAIN", 4, 30, 2);
  tft.drawString("MENU", 4, 46, 2);

  for (int i = 0; i < 3; i++) {
    int y = 68 + i * 16;

    if (i == mainSelected) {
      tft.fillRoundRect(4, y, 152, 14, 2, COL_WHITE);
      tft.drawRoundRect(4, y, 152, 14, 2, COL_RED);
      tft.fillRect(6, y + 2, 14, 10, COL_RED);

      tft.setTextColor(COL_WHITE, COL_RED);
      tft.drawNumber(i + 1, 10, y + 3, 1);

      tft.setTextColor(COL_BG, COL_WHITE);
      tft.drawString(mainItems[i], 26, y + 2, 2);

      tft.setTextColor(COL_RED, COL_WHITE);
      tft.drawString(">>", 138, y + 4, 1);
    } else {
      tft.fillRoundRect(4, y, 152, 14, 2, COL_ITEM_BG);

      tft.setTextColor(COL_GRAY, COL_ITEM_BG);
      tft.drawNumber(i + 1, 10, y + 3, 1);

      tft.setTextColor(COL_LIGHT, COL_ITEM_BG);
      tft.drawString(mainItems[i], 26, y + 2, 2);

      tft.setTextColor(COL_GRAY, COL_ITEM_BG);
      tft.drawString(">>", 138, y + 4, 1);
    }
  }

  tft.setTextColor(COL_GRAY, COL_BG);
  tft.drawString("[A] OK   [UP/DN] NAV", 4, 118, 1);
}

// ============================
// Меню игр — старый дизайн
// ============================
void drawGamesMenu() {
  tft.fillScreen(COL_BG);
  tft.fillRect(0, 0, 160, 3, COL_RED);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_RED, COL_BG);
  tft.drawString("TERMINAL // M-03", 4, 6, 1);

  tft.setTextColor(COL_WHITE, COL_BG);
  tft.drawString("// SELECT PROTOCOL", 4, 16, 1);
  tft.drawString("GAMES", 4, 28, 2);

  for (int i = 0; i < 10; i++) {
    int col = (i < 5) ? 0 : 1;
    int row = i % 5;
    int x = 4 + col * 78;
    int y = 48 + row * 13;

    if (i == gamesSelected) {
      tft.fillRoundRect(x, y, 74, 12, 2, COL_WHITE);
      tft.drawRoundRect(x, y, 74, 12, 2, COL_RED);
      tft.fillRect(x + 2, y + 2, 12, 8, COL_RED);

      tft.setTextColor(COL_WHITE, COL_RED);
      tft.drawNumber(i + 1, x + 4, y + 2, 1);

      tft.setTextColor(COL_BG, COL_WHITE);
      tft.drawString(gameNames[i], x + 17, y + 2, 1);
    } else {
      tft.fillRoundRect(x, y, 74, 12, 2, COL_ITEM_BG);

      tft.setTextColor(COL_GRAY, COL_ITEM_BG);
      tft.drawNumber(i + 1, x + 4, y + 2, 1);

      tft.setTextColor(gameExists[i] ? COL_LIGHT : COL_GRAY, COL_ITEM_BG);
      tft.drawString(gameNames[i], x + 17, y + 2, 1);
    }
  }

  tft.setTextColor(COL_GRAY, COL_BG);
  tft.drawString("[A] PLAY   [B] BACK", 4, 118, 1);
}

// ============================
// Меню настроек — старый дизайн
// ============================
void drawSettingsMenu() {
  tft.fillScreen(COL_BG);
  tft.fillRect(0, 0, 160, 3, COL_RED);

  tft.setTextColor(COL_RED, COL_BG);
  tft.drawString("TERMINAL // M-03", 4, 6, 1);

  tft.setTextColor(COL_WHITE, COL_BG);
  tft.drawString("// CONFIGURATION", 4, 18, 1);
  tft.drawString("SETTING", 4, 30, 2);

  for (int i = 0; i < 3; i++) {
    int y = 60 + i * 16;

    if (i == settingsSelected) {
      tft.fillRoundRect(4, y, 152, 14, 2, COL_WHITE);
      tft.drawRoundRect(4, y, 152, 14, 2, COL_RED);
      tft.setTextColor(COL_BG, COL_WHITE);
      tft.drawString(settingItems[i], 12, y + 3, 1);
    } else {
      tft.fillRoundRect(4, y, 152, 14, 2, COL_ITEM_BG);
      tft.setTextColor(COL_LIGHT, COL_ITEM_BG);
      tft.drawString(settingItems[i], 12, y + 3, 1);
    }

    if (i == 0) {
      tft.setTextColor(i == settingsSelected ? COL_RED : COL_GRAY,
                       i == settingsSelected ? COL_WHITE : COL_ITEM_BG);
      tft.drawString(soundEnabled ? "ON" : "OFF", 125, y + 3, 1);
    } else if (i == 1) {
      tft.setTextColor(i == settingsSelected ? COL_RED : COL_GRAY,
                       i == settingsSelected ? COL_WHITE : COL_ITEM_BG);
      tft.drawNumber(volumeLevel, 140, y + 3, 1);
    }
  }

  tft.setTextColor(COL_GRAY, COL_BG);
  tft.drawString("[A] SET   [B] BACK", 4, 118, 1);
}

void showGame(const char* name) {
  tft.fillScreen(COL_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_WHITE, COL_RED);
  tft.drawString(name, 80, 64, 4);
  showOLED("GAME RUNNING", name);
  beep(880, 100);
  delay(700);
}

// ============================
// Обработка меню
// ============================
void handleMain() {
  if (pressed(0)) {
    mainSelected = (mainSelected + 2) % 3;
    drawMainMenu();
    beep(420, 35);
  } else if (pressed(1)) {
    mainSelected = (mainSelected + 1) % 3;
    drawMainMenu();
    beep(520, 35);
  } else if (pressed(4)) {
    if (mainSelected == 0) {
      screen = SCREEN_GAMES;
      gamesSelected = 0;
      drawGamesMenu();
      beep(700, 80);
    } else if (mainSelected == 1) {
      volumeLevel = (volumeLevel + 1) % 6;
      drawMainMenu();
      beep(620, 80);
    } else {
      screen = SCREEN_SETTINGS;
      settingsSelected = 0;
      drawSettingsMenu();
      beep(620, 80);
    }
  }
}

void handleGames() {
  if (pressed(0)) {
    gamesSelected = (gamesSelected + 9) % 10;
    drawGamesMenu();
    beep(420, 35);
  } else if (pressed(1)) {
    gamesSelected = (gamesSelected + 1) % 10;
    drawGamesMenu();
    beep(520, 35);
  } else if (pressed(5)) {
    screen = SCREEN_MAIN;
    drawMainMenu();
    beep(260, 60);
  } else if (pressed(4)) {
    if (gameExists[gamesSelected]) {
      showGame(gameNames[gamesSelected]);
      drawGamesMenu();
    } else {
      showOLED("GAME UNAVAILABLE", "NOT INSTALLED");
      beep(180, 120);
      delay(500);
      drawGamesMenu();
    }
  }
}

void handleSettings() {
  if (pressed(0)) {
    settingsSelected = (settingsSelected + 2) % 3;
    drawSettingsMenu();
    beep(420, 35);
  } else if (pressed(1)) {
    settingsSelected = (settingsSelected + 1) % 3;
    drawSettingsMenu();
    beep(520, 35);
  } else if (pressed(5)) {
    screen = SCREEN_MAIN;
    drawMainMenu();
    beep(260, 60);
  } else if (pressed(4)) {
    if (settingsSelected == 0) {
      soundEnabled = !soundEnabled;
      drawSettingsMenu();
      if (soundEnabled) beep(660, 80);
    } else if (settingsSelected == 1) {
      volumeLevel = (volumeLevel + 1) % 6;
      drawSettingsMenu();
      beep(780, 80);
    } else {
      screen = SCREEN_MAIN;
      drawMainMenu();
      beep(320, 60);
    }
  } else if (pressed(7) && settingsSelected == 1) {
    if (volumeLevel > 0) volumeLevel--;
    drawSettingsMenu();
    beep(240, 60);
  }
}

// ============================
// Setup / loop
// ============================
void setup() {
  Serial.begin(115200);

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  for (uint8_t pin : buttons) {
    pinMode(pin, INPUT_PULLUP);
  }

  Wire.begin(OLED_SDA, OLED_SCL);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  showOLED("BOOTING", "ESP32 GAMEBOX");

  SPI.begin(TFT_SCK, SD_MISO, TFT_MOSI);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(DISPLAY_ROTATION);
  tft.setTextWrap(false);
  tft.fillScreen(COL_BG);

  ledcSetup(0, 2000, 8);
  ledcAttachPin(BUZZER, 0);
  ledcWrite(0, 0);

  if (SD.begin(SD_CS, SPI)) {
    Serial.println("SD ready");
    showOLED("SD READY", "GAME STORAGE");
  } else {
    Serial.println("SD init failed");
    showOLED("SD NOT FOUND", "MENU STILL WORKS");
  }

  delay(500);
  drawMainMenu();
  beep(740, 90);
}

void loop() {
  if (screen == SCREEN_MAIN) {
    handleMain();
  } else if (screen == SCREEN_GAMES) {
    handleGames();
  } else {
    handleSettings();
  }

  delay(25);
}
