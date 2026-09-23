#include <Wire.h>
#include <GyverOLED.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <math.h>

// ====================== ДИСПЛЕЙ ======================
GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;

// ====================== ПИНЫ ======================
const int BTN_PREV  = 4;
const int BTN_NEXT  = 5;
const int BTN_MENU  = 6;
const int BTN_SLEEP = 7;
const int BAT_PIN   = 1;
const int I2C_SDA   = 8;
const int I2C_SCL   = 9;

// ====================== РЕЖИМЫ ======================
enum AppMode {
  MODE_TOP_MENU,
  MODE_CH_MENU,
  MODE_TEXT_VIEW,
  MODE_GAME_MENU,
  MODE_GAME_SNAKE,
  MODE_GAME_TETRIS,
  MODE_GAME_DINO,
  MODE_GAME_DOODLE,
  MODE_GAME_PACMAN,
  MODE_CALC,
  MODE_SETTINGS
};

AppMode currentMode = MODE_TOP_MENU;
int topMenuSelection = 0;
int gameMenuSelection = 0;
int currentSelection = 0;
int settingsSelection = 0;
AppMode settingsFromMode = MODE_GAME_MENU;

// ====================== НАСТРОЙКИ ИГР ======================
int  snakeFieldSize = 1;
int  snakeAppleCount = 1;
int  tetrisDifficulty = 1;
bool dinoBirdsEnabled = true;
bool dinoDayNight = true;
bool dinoCrazyRotate = true;
int  doodleSpeed = 1;
int  pacmanSpeed = 1;

// ====================== ДИНАМИЧЕСКИЕ ШПОРЫ ======================
#define MAX_FILES 30
String fileList[MAX_FILES];
int fileCount = 0;

WebServer server(80);
bool isScreenOn = true;
bool showMenuMode = false;

String textContent = "";
int pagePointers[100];
int currentPage = 0;
int totalPages = 0;

// ====================== ЗМЕЙКА ======================
#define SNAKE_MAX_LEN 60
int snakeX[SNAKE_MAX_LEN];
int snakeY[SNAKE_MAX_LEN];
int snakeLen = 3;
int snakeDir = 1;
int appleX[3], appleY[3];
int activeApples = 1;
unsigned long snakeTimer = 0;
bool snakeGameOver = false;
int snakeGridW = 30;
int snakeGridH = 15;
int snakeCell = 4;

// ====================== ТЕТРИС ======================
#define TETRIS_ROWS 16
#define TETRIS_COLS 8
bool tetrisField[TETRIS_ROWS][TETRIS_COLS];
int pieceX, pieceY, pieceType, pieceRot;
unsigned long tetrisTimer = 0;
bool tetrisGameOver = false;
int tetrisScore = 0;

const uint16_t tetrisPieces[7][4] = {
  {0x6600, 0x6600, 0x6600, 0x6600},
  {0x4444, 0x0F00, 0x4444, 0x0F00},
  {0x4460, 0x0E80, 0xC440, 0x2E00},
  {0x4E00, 0x4640, 0x0E40, 0x4C40},
  {0x6C00, 0x8C40, 0x6C00, 0x8C40},
  {0xC600, 0x4C80, 0xC600, 0x4C80},
  {0x8E00, 0x6440, 0x0E20, 0x44C0}
};

// ====================== ДИНО ======================
int dinoY = 44;
int dinoVelocity = 0;
bool isJumping = false;
int dinoFrame = 0;
int cactusX = 128;
int cactusType = 0;
int birdX = 160;
int birdY = 20;
int birdFrame = 0;
int dinoScore = 0;
bool dinoGameOver = false;
unsigned long dinoTimer = 0;
unsigned long dinoDayNightTimer = 0;
unsigned long dinoRotateTimer = 0;
bool dinoIsNight = false;
bool dinoFlipState = false;

const uint8_t dinoSprite[2][32] = {
  {0x00,0xFC,0xFE,0xA7,0xBF,0xBF,0xCF,0xC0,0xC0,0xFE,0xFE,0x7C,0x30,0x20,0x30,0x00,
   0x00,0x03,0x07,0x07,0x07,0x07,0x03,0x01,0x01,0x03,0x03,0x03,0x02,0x02,0x00,0x00},
  {0x00,0xFC,0xFE,0xA7,0xBF,0xBF,0xCF,0xC0,0xC0,0xFE,0xFE,0x7C,0x30,0x10,0x30,0x00,
   0x00,0x03,0x07,0x07,0x07,0x07,0x03,0x01,0x01,0x03,0x03,0x03,0x01,0x01,0x01,0x00}
};

const uint8_t birdSprite[2][8] = {
  {0x18,0x3C,0x7E,0xFF,0x18,0x18,0x00,0x00},
  {0x00,0x18,0x3C,0x7E,0xFF,0x18,0x18,0x00}
};

// ====================== ДУДЛ ======================
#define DOODLE_PLAT_COUNT 5
#define DOODLE_PLAT_WIDTH 18
int doodleX, doodleY, doodleVY;
int doodlePlatX[DOODLE_PLAT_COUNT];
int doodlePlatY[DOODLE_PLAT_COUNT];
long doodleScore = 0;
bool doodleGameOver = false;
unsigned long doodleTimer = 0;
bool doodleStartCleared = false;
int doodleMoveSpeed = 2;

// ====================== ПАКМЕН ======================
#define PAC_W 16
#define PAC_H 8
uint8_t pacMaze[PAC_H][PAC_W];
int pacX, pacY;
int pacDir = 1;
int ghostX, ghostY;
int ghostDir = 3;
int pacScore = 0;
int pacDotsLeft = 0;
bool pacGameOver = false;
bool pacWin = false;
unsigned long pacTimer = 0;
int pacSpeedMs = 140;

// ====================== КАЛЬКУЛЯТОР ======================
long calcNum1 = 0, calcNum2 = 0, calcResult = 0;
bool hasResult = false;
char calcOp = ' ';
bool enteringSecond = false;
const char grid[4][4] = {
  {'1','2','3','+'},
  {'4','5','6','-'},
  {'7','8','9','*'},
  {'C','0','=','/'}
};
int cursorX = 0, cursorY = 0;

// ====================== ПРОТОТИПЫ ======================
void showBootAnimation();
void showTopMenu();
void showMainMenu();
void showGameMenu();
void showSettings();
void refreshScreen();
void scanFiles();

void initSnake();
void loopSnake();
void drawSnake();
void spawnApple(int idx = 0);
void applySnakeSettings();

void initTetris();
void loopTetris();
void drawTetris();
void spawnTetrisPiece();
bool checkCollision(int px, int py, int pr);
void lockPiece();

void initDino();
void loopDino();
void drawDino();

void initDoodle();
void loopDoodle();
void drawDoodle();
void applyDoodleSettings();

void initPacman();
void loopPacman();
void drawPacman();
void applyPacmanSettings();

void initCalc();
void loopCalc();
void drawCalc();

bool loadTextToRAM(const char* path);
void renderTextPage();
void showInfoScreen();

void handleRoot();
void handleList();
void handleFile();
void handleCreate();
void handleSave();
void handleRename();
void handleDelete();

// ====================== SETUP ======================
void setup() {
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_SLEEP, INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL);
  oled.init();
  Wire.setClock(800000L);

  oled.clear();
  oled.setScale(1);
  oled.update();

  showBootAnimation();

  if (!LittleFS.begin(true)) {
    oled.setCursor(0, 0); oled.print(F("Ошибка Памяти!"));
    oled.update();
    while (1) delay(1000);
  }

  scanFiles();

  WiFi.softAP("ESP32_Cheat", "1332", 1, true);

  server.on("/", handleRoot);
  server.on("/list", handleList);
  server.on("/file", handleFile);
  server.on("/create", handleCreate);
  server.on("/save", handleSave);
  server.on("/rename", handleRename);
  server.on("/delete", handleDelete);
  server.begin();

  randomSeed(analogRead(BAT_PIN));
  showTopMenu();
}

