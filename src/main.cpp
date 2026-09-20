#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_SSD1306.h>

// ============================
// Hardware configuration
// ============================
constexpr int TFT_CS = 5;
constexpr int TFT_RST = 17;
constexpr int TFT_DC = 16;
constexpr int TFT_MOSI = 11;
constexpr int TFT_SCK = 12;
constexpr int TFT_LED = 4;

constexpr int SD_CS = 10;
constexpr int SD_MISO = 13;

constexpr int OLED_SDA = 8;
constexpr int OLED_SCL = 9;
constexpr int OLED_W = 128;
constexpr int OLED_H = 64;

constexpr int BEEP_PIN = 18;

constexpr int BUTTON_UP = 1;
constexpr int BUTTON_DOWN = 2;
constexpr int BUTTON_LEFT = 3;
constexpr int BUTTON_RIGHT = 6;
constexpr int BUTTON_A = 7;
constexpr int BUTTON_B = 15;
constexpr int BUTTON_C = 46;
constexpr int BUTTON_D = 45;
constexpr int BUTTON_VOL = 42;

const uint8_t buttonPins[] = {
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

enum Screen {
  SCREEN_MAIN,
  SCREEN_GAMES,
  SCREEN_SETTINGS
};

Screen currentScreen = SCREEN_MAIN;

struct ConsoleSettings {
  uint8_t volume = 3;
  bool soundEnabled = true;
};

ConsoleSettings settings;
Preferences prefs;

bool previousButtons[9] = {false};

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);

const char* mainItems[] = {"GAMES", "VOLUME", "SETTINGS"};
const char* settingsItems[] = {"SOUND", "VOLUME", "BACK"};
const char* gameNames[10] = {
  "SNAKE", "TETRIS", "DUNGEON", "ARKANOID", "RACER",
  "SHOOTER", "PUZZLE", "RPG", "MUSIC", "TEST"
};

bool gameExists[10] = {
  true, true, false, false, false,
  false, false, false, false, false
};

int mainSelected = 0;
int gamesSelected = 0;
int settingsSelected = 0;
bool sdReady = false;

// ============================
// General helpers
// ============================
bool buttonPressed(uint8_t index) {
  bool current = digitalRead(buttonPins[index]) == LOW;
  bool result = current && !previousButtons[index];
  previousButtons[index] = current;
  return result;
}

void showOLEDStatus(const String& line1, const String& line2 = "") {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("ESP32 GAMEBOX");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setCursor(0, 18);
  oled.println(line1);
  oled.setCursor(0, 34);
  oled.println(line2);
  oled.display();
}

void playTone(uint16_t frequency, uint16_t duration = 80) {
  if (!settings.soundEnabled) {
    return;
  }

  ledcWriteTone(0, frequency);
  delay(duration);
  ledcWriteTone(0, 0);
}

void updateBuzzerLevel() {
  uint8_t level = settings.volume;
  uint8_t duty = map(level, 0, 5, 0, 255);
  ledcWrite(0, duty);
}

void loadSettings() {
  prefs.begin("esp32_gamebox", false);
  settings.volume = prefs.getUChar("volume", 3);
  settings.soundEnabled = prefs.getBool("sound", true);
  prefs.end();

  if (settings.volume > 5) {
    settings.volume = 5;
  }
}

void saveSettings() {
  prefs.begin("esp32_gamebox", false);
  prefs.putUChar("volume", settings.volume);
  prefs.putBool("sound", settings.soundEnabled);
  prefs.end();
}

void initBuzzer() {
  ledcSetup(0, 2000, 8);
  ledcAttachPin(BEEP_PIN, 0);
  ledcWrite(0, 0);
  updateBuzzerLevel();
}

void initButtons() {
  for (uint8_t pin : buttonPins) {
    pinMode(pin, INPUT_PULLUP);
  }
}

// ============================
// TFT drawing functions
// ============================
void drawMainMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, 0, 160, 3, ST77XX_RED);

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.setCursor(4, 6);
  tft.println("TERMINAL // M-03");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(4, 18);
  tft.println("// SELECT");
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(58, 18);
  tft.println("PROTOCOL");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(4, 30);
  tft.println("MAIN");
  tft.setCursor(4, 46);
  tft.println("MENU");

  for (int i = 0; i < 3; ++i) {
    int y = 68 + i * 16;

    if (i == mainSelected) {
      tft.fillRoundRect(4, y, 152, 14, 2, ST77XX_WHITE);
      tft.drawRoundRect(4, y, 152, 14, 2, ST77XX_RED);
      tft.fillRect(6, y + 2, 14, 10, ST77XX_RED);
      tft.setTextColor(ST77XX_WHITE, ST77XX_RED);
      tft.setCursor(13, y + 4);
      tft.setTextSize(1);
      tft.print(i + 1);

      tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
      tft.setCursor(26, y + 3);
      tft.setTextSize(2);
      tft.print(mainItems[i]);

      tft.setTextColor(ST77XX_RED, ST77XX_WHITE);
      tft.setCursor(142, y + 5);
      tft.setTextSize(1);
      tft.print(">>");
    } else {
      tft.fillRoundRect(4, y, 152, 14, 2, ST7735_BLUE);
      tft.setTextColor(ST77XX_GRAY, ST7735_BLUE);
      tft.setCursor(10, y + 3);
      tft.setTextSize(1);
      tft.print(i + 1);

      tft.setTextColor(ST77XX_LIGHTGREY, ST7735_BLUE);
      tft.setCursor(26, y + 3);
      tft.setTextSize(2);
      tft.print(mainItems[i]);

      tft.setTextColor(ST77XX_GRAY, ST7735_BLUE);
      tft.setCursor(142, y + 5);
      tft.print(">>");
    }
  }

  tft.setTextColor(ST77XX_GRAY);
  tft.setTextSize(1);
  tft.setCursor(4, 118);
  tft.println("[A] OK   [UP/DN] NAV");
}

void drawGamesMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, 0, 160, 3, ST77XX_RED);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(4, 6);
  tft.println("TERMINAL // M-03");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(4, 16);
  tft.println("// SELECT PROTOCOL");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(4, 28);
  tft.println("GAMES");

  for (int i = 0; i < 10; ++i) {
    int col = (i < 5) ? 0 : 1;
    int row = i % 5;
    int x = 4 + col * 78;
    int y = 48 + row * 13;

    if (i == gamesSelected) {
      tft.fillRoundRect(x, y, 74, 12, 2, ST77XX_WHITE);
      tft.drawRoundRect(x, y, 74, 12, 2, ST77XX_RED);
      tft.fillRect(x + 2, y + 2, 12, 8, ST77XX_RED);
      tft.setTextColor(ST77XX_WHITE, ST77XX_RED);
      tft.setTextSize(1);
      tft.setCursor(x + 6, y + 3);
      tft.print(i + 1);
      tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
      tft.setCursor(x + 17, y + 2);
      tft.print(gameNames[i]);
    } else {
      tft.fillRoundRect(x, y, 74, 12, 2, ST7735_BLUE);
      tft.setTextColor(ST77XX_GRAY, ST7735_BLUE);
      tft.setTextSize(1);
      tft.setCursor(x + 4, y + 3);
      tft.print(i + 1);
      tft.setTextColor(gameExists[i] ? ST77XX_LIGHTGREY : ST77XX_DARKGREY, ST7735_BLUE);
      tft.setCursor(x + 17, y + 2);
      tft.print(gameNames[i]);
    }
  }

  tft.setTextColor(ST77XX_GRAY);
  tft.setTextSize(1);
  tft.setCursor(4, 118);
  tft.println("[A] PLAY   [B] BACK");
}

void drawSettingsMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRect(0, 0, 160, 3, ST77XX_RED);

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.setCursor(4, 6);
  tft.println("TERMINAL // M-03");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(4, 28);
  tft.println("SETTINGS");

  int yBase = 60;
  for (int i = 0; i < 3; ++i) {
    int y = yBase + i * 18;

    if (i == settingsSelected) {
      tft.fillRoundRect(6, y, 148, 14, 2, ST77XX_WHITE);
      tft.drawRoundRect(6, y, 148, 14, 2, ST77XX_RED);
      tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
    } else {
      tft.fillRoundRect(6, y, 148, 14, 2, ST7735_BLUE);
      tft.setTextColor(ST77XX_LIGHTGREY, ST7735_BLUE);
    }

    tft.setTextSize(1);
    tft.setCursor(12, y + 4);
    tft.print(settingsItems[i]);

    if (i == 0) {
      tft.setCursor(96, y + 4);
      tft.print(settings.soundEnabled ? "ON" : "OFF");
    } else if (i == 1) {
      tft.setCursor(96, y + 4);
      tft.print("VOL:");
      tft.print(settings.volume);
    }
  }

  tft.setTextColor(ST77XX_GRAY);
  tft.setTextSize(1);
  tft.setCursor(4, 118);
  tft.println("[A] SET   [B] BACK");
}

void drawGameLaunchScreen(const char* title) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(12, 40);
  tft.println("LOADING");

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(14, 70);
  tft.println(title);

  showOLEDStatus("Launching", String(title));
  playTone(440, 90);
  delay(400);
  playTone(660, 110);
}