// ====================== СКАНИРОВАНИЕ ФАЙЛОВ ======================
void scanFiles() {
  fileCount = 0;
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file && fileCount < MAX_FILES) {
    String name = file.name();
    if (name.startsWith("/")) name = name.substring(1);
    if (name.endsWith(".txt")) {
      fileList[fileCount] = name;
      fileCount++;
    }
    file = root.openNextFile();
  }
}

// ====================== LOOP ======================
void loop() {
  server.handleClient();

  // Кнопка Паника
  if (digitalRead(BTN_SLEEP) == LOW) {
    static unsigned long lastClick = 0;
    unsigned long now = millis();
    delay(40);
    if (now - lastClick < 350) {
      currentMode = MODE_TOP_MENU;
      topMenuSelection = 0;
      isScreenOn = true;
      oled.setPower(true);
      oled.invertDisplay(false);
      oled.flipH(false);
      showTopMenu();
      while (digitalRead(BTN_SLEEP) == LOW) delay(10);
      lastClick = 0;
      return;
    }
    lastClick = now;
    isScreenOn = !isScreenOn;
    if (!isScreenOn) oled.setPower(false);
    else {
      oled.setPower(true);
      refreshScreen();
    }
    while (digitalRead(BTN_SLEEP) == LOW) delay(10);
    return;
  }

  if (!isScreenOn) {
    delay(10);
    return;
  }

  // Долгое нажатие MENU → настройки
  static unsigned long menuPressStart = 0;
  static bool menuWasPressed = false;
  if (digitalRead(BTN_MENU) == LOW) {
    if (!menuWasPressed) {
      menuWasPressed = true;
      menuPressStart = millis();
    } else if (millis() - menuPressStart > 800) {
      if (currentMode == MODE_GAME_SNAKE || currentMode == MODE_GAME_TETRIS ||
          currentMode == MODE_GAME_DINO || currentMode == MODE_GAME_DOODLE ||
          currentMode == MODE_GAME_PACMAN) {
        settingsFromMode = currentMode;
        currentMode = MODE_SETTINGS;
        settingsSelection = 0;
        showSettings();
        while (digitalRead(BTN_MENU) == LOW) delay(10);
        menuWasPressed = false;
        return;
      }
    }
  } else {
    menuWasPressed = false;
  }

  switch (currentMode) {

    case MODE_TOP_MENU:
      if (digitalRead(BTN_NEXT) == LOW) {
        delay(140);
        topMenuSelection = (topMenuSelection + 1) % 3;
        showTopMenu();
        while (digitalRead(BTN_NEXT) == LOW) delay(10);
      }
      if (digitalRead(BTN_PREV) == LOW) {
        delay(140);
        topMenuSelection = (topMenuSelection + 2) % 3;
        showTopMenu();
        while (digitalRead(BTN_PREV) == LOW) delay(10);
      }
      if (digitalRead(BTN_MENU) == LOW) {
        delay(220);
        if (topMenuSelection == 0) {
          scanFiles();
          currentMode = MODE_CH_MENU;
          currentSelection = 0;
          showMainMenu();
        } else if (topMenuSelection == 1) {
          currentMode = MODE_GAME_MENU;
          gameMenuSelection = 0;
          showGameMenu();
        } else {
          initCalc();
          currentMode = MODE_CALC;
        }
        while (digitalRead(BTN_MENU) == LOW) delay(10);
      }
      break;

    case MODE_CH_MENU:
      if (digitalRead(BTN_NEXT) == LOW) {
        delay(140);
        if (fileCount > 0) {
          currentSelection = (currentSelection + 1) % fileCount;
          showMainMenu();
        }
        while (digitalRead(BTN_NEXT) == LOW) delay(10);
      }
      if (digitalRead(BTN_PREV) == LOW) {
        delay(140);
        if (fileCount > 0) {
          currentSelection = (currentSelection - 1 + fileCount) % fileCount;
          showMainMenu();
        }
        while (digitalRead(BTN_PREV) == LOW) delay(10);
      }
      if (digitalRead(BTN_MENU) == LOW) {
        delay(220);
        if (fileCount == 0) {
          oled.clear();
          oled.setCursor(0, 2); oled.print(F("Нет файлов"));
          oled.setCursor(0, 4); oled.print(F("Создай через сайт"));
          oled.update();
          delay(1800);
          showMainMenu();
        } else {
          String path = "/" + fileList[currentSelection];
          if (loadTextToRAM(path.c_str())) {
            currentPage = 0;
            currentMode = MODE_TEXT_VIEW;
            renderTextPage();
          } else {
            oled.clear();
            oled.setCursor(0, 2); oled.print(F("Ошибка чтения"));
            oled.update();
            delay(1500);
            showMainMenu();
          }
        }
        while (digitalRead(BTN_MENU) == LOW) delay(10);
      }
      break;

    case MODE_TEXT_VIEW:
      if (digitalRead(BTN_MENU) == LOW) {
        delay(180);
        showMenuMode = !showMenuMode;
        if (showMenuMode) showInfoScreen();
        else renderTextPage();
        while (digitalRead(BTN_MENU) == LOW) delay(10);
      }
      if (showMenuMode) { delay(10); return; }

      if (digitalRead(BTN_NEXT) == LOW) {
        delay(180);
        if (currentPage < totalPages - 1) {
          currentPage++;
          renderTextPage();
        } else {
          currentMode = MODE_CH_MENU;
          showMainMenu();
        }
        while (digitalRead(BTN_NEXT) == LOW) delay(10);
      }
      if (digitalRead(BTN_PREV) == LOW) {
        delay(180);
        if (currentPage > 0) {
          currentPage--;
          renderTextPage();
        } else {
          currentMode = MODE_CH_MENU;
          showMainMenu();
        }
        while (digitalRead(BTN_PREV) == LOW) delay(10);
      }
      break;

    case MODE_GAME_MENU:
      if (digitalRead(BTN_NEXT) == LOW) {
        delay(140);
        gameMenuSelection = (gameMenuSelection + 1) % 5;
        showGameMenu();
        while (digitalRead(BTN_NEXT) == LOW) delay(10);
      }
      if (digitalRead(BTN_PREV) == LOW) {
        delay(140);
        gameMenuSelection = (gameMenuSelection + 4) % 5;
        showGameMenu();
        while (digitalRead(BTN_PREV) == LOW) delay(10);
      }
      if (digitalRead(BTN_MENU) == LOW) {
        delay(220);
        settingsFromMode = MODE_GAME_MENU;
        currentMode = MODE_SETTINGS;
        settingsSelection = 0;
        showSettings();
        while (digitalRead(BTN_MENU) == LOW) delay(10);
      }
      break;

    case MODE_SETTINGS:
      if (digitalRead(BTN_NEXT) == LOW) {
        delay(140);
        settingsSelection++;
        showSettings();
        while (digitalRead(BTN_NEXT) == LOW) delay(10);
      }
      if (digitalRead(BTN_PREV) == LOW) {
        delay(140);
        if (settingsSelection > 0) settingsSelection--;
        showSettings();
        while (digitalRead(BTN_PREV) == LOW) delay(10);
      }
      if (digitalRead(BTN_MENU) == LOW) {
        delay(180);

        if (settingsFromMode == MODE_GAME_SNAKE || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 0)) {
          if (settingsSelection == 0) snakeFieldSize = (snakeFieldSize + 1) % 3;
          else if (settingsSelection == 1) snakeAppleCount = snakeAppleCount % 3 + 1;
          else if (settingsSelection == 2) {
            applySnakeSettings();
            initSnake();
            currentMode = MODE_GAME_SNAKE;
          } else {
            currentMode = MODE_GAME_MENU;
            showGameMenu();
          }
        }
        else if (settingsFromMode == MODE_GAME_TETRIS || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 1)) {
          if (settingsSelection == 0) tetrisDifficulty = tetrisDifficulty % 3 + 1;
          else if (settingsSelection == 1) {
            initTetris();
            currentMode = MODE_GAME_TETRIS;
          } else {
            currentMode = MODE_GAME_MENU;
            showGameMenu();
          }
        }
        else if (settingsFromMode == MODE_GAME_DINO || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 2)) {
          if (settingsSelection == 0) dinoBirdsEnabled = !dinoBirdsEnabled;
          else if (settingsSelection == 1) dinoDayNight = !dinoDayNight;
          else if (settingsSelection == 2) dinoCrazyRotate = !dinoCrazyRotate;
          else if (settingsSelection == 3) {
            initDino();
            currentMode = MODE_GAME_DINO;
          } else {
            currentMode = MODE_GAME_MENU;
            showGameMenu();
          }
        }
        else if (settingsFromMode == MODE_GAME_DOODLE || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 3)) {
          if (settingsSelection == 0) {
            doodleSpeed = (doodleSpeed + 1) % 3;
            applyDoodleSettings();
          } else if (settingsSelection == 1) {
            applyDoodleSettings();
            initDoodle();
            currentMode = MODE_GAME_DOODLE;
          } else {
            currentMode = MODE_GAME_MENU;
            showGameMenu();
          }
        }
        else if (settingsFromMode == MODE_GAME_PACMAN || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 4)) {
          if (settingsSelection == 0) {
            pacmanSpeed = (pacmanSpeed + 1) % 3;
            applyPacmanSettings();
          } else if (settingsSelection == 1) {
            applyPacmanSettings();
            initPacman();
            currentMode = MODE_GAME_PACMAN;
          } else {
            currentMode = MODE_GAME_MENU;
            showGameMenu();
          }
        }

        showSettings();
        while (digitalRead(BTN_MENU) == LOW) delay(10);
      }
      break;

    case MODE_GAME_SNAKE:   loopSnake();   break;
    case MODE_GAME_TETRIS:  loopTetris();  break;
    case MODE_GAME_DINO:    loopDino();    break;
    case MODE_GAME_DOODLE:  loopDoodle();  break;
    case MODE_GAME_PACMAN:  loopPacman();  break;
    case MODE_CALC:         loopCalc();    break;
  }
  delay(4);
}

// ====================== ОБЩИЕ ФУНКЦИИ ======================
void refreshScreen() {
  switch (currentMode) {
    case MODE_TOP_MENU:   showTopMenu(); break;
    case MODE_CH_MENU:    showMainMenu(); break;
    case MODE_GAME_MENU:  showGameMenu(); break;
    case MODE_SETTINGS:   showSettings(); break;
    case MODE_GAME_SNAKE: drawSnake(); break;
    case MODE_GAME_TETRIS:drawTetris(); break;
    case MODE_GAME_DINO:  drawDino(); break;
    case MODE_GAME_DOODLE:drawDoodle(); break;
    case MODE_GAME_PACMAN:drawPacman(); break;
    case MODE_CALC:       drawCalc(); break;
    case MODE_TEXT_VIEW:  if (showMenuMode) showInfoScreen(); else renderTextPage(); break;
  }
}

void showBootAnimation() {
  for (int cycle = 0; cycle < 2; cycle++) {
    for (int r = 0; r <= 8; r += 2) {
      oled.clear();
      oled.setScale(2);
      oled.setCursor(16, 2);
      oled.print(F("CHEAT"));
      oled.setScale(1);
      oled.rect(18 - r/2, 16 - r/2, 110 + r/2, 40 + r/2, OLED_STROKE);
      oled.update();
      delay(40);
    }
    for (int r = 8; r >= 0; r -= 2) {
      oled.clear();
      oled.setScale(2);
      oled.setCursor(16, 2);
      oled.print(F("CHEAT"));
      oled.setScale(1);
      oled.rect(18 - r/2, 16 - r/2, 110 + r/2, 40 + r/2, OLED_STROKE);
      oled.update();
      delay(40);
    }
  }
  delay(150);
}

void showTopMenu() {
  oled.clear();
  oled.setCursor(0, 0); oled.print(F("=== ГЛАВНОЕ МЕНЮ ==="));
  oled.setCursor(0, 2); oled.print(topMenuSelection == 0 ? F("> ШПАРГАЛКИ") : F("  ШПАРГАЛКИ"));
  oled.setCursor(0, 4); oled.print(topMenuSelection == 1 ? F("> ИГРЫ") : F("  ИГРЫ"));
  oled.setCursor(0, 6); oled.print(topMenuSelection == 2 ? F("> КАЛЬКУЛЯТОР") : F("  КАЛЬКУЛЯТОР"));
  oled.update();
}

void showMainMenu() {
  oled.clear();
  oled.setCursor(0, 0); oled.print(F("  ШПАРГАЛКИ  "));
  if (fileCount == 0) {
    oled.setCursor(0, 3); oled.print(F("Нет файлов"));
    oled.setCursor(0, 5); oled.print(F("Создай через сайт"));
  } else {
    // Показываем до 6 файлов с прокруткой
    int start = 0;
    if (currentSelection > 5) start = currentSelection - 5;
    for (int i = 0; i < 6 && (start + i) < fileCount; i++) {
      oled.setCursor(0, i + 2);
      if (start + i == currentSelection) oled.print(F("> "));
      else oled.print(F("  "));
      String name = fileList[start + i];
      if (name.length() > 18) name = name.substring(0, 18);
      oled.print(name);
    }
  }
  oled.update();
}

void showGameMenu() {
  oled.clear();
  oled.setCursor(0, 0); oled.print(F("=== ВЫБОР ИГРЫ ==="));
  oled.setCursor(0, 2); oled.print(gameMenuSelection == 0 ? F("> ЗМЕЙКА") : F("  ЗМЕЙКА"));
  oled.setCursor(0, 3); oled.print(gameMenuSelection == 1 ? F("> ТЕТРИС") : F("  ТЕТРИС"));
  oled.setCursor(0, 4); oled.print(gameMenuSelection == 2 ? F("> ДИНОЗАВРИК") : F("  ДИНОЗАВРИК"));
  oled.setCursor(0, 5); oled.print(gameMenuSelection == 3 ? F("> ДУДЛ ДЖАМП") : F("  ДУДЛ ДЖАМП"));
  oled.setCursor(0, 6); oled.print(gameMenuSelection == 4 ? F("> ПАКМЕН") : F("  ПАКМЕН"));
  oled.update();
}