// ============================
// Menu logic
// ============================
void handleMainMenu() {
  if (buttonPressed(0)) {
    mainSelected = (mainSelected + 2) % 3;
    drawMainMenu();
    playTone(420, 35);
  }

  if (buttonPressed(1)) {
    mainSelected = (mainSelected + 1) % 3;
    drawMainMenu();
    playTone(520, 35);
  }

  if (buttonPressed(4)) {
    if (mainSelected == 0) {
      currentScreen = SCREEN_GAMES;
      gamesSelected = 0;
      drawGamesMenu();
      playTone(700, 80);
    } else if (mainSelected == 1) {
      settings.volume = (settings.volume + 1) % 6;
      saveSettings();
      updateBuzzerLevel();
      drawMainMenu();
      playTone(620, 80);
    } else if (mainSelected == 2) {
      currentScreen = SCREEN_SETTINGS;
      settingsSelected = 0;
      drawSettingsMenu();
      playTone(620, 80);
    }
  }
}

void handleGamesMenu() {
  if (buttonPressed(0)) {
    gamesSelected = (gamesSelected + 9) % 10;
    drawGamesMenu();
    playTone(450, 30);
  }

  if (buttonPressed(1)) {
    gamesSelected = (gamesSelected + 1) % 10;
    drawGamesMenu();
    playTone(520, 30);
  }

  if (buttonPressed(2)) {
    gamesSelected = (gamesSelected + 4) % 10;
    drawGamesMenu();
    playTone(500, 30);
  }

  if (buttonPressed(3)) {
    gamesSelected = (gamesSelected + 6) % 10;
    drawGamesMenu();
    playTone(560, 30);
  }

  if (buttonPressed(5)) {
    currentScreen = SCREEN_MAIN;
    drawMainMenu();
    playTone(260, 60);
  }

  if (buttonPressed(4)) {
    if (gameExists[gamesSelected]) {
      drawGameLaunchScreen(gameNames[gamesSelected]);
      delay(700);
      drawGamesMenu();
      playTone(780, 100);
    } else {
      showOLEDStatus("Game locked", "Not available");
      playTone(180, 120);
      delay(500);
      drawGamesMenu();
    }
  }
}

void handleSettingsMenu() {
  if (buttonPressed(0)) {
    settingsSelected = (settingsSelected + 2) % 3;
    drawSettingsMenu();
    playTone(440, 30);
  }

  if (buttonPressed(1)) {
    settingsSelected = (settingsSelected + 1) % 3;
    drawSettingsMenu();
    playTone(500, 30);
  }

  if (buttonPressed(5)) {
    currentScreen = SCREEN_MAIN;
    saveSettings();
    drawMainMenu();
    playTone(260, 60);
  }

  if (buttonPressed(4)) {
    if (settingsSelected == 0) {
      settings.soundEnabled = !settings.soundEnabled;
      saveSettings();
      drawSettingsMenu();
      playTone(660, 80);
    } else if (settingsSelected == 1) {
      settings.volume = (settings.volume + 1) % 6;
      saveSettings();
      updateBuzzerLevel();
      drawSettingsMenu();
      playTone(780, 80);
    } else {
      currentScreen = SCREEN_MAIN;
      drawMainMenu();
      playTone(320, 60);
    }
  }

  if (buttonPressed(7)) {
    if (settingsSelected == 1) {
      settings.volume = (settings.volume == 0) ? 0 : settings.volume - 1;
      saveSettings();
      updateBuzzerLevel();
      drawSettingsMenu();
      playTone(240, 60);
    }
  }
}

// ============================
// Setup / loop
// ============================
void setup() {
  Serial.begin(115200);

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  initButtons();
  loadSettings();

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
  }

  SPI.begin(TFT_SCK, SD_MISO, TFT_MOSI);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);

  initBuzzer();

  showOLEDStatus("BOOTING", "ESP32 GAMEBOX");
  delay(500);

  sdReady = SD.begin(SD_CS, SPI);
  if (sdReady) {
    Serial.println("SD card ready");
    showOLEDStatus("SD READY", "Scan games");
  } else {
    Serial.println("SD card failed");
    showOLEDStatus("SD FAIL", "No storage");
  }

  delay(600);

  currentScreen = SCREEN_MAIN;
  drawMainMenu();
  playTone(740, 90);
}

void loop() {
  if (currentScreen == SCREEN_MAIN) {
    handleMainMenu();
  } else if (currentScreen == SCREEN_GAMES) {
    handleGamesMenu();
  } else if (currentScreen == SCREEN_SETTINGS) {
    handleSettingsMenu();
  }

  delay(25);
}