void showSettings() {
  oled.clear();
  oled.setCursor(0, 0); oled.print(F("=== НАСТРОЙКИ ==="));

  if (settingsFromMode == MODE_GAME_SNAKE || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 0)) {
    oled.setCursor(0, 2);
    oled.print(settingsSelection == 0 ? F("> Поле: ") : F("  Поле: "));
    if (snakeFieldSize == 0) oled.print(F("15x15"));
    else if (snakeFieldSize == 1) oled.print(F("30x30"));
    else oled.print(F("60x60"));

    oled.setCursor(0, 3);
    oled.print(settingsSelection == 1 ? F("> Яблок: ") : F("  Яблок: "));
    oled.print(snakeAppleCount);

    oled.setCursor(0, 5);
    oled.print(settingsSelection == 2 ? F("> СТАРТ") : F("  СТАРТ"));
    oled.setCursor(0, 6);
    oled.print(settingsSelection == 3 ? F("> НАЗАД") : F("  НАЗАД"));
  }
  else if (settingsFromMode == MODE_GAME_TETRIS || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 1)) {
    oled.setCursor(0, 2);
    oled.print(settingsSelection == 0 ? F("> Сложность: ") : F("  Сложность: "));
    oled.print(tetrisDifficulty);
    oled.setCursor(0, 4);
    oled.print(settingsSelection == 1 ? F("> СТАРТ") : F("  СТАРТ"));
    oled.setCursor(0, 5);
    oled.print(settingsSelection == 2 ? F("> НАЗАД") : F("  НАЗАД"));
  }
  else if (settingsFromMode == MODE_GAME_DINO || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 2)) {
    oled.setCursor(0, 2);
    oled.print(settingsSelection == 0 ? F("> Птицы: ") : F("  Птицы: "));
    oled.print(dinoBirdsEnabled ? F("ВКЛ") : F("ВЫКЛ"));
    oled.setCursor(0, 3);
    oled.print(settingsSelection == 1 ? F("> День/Ночь: ") : F("  День/Ночь: "));
    oled.print(dinoDayNight ? F("ВКЛ") : F("ВЫКЛ"));
    oled.setCursor(0, 4);
    oled.print(settingsSelection == 2 ? F("> Поворот: ") : F("  Поворот: "));
    oled.print(dinoCrazyRotate ? F("ВКЛ") : F("ВЫКЛ"));
    oled.setCursor(0, 6);
    oled.print(settingsSelection == 3 ? F("> СТАРТ") : F("  СТАРТ"));
    oled.setCursor(0, 7);
    oled.print(settingsSelection == 4 ? F("> НАЗАД") : F("  НАЗАД"));
  }
  else if (settingsFromMode == MODE_GAME_DOODLE || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 3)) {
    oled.setCursor(0, 2);
    oled.print(settingsSelection == 0 ? F("> Скорость: ") : F("  Скорость: "));
    if (doodleSpeed == 0) oled.print(F("Медленно"));
    else if (doodleSpeed == 1) oled.print(F("Нормально"));
    else oled.print(F("Быстро"));
    oled.setCursor(0, 4);
    oled.print(settingsSelection == 1 ? F("> СТАРТ") : F("  СТАРТ"));
    oled.setCursor(0, 5);
    oled.print(settingsSelection == 2 ? F("> НАЗАД") : F("  НАЗАД"));
  }
  else if (settingsFromMode == MODE_GAME_PACMAN || (settingsFromMode == MODE_GAME_MENU && gameMenuSelection == 4)) {
    oled.setCursor(0, 2);
    oled.print(settingsSelection == 0 ? F("> Скорость: ") : F("  Скорость: "));
    if (pacmanSpeed == 0) oled.print(F("Медленно"));
    else if (pacmanSpeed == 1) oled.print(F("Нормально"));
    else oled.print(F("Быстро"));
    oled.setCursor(0, 4);
    oled.print(settingsSelection == 1 ? F("> СТАРТ") : F("  СТАРТ"));
    oled.setCursor(0, 5);
    oled.print(settingsSelection == 2 ? F("> НАЗАД") : F("  НАЗАД"));
  }
  oled.update();
}

// ====================== ЗМЕЙКА ======================
void applySnakeSettings() {
  if (snakeFieldSize == 0) { snakeGridW = 15; snakeGridH = 15; snakeCell = 8; }
  else if (snakeFieldSize == 1) { snakeGridW = 30; snakeGridH = 15; snakeCell = 4; }
  else { snakeGridW = 60; snakeGridH = 30; snakeCell = 2; }
  activeApples = snakeAppleCount;
}

void initSnake() {
  applySnakeSettings();
  snakeLen = 3;
  snakeDir = 1;
  snakeGameOver = false;
  for (int i = 0; i < snakeLen; i++) {
    snakeX[i] = snakeGridW / 2 - i;
    snakeY[i] = snakeGridH / 2;
  }
  for (int i = 0; i < activeApples; i++) spawnApple(i);
  snakeTimer = millis();
}

void spawnApple(int idx) {
  appleX[idx] = random(0, snakeGridW);
  appleY[idx] = random(0, snakeGridH);
}

void loopSnake() {
  if (snakeGameOver) {
    if (digitalRead(BTN_MENU) == LOW) {
      delay(220);
      currentMode = MODE_TOP_MENU;
      showTopMenu();
    }
    return;
  }
  if (digitalRead(BTN_PREV) == LOW) {
    snakeDir = (snakeDir + 1) % 4;
    while (digitalRead(BTN_PREV) == LOW);
    delay(30);
  }
  if (digitalRead(BTN_NEXT) == LOW) {
    snakeDir = (snakeDir + 3) % 4;
    while (digitalRead(BTN_NEXT) == LOW);
    delay(30);
  }
  int speed = (snakeFieldSize == 2) ? 85 : (snakeFieldSize == 1) ? 120 : 150;
  if (millis() - snakeTimer >= speed) {
    snakeTimer = millis();
    for (int i = snakeLen - 1; i > 0; i--) {
      snakeX[i] = snakeX[i - 1];
      snakeY[i] = snakeY[i - 1];
    }
    if (snakeDir == 0) snakeY[0]--;
    if (snakeDir == 1) snakeX[0]++;
    if (snakeDir == 2) snakeY[0]++;
    if (snakeDir == 3) snakeX[0]--;
    if (snakeX[0] < 0 || snakeX[0] >= snakeGridW || snakeY[0] < 0 || snakeY[0] >= snakeGridH)
      snakeGameOver = true;
    for (int i = 1; i < snakeLen; i++)
      if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) snakeGameOver = true;
    for (int a = 0; a < activeApples; a++) {
      if (snakeX[0] == appleX[a] && snakeY[0] == appleY[a]) {
        if (snakeLen < SNAKE_MAX_LEN) snakeLen++;
        spawnApple(a);
      }
    }
    drawSnake();
  }
}

void drawSnake() {
  oled.clear();
  if (snakeGameOver) {
    oled.setCursor(35, 2); oled.print(F("GAME OVER"));
    oled.setCursor(18, 5); oled.print(F("MENU - выход"));
  } else {
    oled.rect(0, 0, snakeGridW * snakeCell - 1, snakeGridH * snakeCell - 1, OLED_STROKE);
    for (int i = 0; i < snakeLen; i++)
      oled.rect(snakeX[i]*snakeCell, snakeY[i]*snakeCell,
                snakeX[i]*snakeCell + snakeCell-1, snakeY[i]*snakeCell + snakeCell-1, OLED_FILL);
    for (int a = 0; a < activeApples; a++)
      oled.circle(appleX[a]*snakeCell + snakeCell/2, appleY[a]*snakeCell + snakeCell/2, 1, OLED_FILL);
  }
  oled.update();
}

// ====================== ТЕТРИС ======================
void initTetris() {
  tetrisGameOver = false;
  tetrisScore = 0;
  for (int r = 0; r < TETRIS_ROWS; r++)
    for (int c = 0; c < TETRIS_COLS; c++)
      tetrisField[r][c] = false;
  spawnTetrisPiece();
  tetrisTimer = millis();
}

void spawnTetrisPiece() {
  int maxType = (tetrisDifficulty == 1) ? 4 : (tetrisDifficulty == 2) ? 6 : 7;
  pieceType = random(0, maxType);
  pieceRot = 0;
  pieceX = TETRIS_COLS / 2 - 1;
  pieceY = 0;
  if (checkCollision(pieceX, pieceY, pieceRot)) tetrisGameOver = true;
}

bool checkCollision(int px, int py, int pr) {
  uint16_t mask = tetrisPieces[pieceType][pr];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (mask & (1 << (15 - (i*4 + j)))) {
        int fx = px + j, fy = py + i;
        if (fx < 0 || fx >= TETRIS_COLS || fy >= TETRIS_ROWS) return true;
        if (fy >= 0 && tetrisField[fy][fx]) return true;
      }
  return false;
}

void lockPiece() {
  uint16_t mask = tetrisPieces[pieceType][pieceRot];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (mask & (1 << (15 - (i*4 + j)))) {
        int fx = pieceX + j, fy = pieceY + i;
        if (fy >= 0 && fy < TETRIS_ROWS && fx >= 0 && fx < TETRIS_COLS)
          tetrisField[fy][fx] = true;
      }
  for (int r = TETRIS_ROWS - 1; r >= 0; r--) {
    bool full = true;
    for (int c = 0; c < TETRIS_COLS; c++)
      if (!tetrisField[r][c]) { full = false; break; }
    if (full) {
      tetrisScore += 10 * tetrisDifficulty;
      for (int tr = r; tr > 0; tr--)
        for (int c = 0; c < TETRIS_COLS; c++)
          tetrisField[tr][c] = tetrisField[tr-1][c];
      for (int c = 0; c < TETRIS_COLS; c++) tetrisField[0][c] = false;
      r++;
    }
  }
}

void loopTetris() {
  if (tetrisGameOver) {
    if (digitalRead(BTN_MENU) == LOW) {
      delay(220);
      currentMode = MODE_TOP_MENU;
      showTopMenu();
    }
    return;
  }
  if (digitalRead(BTN_PREV) == LOW) {
    int next = (pieceRot + 1) % 4;
    if (!checkCollision(pieceX, pieceY, next)) pieceRot = next;
    drawTetris();
    while (digitalRead(BTN_PREV) == LOW);
    delay(30);
  }
  if (digitalRead(BTN_NEXT) == LOW) {
    if (!checkCollision(pieceX + 1, pieceY, pieceRot)) pieceX++;
    drawTetris();
    while (digitalRead(BTN_NEXT) == LOW);
    delay(30);
  }
  if (digitalRead(BTN_MENU) == LOW) {
    if (!checkCollision(pieceX - 1, pieceY, pieceRot)) pieceX--;
    drawTetris();
    while (digitalRead(BTN_MENU) == LOW);
    delay(30);
  }
  int fall = (tetrisDifficulty == 3) ? 260 : (tetrisDifficulty == 2) ? 360 : 440;
  if (millis() - tetrisTimer >= fall) {
    tetrisTimer = millis();
    if (!checkCollision(pieceX, pieceY + 1, pieceRot)) pieceY++;
    else {
      lockPiece();
      spawnTetrisPiece();
    }
    drawTetris();
  }
}

void drawTetris() {
  oled.clear();
  if (tetrisGameOver) {
    oled.setCursor(35, 2); oled.print(F("GAME OVER"));
    oled.setCursor(30, 4); oled.print(F("Очки: ")); oled.print(tetrisScore);
    oled.update();
    return;
  }
  int ox = 32;
  oled.rect(ox-1, 0, ox + TETRIS_COLS*4, TETRIS_ROWS*4, OLED_STROKE);
  for (int r = 0; r < TETRIS_ROWS; r++)
    for (int c = 0; c < TETRIS_COLS; c++)
      if (tetrisField[r][c])
        oled.rect(ox + c*4, r*4, ox + c*4 + 3, r*4 + 3, OLED_FILL);
  uint16_t mask = tetrisPieces[pieceType][pieceRot];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (mask & (1 << (15 - (i*4 + j)))) {
        int fx = pieceX + j, fy = pieceY + i;
        if (fy >= 0 && fy < TETRIS_ROWS && fx >= 0 && fx < TETRIS_COLS)
          oled.rect(ox + fx*4, fy*4, ox + fx*4 + 3, fy*4 + 3, OLED_FILL);
      }
  oled.setCursor(75, 2); oled.print(F("Очки:"));
  oled.setCursor(75, 4); oled.print(tetrisScore);
  oled.setCursor(75, 6); oled.print(F("Lv:")); oled.print(tetrisDifficulty);
  oled.update();
}

// ====================== ДИНО ======================
void initDino() {
  dinoY = 44; dinoVelocity = 0; isJumping = false;
  cactusX = 128; cactusType = random(0, 2);
  birdX = 180; birdY = random(15, 35);
  dinoScore = 0; dinoGameOver = false;
  dinoTimer = millis();
  dinoDayNightTimer = millis();
  dinoRotateTimer = millis();
  dinoIsNight = false; dinoFlipState = false;
  oled.invertDisplay(false);
  oled.flipH(false);
}

void loopDino() {
  if (dinoGameOver) {
    if (digitalRead(BTN_MENU) == LOW) {
      delay(220);
      oled.invertDisplay(false);
      oled.flipH(false);
      currentMode = MODE_TOP_MENU;
      showTopMenu();
    }
    return;
  }
  if ((digitalRead(BTN_PREV) == LOW || digitalRead(BTN_NEXT) == LOW || digitalRead(BTN_MENU) == LOW) && !isJumping) {
    dinoVelocity = -8;
    isJumping = true;
  }
  if (millis() - dinoTimer >= 24) {
    dinoTimer = millis();
    if (isJumping) {
      dinoY += dinoVelocity;
      dinoVelocity += 1;
      if (dinoY >= 44) { dinoY = 44; dinoVelocity = 0; isJumping = false; }
    } else if (random(0, 7) > 3) dinoFrame = !dinoFrame;

    int spd = 4 + dinoScore / 8;
    if (spd > 12) spd = 12;
    cactusX -= spd;
    if (cactusX < -12) {
      cactusX = 128 + random(0, 40);
      cactusType = random(0, 2);
      dinoScore++;
    }
    if (dinoBirdsEnabled) {
      birdX -= spd + 1;
      birdFrame = (millis() / 110) % 2;
      if (birdX < -10) {
        birdX = 140 + random(20, 70);
        birdY = random(12, 36);
      }
    }
    if (cactusX > 10 && cactusX < 28 && dinoY > 32) dinoGameOver = true;
    if (dinoBirdsEnabled && birdX > 12 && birdX < 30 && abs(dinoY - birdY) < 12) dinoGameOver = true;

    if (dinoDayNight && millis() - dinoDayNightTimer > 11000) {
      dinoDayNightTimer = millis();
      dinoIsNight = !dinoIsNight;
      oled.invertDisplay(dinoIsNight);
    }
    if (dinoCrazyRotate && millis() - dinoRotateTimer > 10000) {
      dinoRotateTimer = millis();
      dinoFlipState = !dinoFlipState;
      oled.flipH(dinoFlipState);
    }
    drawDino();
  }
}

void drawDino() {
  oled.clear();
  if (dinoGameOver) {
    oled.setCursor(35, 2); oled.print(F("GAME OVER"));
    oled.setCursor(25, 4); oled.print(F("Очки: ")); oled.print(dinoScore);
    oled.setCursor(15, 6); oled.print(F("MENU - выход"));
  } else {
    oled.line(0, 60, 127, 60, OLED_FILL);
    oled.drawBitmap(15, dinoY, (const uint8_t*)dinoSprite[dinoFrame], 16, 16);
    if (cactusType == 0) {
      oled.rect(cactusX+3, 44, cactusX+6, 59, OLED_FILL);
      oled.rect(cactusX, 48, cactusX+2, 52, OLED_FILL);
      oled.rect(cactusX+7, 46, cactusX+9, 50, OLED_FILL);
    } else {
      oled.rect(cactusX, 44, cactusX+3, 59, OLED_FILL);
      oled.rect(cactusX+6, 40, cactusX+9, 59, OLED_FILL);
    }
    if (dinoBirdsEnabled)
      oled.drawBitmap(birdX, birdY, (const uint8_t*)birdSprite[birdFrame], 8, 8);
    oled.setCursor(90, 0); oled.print(F("Sc:")); oled.print(dinoScore);
  }
  oled.update();
}

// ====================== ДУДЛ ======================
void applyDoodleSettings() {
  if (doodleSpeed == 0) doodleMoveSpeed = 1;
  else if (doodleSpeed == 1) doodleMoveSpeed = 2;
  else doodleMoveSpeed = 3;
}

void initDoodle() {
  applyDoodleSettings();
  doodleX = 60; doodleY = 50; doodleVY = -9;
  doodleScore = 0; doodleGameOver = false;
  doodleTimer = millis();
  doodleStartCleared = false;
  doodlePlatX[0] = doodleX - (DOODLE_PLAT_WIDTH/2 - 3);
  doodlePlatY[0] = 58;
  for (int i = 1; i < DOODLE_PLAT_COUNT; i++) {
    doodlePlatY[i] = 58 - i * 15;
    doodlePlatX[i] = random(0, 128 - DOODLE_PLAT_WIDTH);
  }
}

void loopDoodle() {
  if (doodleGameOver) {
    if (digitalRead(BTN_MENU) == LOW) {
      delay(220);
      currentMode = MODE_TOP_MENU;
      showTopMenu();
    }
    return;
  }
  if (digitalRead(BTN_PREV) == LOW) {
    doodleX -= doodleMoveSpeed;
    if (doodleX < -8) doodleX = 127;
  }
  if (digitalRead(BTN_NEXT) == LOW) {
    doodleX += doodleMoveSpeed;
    if (doodleX > 127) doodleX = -8;
  }
  if (millis() - doodleTimer >= 20) {
    doodleTimer = millis();
    doodleVY += 1;
    doodleY += doodleVY;
    if (doodleVY > 0) {
      for (int i = 0; i < DOODLE_PLAT_COUNT; i++) {
        if (doodleX + 6 >= doodlePlatX[i] && doodleX <= doodlePlatX[i] + DOODLE_PLAT_WIDTH &&
            doodleY + 7 >= doodlePlatY[i] && doodleY + 7 <= doodlePlatY[i] + 4) {
          doodleVY = -9;
          if (i != 0 && !doodleStartCleared) {
            doodleStartCleared = true;
            doodlePlatY[0] = 0;
            doodlePlatX[0] = random(0, 128 - DOODLE_PLAT_WIDTH);
          }
        }
      }
    }
    if (doodleY < 20) {
      int diff = 20 - doodleY;
      doodleY = 20;
      doodleScore += diff;
      for (int i = 0; i < DOODLE_PLAT_COUNT; i++) {
        doodlePlatY[i] += diff;
        if (doodlePlatY[i] > 63) {
          if (i == 0 && !doodleStartCleared) continue;
          doodlePlatY[i] = 0;
          doodlePlatX[i] = random(0, 128 - DOODLE_PLAT_WIDTH);
        }
      }
    }
    if (doodleY > 63) doodleGameOver = true;
    drawDoodle();
  }
}

void drawDoodle() {
  oled.clear();
  if (doodleGameOver) {
    oled.setCursor(35, 2); oled.print(F("GAME OVER"));
    oled.setCursor(25, 4); oled.print(F("Очки: ")); oled.print(doodleScore);
    oled.setCursor(15, 6); oled.print(F("MENU - выход"));
  } else {
    for (int i = 0; i < DOODLE_PLAT_COUNT; i++)
      if (doodlePlatY[i] >= 0 && doodlePlatY[i] <= 63)
        oled.rect(doodlePlatX[i], doodlePlatY[i], doodlePlatX[i] + DOODLE_PLAT_WIDTH, doodlePlatY[i] + 2, OLED_FILL);
    oled.rect(doodleX, doodleY, doodleX + 6, doodleY + 7, OLED_FILL);
    oled.setCursor(85, 0); oled.print(F("Sc:")); oled.print(doodleScore);
  }
  oled.update();
}

// ====================== ПАКМЕН ======================
void applyPacmanSettings() {
  if (pacmanSpeed == 0) pacSpeedMs = 180;
  else if (pacmanSpeed == 1) pacSpeedMs = 130;
  else pacSpeedMs = 90;
}

void initPacman() {
  applyPacmanSettings();
  const uint8_t mazeTemplate[PAC_H][PAC_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,2,2,2,2,2,1,1,2,2,2,2,2,2,1},
    {1,2,1,1,2,1,2,2,2,2,1,2,1,1,2,1},
    {1,2,2,2,2,1,1,1,1,1,1,2,2,2,2,1},
    {1,2,1,1,2,2,2,0,0,2,2,2,1,1,2,1},
    {1,2,2,2,2,1,1,1,1,1,1,2,2,2,2,1},
    {1,2,1,1,2,2,2,2,2,2,2,2,1,1,2,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
  };
  pacDotsLeft = 0;
  for (int y = 0; y < PAC_H; y++)
    for (int x = 0; x < PAC_W; x++) {
      pacMaze[y][x] = mazeTemplate[y][x];
      if (pacMaze[y][x] == 2) pacDotsLeft++;
    }
  pacX = 1; pacY = 1;
  pacDir = 1;
  ghostX = 14; ghostY = 1;
  ghostDir = 3;
  pacScore = 0;
  pacGameOver = false;
  pacWin = false;
  pacTimer = millis();
}

void loopPacman() {
  if (pacGameOver || pacWin) {
    if (digitalRead(BTN_MENU) == LOW) {
      delay(220);
      currentMode = MODE_TOP_MENU;
      showTopMenu();
    }
    return;
  }
  if (digitalRead(BTN_PREV) == LOW) {
    pacDir = (pacDir + 1) % 4;
    while (digitalRead(BTN_PREV) == LOW);
    delay(25);
  }
  if (digitalRead(BTN_NEXT) == LOW) {
    pacDir = (pacDir + 3) % 4;
    while (digitalRead(BTN_NEXT) == LOW);
    delay(25);
  }
  if (millis() - pacTimer >= pacSpeedMs) {
    pacTimer = millis();
    int nx = pacX, ny = pacY;
    if (pacDir == 0) ny--;
    if (pacDir == 1) nx++;
    if (pacDir == 2) ny++;
    if (pacDir == 3) nx--;
    if (nx >= 0 && nx < PAC_W && ny >= 0 && ny < PAC_H && pacMaze[ny][nx] != 1) {
      pacX = nx; pacY = ny;
      if (pacMaze[pacY][pacX] == 2) {
        pacMaze[pacY][pacX] = 0;
        pacScore += 10;
        pacDotsLeft--;
        if (pacDotsLeft <= 0) pacWin = true;
      }
    }
    int gx = ghostX, gy = ghostY;
    if (random(0, 3) == 0) ghostDir = random(0, 4);
    else {
      if (abs(pacX - ghostX) > abs(pacY - ghostY))
        ghostDir = (pacX > ghostX) ? 1 : 3;
      else
        ghostDir = (pacY > ghostY) ? 2 : 0;
    }
    if (ghostDir == 0) gy--;
    if (ghostDir == 1) gx++;
    if (ghostDir == 2) gy++;
    if (ghostDir == 3) gx--;
    if (gx >= 0 && gx < PAC_W && gy >= 0 && gy < PAC_H && pacMaze[gy][gx] != 1) {
      ghostX = gx; ghostY = gy;
    }
    if (pacX == ghostX && pacY == ghostY) pacGameOver = true;
    drawPacman();
  }
}

void drawPacman() {
  oled.clear();
  if (pacGameOver) {
    oled.setCursor(35, 2); oled.print(F("GAME OVER"));
    oled.setCursor(25, 4); oled.print(F("Очки: ")); oled.print(pacScore);
    oled.setCursor(15, 6); oled.print(F("MENU - выход"));
  } else if (pacWin) {
    oled.setCursor(40, 2); oled.print(F("ПОБЕДА!"));
    oled.setCursor(25, 4); oled.print(F("Очки: ")); oled.print(pacScore);
    oled.setCursor(15, 6); oled.print(F("MENU - выход"));
  } else {
    for (int y = 0; y < PAC_H; y++) {
      for (int x = 0; x < PAC_W; x++) {
        int px = x * 8;
        int py = y * 8;
        if (pacMaze[y][x] == 1) oled.rect(px, py, px + 7, py + 7, OLED_FILL);
        else if (pacMaze[y][x] == 2) oled.circle(px + 4, py + 4, 1, OLED_FILL);
      }
    }
    oled.circle(pacX * 8 + 4, pacY * 8 + 4, 3, OLED_FILL);
    oled.rect(ghostX * 8 + 1, ghostY * 8 + 1, ghostX * 8 + 6, ghostY * 8 + 6, OLED_STROKE);
    oled.setCursor(0, 0); oled.print(pacScore);
  }
  oled.update();
}

// ====================== КАЛЬКУЛЯТОР ======================
void initCalc() {
  calcNum1 = 0; calcNum2 = 0; calcResult = 0;
  calcOp = ' '; hasResult = false; enteringSecond = false;
  cursorX = 0; cursorY = 0;
  drawCalc();
}

void loopCalc() {
  if (digitalRead(BTN_PREV) == LOW) {
    delay(140);
    cursorX--;
    if (cursorX < 0) { cursorX = 3; cursorY = (cursorY + 3) % 4; }
    drawCalc();
    while (digitalRead(BTN_PREV) == LOW) delay(10);
  }
  if (digitalRead(BTN_NEXT) == LOW) {
    delay(140);
    cursorX++;
    if (cursorX > 3) { cursorX = 0; cursorY = (cursorY + 1) % 4; }
    drawCalc();
    while (digitalRead(BTN_NEXT) == LOW) delay(10);
  }
  if (digitalRead(BTN_MENU) == LOW) {
    delay(200);
    char clicked = grid[cursorY][cursorX];
    if (clicked >= '0' && clicked <= '9') {
      if (hasResult) initCalc();
      int d = clicked - '0';
      if (!enteringSecond) calcNum1 = calcNum1 * 10 + d;
      else calcNum2 = calcNum2 * 10 + d;
    } else if (clicked == '+' || clicked == '-' || clicked == '*' || clicked == '/') {
      if (hasResult) { calcNum1 = calcResult; calcNum2 = 0; hasResult = false; }
      calcOp = clicked;
      enteringSecond = true;
    } else if (clicked == '=') {
      if (enteringSecond) {
        if (calcOp == '+') calcResult = calcNum1 + calcNum2;
        else if (calcOp == '-') calcResult = calcNum1 - calcNum2;
        else if (calcOp == '*') calcResult = calcNum1 * calcNum2;
        else if (calcOp == '/') calcResult = (calcNum2 != 0) ? calcNum1 / calcNum2 : 0;
        hasResult = true;
      }
    } else if (clicked == 'C') initCalc();
    drawCalc();
    while (digitalRead(BTN_MENU) == LOW) delay(10);
  }
}

void drawCalc() {
  oled.clear();
  oled.setCursor(0, 0);
  if (hasResult) {
    oled.print(calcNum1); oled.print(' '); oled.print(calcOp); oled.print(' '); oled.print(calcNum2); oled.print('=');
    oled.setCursor(0, 1); oled.print(F("> ")); oled.print(calcResult);
  } else {
    oled.print(calcNum1);
    if (enteringSecond) {
      oled.print(' '); oled.print(calcOp); oled.print(' ');
      if (calcNum2 != 0 || calcNum1 == 0) oled.print(calcNum2);
    }
  }
  oled.setCursor(0, 2); oled.print(F("---------------------"));
  for (int y = 0; y < 4; y++) {
    oled.setCursor(10, 3 + y);
    for (int x = 0; x < 4; x++) {
      if (x == cursorX && y == cursorY) {
        oled.print('['); oled.print(grid[y][x]); oled.print(']');
      } else {
        oled.print(' '); oled.print(grid[y][x]); oled.print(' ');
      }
      oled.print(' ');
    }
  }
  oled.update();
}

// ====================== ТЕКСТ ======================
bool loadTextToRAM(const char* path) {
  if (!LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, "r");
  if (!f || f.size() == 0) { if (f) f.close(); return false; }
  textContent = f.readString();
  f.close();

  int len = textContent.length();
  int idx = 0;
  totalPages = 0;
  pagePointers[0] = 0;

  while (idx < len && totalPages < 99) {
    int lines = 0;
    while (idx < len && lines < 8) {
      int chars = 0;
      while (idx < len && chars < 21) {
        uint8_t c = textContent[idx];
        if (c == '\r') { idx++; continue; }
        if (c == '\n') { idx++; break; }
        if ((c & 0x80) == 0) { idx++; chars++; }
        else if ((c & 0xE0) == 0xC0) { idx += 2; chars++; }
        else if ((c & 0xF0) == 0xE0) { idx += 3; chars += 2; }
        else { idx++; chars++; }
      }
      lines++;
    }
    totalPages++;
    if (idx < len) pagePointers[totalPages] = idx;
  }
  return true;
}

void renderTextPage() {
  oled.clear();
  int start = pagePointers[currentPage];
  int end = (currentPage + 1 < totalPages) ? pagePointers[currentPage + 1] : textContent.length();
  int idx = start;
  int lines = 0;
  while (idx < end && lines < 8) {
    String line = "";
    int chars = 0;
    while (idx < end && chars < 21) {
      uint8_t c = textContent[idx];
      if (c == '\r') { idx++; continue; }
      if (c == '\n') { idx++; break; }
      if ((c & 0x80) == 0) { line += (char)c; chars++; idx++; }
      else if ((c & 0xE0) == 0xC0) {
        line += (char)c; line += textContent[idx+1];
        chars++; idx += 2;
      } else if ((c & 0xF0) == 0xE0) {
        line += '?'; chars++; idx += 3;
      } else idx++;
    }
    oled.setCursor(0, lines);
    oled.print(line);
    lines++;
  }
  oled.update();
}

void showInfoScreen() {
  oled.clear();
  oled.setCursor(0, 0); oled.print(F("=== ИНФОРМАЦИЯ ==="));
  oled.setCursor(0, 2); oled.print(F("Стр: ")); oled.print(currentPage+1); oled.print('/'); oled.print(totalPages);
  oled.setCursor(0, 4); oled.print(F("IP: 192.168.4.1"));
  int raw = analogRead(BAT_PIN);
  float v = (raw * 3.3 / 4095.0) * 2.0;
  int pct = constrain(map(v * 100, 340, 420, 0, 100), 0, 100);
  oled.setCursor(0, 6); oled.print(F("Заряд: ")); oled.print(pct); oled.print('%');
  oled.update();
}

// ====================== ВЕБ-ИНТЕРФЕЙС ======================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
<title>ESP32 Cheat</title>
<style>
:root{--bg:#0f1115;--card:#1a1d24;--border:#2a2f3a;--accent:#3b82f6;--accent-h:#2563eb;--text:#e5e7eb;--muted:#9ca3af;--danger:#ef4444;--ok:#22c55e}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);padding:16px;max-width:900px;margin:0 auto;line-height:1.5}
h1{font-size:1.5rem;margin-bottom:4px}
.sub{color:var(--muted);font-size:.9rem;margin-bottom:20px}
.card{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:16px;margin-bottom:16px}
.btn{display:inline-flex;align-items:center;gap:6px;background:var(--accent);color:#fff;border:none;border-radius:8px;padding:10px 16px;font-size:.95rem;cursor:pointer}
.btn:hover{background:var(--accent-h)}
.btn-out{background:transparent;border:1px solid var(--border);color:var(--text)}
.btn-out:hover{border-color:var(--accent);color:var(--accent)}
.btn-d{background:var(--danger)}
.btn-sm{padding:6px 12px;font-size:.85rem}
input[type=text],textarea{width:100%;background:var(--bg);border:1px solid var(--border);border-radius:8px;color:var(--text);padding:12px;font-size:1rem;font-family:inherit}
textarea{min-height:300px;resize:vertical;line-height:1.6}
input:focus,textarea:focus{outline:none;border-color:var(--accent)}
.file-list{list-style:none}
.file-item{display:flex;align-items:center;justify-content:space-between;padding:12px 14px;border-bottom:1px solid var(--border);gap:12px}
.file-item:last-child{border-bottom:none}
.file-name{flex:1;font-weight:500;cursor:pointer;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.file-name:hover{color:var(--accent)}
.file-act{display:flex;gap:8px;flex-shrink:0}
.empty{text-align:center;color:var(--muted);padding:40px 20px}
.ed-h{display:flex;gap:10px;margin-bottom:12px;flex-wrap:wrap}
.status{font-size:.85rem;color:var(--muted);margin-top:8px}
.status.ok{color:var(--ok)}
.status.err{color:var(--danger)}
.hidden{display:none!important}
@media(max-width:600px){.file-item{flex-direction:column;align-items:flex-start}.file-act{width:100%}}
</style>
</head>
<body>
<h1>ESP32 Cheat</h1>
<p class="sub">Управление шпаргалками · файлы в памяти устройства</p>

<div id="listView">
  <div class="card">
    <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px">
      <strong>Файлы шпаргалок</strong>
      <button class="btn" onclick="showCreate()">+ Новый файл</button>
    </div>
    <ul class="file-list" id="fileList"><li class="empty">Загрузка...</li></ul>
  </div>
</div>

<div id="createView" class="card hidden">
  <h3 style="margin-bottom:12px">Новый файл</h3>
  <input type="text" id="newName" placeholder="Название (например math.txt)" maxlength="32">
  <div style="margin-top:12px;display:flex;gap:10px">
    <button class="btn" onclick="createFile()">Создать</button>
    <button class="btn btn-out" onclick="showList()">Отмена</button>
  </div>
  <div class="status" id="createStatus"></div>
</div>

<div id="editorView" class="card hidden">
  <div class="ed-h">
    <input type="text" id="editName" style="flex:1;min-width:140px" maxlength="32">
    <button class="btn" onclick="saveFile()">Сохранить</button>
    <button class="btn btn-out" onclick="showList()">Назад</button>
  </div>
  <textarea id="editContent" placeholder="Текст шпаргалки..."></textarea>
  <div class="status" id="editStatus"></div>
</div>

<script>
let currentFile=null;
async function loadList(){
  const r=await fetch('/list');
  const files=await r.json();
  const ul=document.getElementById('fileList');
  if(!files.length){ul.innerHTML='<li class="empty">Нет файлов.<br>Нажми «+ Новый файл»</li>';return;}
  ul.innerHTML=files.map(f=>`<li class="file-item"><span class="file-name" onclick="openFile('${f}')">${f}</span><div class="file-act"><button class="btn btn-sm btn-out" onclick="openFile('${f}')">Открыть</button><button class="btn btn-sm btn-d" onclick="deleteFile('${f}')">Удалить</button></div></li>`).join('');
}
function showList(){document.getElementById('listView').classList.remove('hidden');document.getElementById('createView').classList.add('hidden');document.getElementById('editorView').classList.add('hidden');loadList();}
function showCreate(){document.getElementById('listView').classList.add('hidden');document.getElementById('createView').classList.remove('hidden');document.getElementById('editorView').classList.add('hidden');document.getElementById('newName').value='';document.getElementById('createStatus').textContent='';}
async function createFile(){
  let name=document.getElementById('newName').value.trim();
  if(!name)return alert('Введи название');
  if(!name.endsWith('.txt'))name+='.txt';
  name=name.replace(/[^a-zA-Z0-9а-яА-ЯёЁ_\-\.]/g,'_');
  const r=await fetch('/create',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+encodeURIComponent(name)});
  if(r.ok){currentFile=name;document.getElementById('editName').value=name;document.getElementById('editContent').value='';document.getElementById('editStatus').textContent='';document.getElementById('listView').classList.add('hidden');document.getElementById('createView').classList.add('hidden');document.getElementById('editorView').classList.remove('hidden');}
  else{document.getElementById('createStatus').textContent=await r.text();document.getElementById('createStatus').className='status err';}
}
async function openFile(name){
  const r=await fetch('/file?name='+encodeURIComponent(name));
  if(!r.ok)return alert('Ошибка');
  currentFile=name;
  document.getElementById('editName').value=name;
  document.getElementById('editContent').value=await r.text();
  document.getElementById('editStatus').textContent='';
  document.getElementById('listView').classList.add('hidden');
  document.getElementById('createView').classList.add('hidden');
  document.getElementById('editorView').classList.remove('hidden');
}
async function saveFile(){
  const name=document.getElementById('editName').value.trim();
  const content=document.getElementById('editContent').value;
  if(!name)return alert('Название пустое');
  if(currentFile&&name!==currentFile){
    const ren=await fetch('/rename',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'old='+encodeURIComponent(currentFile)+'&new='+encodeURIComponent(name)});
    if(!ren.ok){document.getElementById('editStatus').textContent=await ren.text();document.getElementById('editStatus').className='status err';return;}
    currentFile=name;
  }
  const r=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+encodeURIComponent(name)+'&text='+encodeURIComponent(content)});
  if(r.ok){document.getElementById('editStatus').textContent='Сохранено ✓';document.getElementById('editStatus').className='status ok';setTimeout(()=>document.getElementById('editStatus').textContent='',2500);}
  else{document.getElementById('editStatus').textContent=await r.text();document.getElementById('editStatus').className='status err';}
}
async function deleteFile(name){
  if(!confirm('Удалить «'+name+'»?'))return;
  await fetch('/delete',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'name='+encodeURIComponent(name)});
  loadList();
}
loadList();
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleList() {
  String json = "[";
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  bool first = true;
  while (file) {
    String name = file.name();
    if (name.startsWith("/")) name = name.substring(1);
    if (name.endsWith(".txt")) {
      if (!first) json += ",";
      json += "\"" + name + "\"";
      first = false;
    }
    file = root.openNextFile();
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleFile() {
  if (!server.hasArg("name")) { server.send(400, "text/plain", "Нет имени"); return; }
  String name = server.arg("name");
  if (!name.startsWith("/")) name = "/" + name;
  if (!LittleFS.exists(name)) { server.send(404, "text/plain", "Не найден"); return; }
  File f = LittleFS.open(name, "r");
  String content = f.readString();
  f.close();
  server.send(200, "text/plain; charset=utf-8", content);
}

void handleCreate() {
  if (!server.hasArg("name")) { server.send(400, "text/plain", "Нет имени"); return; }
  String name = server.arg("name");
  if (!name.startsWith("/")) name = "/" + name;
  if (LittleFS.exists(name)) { server.send(400, "text/plain", "Уже существует"); return; }
  File f = LittleFS.open(name, "w");
  if (!f) { server.send(500, "text/plain", "Ошибка"); return; }
  f.close();
  server.send(200, "text/plain", "OK");
}

void handleSave() {
  if (!server.hasArg("name") || !server.hasArg("text")) { server.send(400, "text/plain", "Нет данных"); return; }
  String name = server.arg("name");
  if (!name.startsWith("/")) name = "/" + name;
  File f = LittleFS.open(name, "w");
  if (!f) { server.send(500, "text/plain", "Ошибка"); return; }
  f.print(server.arg("text"));
  f.close();
  server.send(200, "text/plain", "OK");
}

void handleRename() {
  if (!server.hasArg("old") || !server.hasArg("new")) { server.send(400, "text/plain", "Нет данных"); return; }
  String oldName = server.arg("old");
  String newName = server.arg("new");
  if (!oldName.startsWith("/")) oldName = "/" + oldName;
  if (!newName.startsWith("/")) newName = "/" + newName;
  if (!LittleFS.exists(oldName)) { server.send(404, "text/plain", "Не найден"); return; }
  if (LittleFS.exists(newName)) { server.send(400, "text/plain", "Уже есть"); return; }
  if (LittleFS.rename(oldName, newName)) server.send(200, "text/plain", "OK");
  else server.send(500, "text/plain", "Ошибка");
}

void handleDelete() {
  if (!server.hasArg("name")) { server.send(400, "text/plain", "Нет имени"); return; }
  String name = server.arg("name");
  if (!name.startsWith("/")) name = "/" + name;
  if (LittleFS.remove(name)) server.send(200, "text/plain", "OK");
  else server.send(500, "text/plain", "Ошибка");
}