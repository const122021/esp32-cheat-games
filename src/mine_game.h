#pragma once

// MINE 2.0 для основного скетча TFT_eSPI.
// Использует объекты и константы, объявленные до #include "mine_game.h":
// tft, volume, BUZZ_CHANNEL, sdOk, COL_BG, COL_RED, COL_WHITE,
// COL_GRAY, COL_LIGHT, COL_ITEM_BG.

#ifndef BTN_C
#define BTN_C 46
#endif
#ifndef BTN_D
#define BTN_D 45
#endif

#define MINE_W 32
#define MINE_H 24
#define MINE_TILE 8
#define MINE_VIEW_W 20
#define MINE_VIEW_H 12

// Типы блоков мира.
enum MineTile : uint8_t {
  M_AIR, M_GRASS, M_DIRT, M_STONE, M_WATER, M_SAND,
  M_WOOD, M_LEAVES, M_MOUNTAIN, M_PIG
};

struct MineItem {
  uint8_t id;
  uint8_t count;
};

enum MineItemId : uint8_t {
  I_EMPTY, I_DIRT, I_STONE, I_SAND, I_WOOD, I_LEAVES,
  I_PIG, I_PLANK, I_PICKAXE, I_AXE, I_TORCH
};

MineTile mineWorld[MINE_W][MINE_H];
MineItem mineInventory[12];
int mineX = 5, mineY = 5;
int mineCameraX = 0, mineCameraY = 0;
int mineHealth = 100, mineFood = 100;
int mineSelected = 0;
int mineWorldSelected = 0;
int mineWorldMode = 0; // 0 = survival, 1 = creative
bool mineDWasDown = false;
uint32_t mineDStarted = 0;
uint32_t mineLastTick = 0;

uint16_t mineColor(MineTile t) {
  switch (t) {
    case M_GRASS: return 0x05E0;
    case M_DIRT: return 0xA145;
    case M_STONE: return COL_GRAY;
    case M_WATER: return 0x025F;
    case M_SAND: return 0xFEA0;
    case M_WOOD: return 0xA260;
    case M_LEAVES: return 0x03A0;
    case M_MOUNTAIN: return 0x7BEF;
    case M_PIG: return 0xF81F;
    default: return COL_BG;
  }
}

const char* mineItemName(uint8_t id) {
  switch (id) {
    case I_DIRT: return "DIRT";
    case I_STONE: return "STONE";
    case I_SAND: return "SAND";
    case I_WOOD: return "WOOD";
    case I_LEAVES: return "LEAF";
    case I_PIG: return "PIG";
    case I_PLANK: return "PLANK";
    case I_PICKAXE: return "PICK";
    case I_AXE: return "AXE";
    case I_TORCH: return "TORCH";
    default: return "EMPTY";
  }
}

void mineClearInventory() {
  for (int i = 0; i < 12; ++i) { mineInventory[i].id = I_EMPTY; mineInventory[i].count = 0; }
  mineInventory[0] = {I_DIRT, 8};
  mineInventory[1] = {I_STONE, 8};
  mineInventory[2] = {I_WOOD, 4};
  mineInventory[3] = {I_SAND, 8};
}

void mineGive(uint8_t id, uint8_t amount = 1) {
  for (int i = 0; i < 12; ++i) {
    if (mineInventory[i].id == id) { mineInventory[i].count += amount; return; }
  }
  for (int i = 0; i < 12; ++i) {
    if (mineInventory[i].id == I_EMPTY) { mineInventory[i] = {id, amount}; return; }
  }
}

bool mineTake(uint8_t id, uint8_t amount = 1) {
  for (int i = 0; i < 12; ++i) {
    if (mineInventory[i].id == id && mineInventory[i].count >= amount) {
      mineInventory[i].count -= amount;
      if (!mineInventory[i].count) mineInventory[i].id = I_EMPTY;
      return true;
    }
  }
  return false;
}

void mineGenerateWorld(uint32_t seed) {
  randomSeed(seed);
  for (int y = 0; y < MINE_H; ++y) {
    for (int x = 0; x < MINE_W; ++x) {
      int wave = (int)(sin((x + seed % 17) * 0.45f) * 3.0f + cos((y + seed % 23) * 0.55f) * 2.0f);
      if (abs(x - (9 + (int)sin(y * .4f) * 3)) <= 1) mineWorld[x][y] = M_WATER; // река
      else if (wave >= 4 || (x > 25 && random(100) < 12)) mineWorld[x][y] = M_MOUNTAIN; // горы
      else if (wave <= -2) mineWorld[x][y] = M_WATER;
      else if (wave == -1) mineWorld[x][y] = M_SAND; // пляж
      else mineWorld[x][y] = (random(100) < 20) ? M_DIRT : M_GRASS;
    }
  }

  // Деревья и листья.
  for (int n = 0; n < 22; ++n) {
    int x = random(2, MINE_W - 2), y = random(2, MINE_H - 2);
    if (mineWorld[x][y] != M_GRASS) continue;
    mineWorld[x][y] = M_WOOD;
    if (mineWorld[x - 1][y] == M_GRASS) mineWorld[x - 1][y] = M_LEAVES;
    if (mineWorld[x + 1][y] == M_GRASS) mineWorld[x + 1][y] = M_LEAVES;
    if (mineWorld[x][y - 1] == M_GRASS) mineWorld[x][y - 1] = M_LEAVES;
    if (mineWorld[x][y + 1] == M_GRASS) mineWorld[x][y + 1] = M_LEAVES;
  }

  // Свиньи.
  for (int n = 0; n < 8; ++n) {
    int x = random(1, MINE_W - 1), y = random(1, MINE_H - 1);
    if (mineWorld[x][y] == M_GRASS) mineWorld[x][y] = M_PIG;
  }

  mineX = 2; mineY = 2;
  while (mineWorld[mineX][mineY] == M_WATER || mineWorld[mineX][mineY] == M_MOUNTAIN) {
    mineX = random(1, MINE_W - 1); mineY = random(1, MINE_H - 1);
  }
  mineHealth = 100; mineFood = 100; mineSelected = 0;
  mineClearInventory();
}

void mineText(const char* s, int x, int y, uint16_t fg = COL_WHITE, uint16_t bg = COL_BG, uint8_t size = 1) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(fg, bg);
  tft.drawString(s, x, y, size);
}

void mineDrawWorld() {
  mineCameraX = constrain(mineX - 9, 0, MINE_W - MINE_VIEW_W);
  mineCameraY = constrain(mineY - 5, 0, MINE_H - MINE_VIEW_H);
  tft.fillScreen(COL_BG);
  tft.fillRect(0, 0, 160, 17, COL_BG);
  mineText("MINE 2.0", 3, 2, COL_RED);
  char hud[30];
  sprintf(hud, "%s HP:%d F:%d", mineWorldMode ? "CRE" : "SUR", mineHealth, mineFood);
  mineText(hud, 62, 2, COL_WHITE);

  for (int sy = 0; sy < MINE_VIEW_H; ++sy) for (int sx = 0; sx < MINE_VIEW_W; ++sx) {
    int wx = mineCameraX + sx, wy = mineCameraY + sy;
    tft.fillRect(sx * MINE_TILE, 17 + sy * MINE_TILE, MINE_TILE, MINE_TILE, mineColor(mineWorld[wx][wy]));
  }
  int px = (mineX - mineCameraX) * MINE_TILE;
  int py = 17 + (mineY - mineCameraY) * MINE_TILE;
  tft.fillRect(px + 1, py + 1, 6, 6, COL_WHITE);
  mineText("A:BREAK  C:BUILD  D:INV", 2, 116, COL_GRAY);
  mineText("B:EXIT", 122, 116, COL_GRAY);
}

void mineDrawInventory() {
  tft.fillScreen(COL_BG);
  tft.fillRect(0, 0, 160, 3, COL_RED);
  mineText("INVENTORY / CRAFT", 4, 7, COL_WHITE);
  for (int i = 0; i < 12; ++i) {
    int x = 4 + (i % 4) * 39, y = 28 + (i / 4) * 25;
    uint16_t bg = i == mineSelected ? COL_WHITE : COL_ITEM_BG;
    tft.fillRoundRect(x, y, 35, 21, 2, bg);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(i == mineSelected ? COL_BG : COL_LIGHT, bg);
    tft.drawString(mineItemName(mineInventory[i].id), x + 17, y + 7, 1);
    tft.drawNumber(mineInventory[i].count, x + 17, y + 16, 1);
  }
  mineText("A CRAFT  L/R SELECT  B BACK", 3, 116, COL_GRAY);
}

void mineCraft() {
  // Рецепты: 2 дерева -> доски, 2 камня + дерево -> кирка, 3 песка -> факел.
  if (mineSelected == 0 && mineTake(I_WOOD, 2)) mineGive(I_PLANK, 4);
  else if (mineSelected == 1 && mineTake(I_STONE, 2) && mineTake(I_WOOD, 1)) mineGive(I_PICKAXE);
  else if (mineSelected == 2 && mineTake(I_SAND, 3)) mineGive(I_TORCH);
  else { beep(220, 80); return; }
  beep(1400, 50);
}

void mineBreak() {
  int tx = mineX, ty = mineY + 1;
  if (ty >= MINE_H) return;
  MineTile b = mineWorld[tx][ty];
  if (b == M_WATER || b == M_AIR || b == M_MOUNTAIN) return;
  if (mineWorldMode) return;
  if (b == M_PIG) { mineGive(I_PIG); mineFood = min(100, mineFood + 15); }
  else if (b == M_WOOD) mineGive(I_WOOD);
  else if (b == M_LEAVES) mineGive(I_LEAVES);
  else if (b == M_STONE) mineGive(I_STONE);
  else if (b == M_SAND) mineGive(I_SAND);
  else mineGive(b == M_DIRT ? I_DIRT : I_DIRT);
  mineWorld[tx][ty] = M_GRASS;
  beep(900, 25);
}

void mineBuild() {
  int tx = mineX, ty = mineY + 1;
  if (ty >= MINE_H || mineWorldMode == 0) return;
  MineItem &it = mineInventory[mineSelected];
  MineTile tile = M_AIR;
  if (it.id == I_DIRT) tile = M_DIRT;
  else if (it.id == I_STONE) tile = M_STONE;
  else if (it.id == I_SAND) tile = M_SAND;
  else if (it.id == I_WOOD) tile = M_WOOD;
  else return;
  if (it.count) { mineWorld[tx][ty] = tile; if (--it.count == 0) it.id = I_EMPTY; beep(1100, 25); }
}

void mineMove(int dx, int dy) {
  int nx = mineX + dx, ny = mineY + dy;
  if (nx < 0 || nx >= MINE_W || ny < 0 || ny >= MINE_H) return;
  if (mineWorld[nx][ny] == M_WATER || mineWorld[nx][ny] == M_MOUNTAIN) return;
  if (mineWorld[nx][ny] == M_PIG && !mineWorldMode) mineHealth = max(0, mineHealth - 5);
  mineX = nx; mineY = ny;
  if (!mineWorldMode && millis() - mineLastTick > 5000) { mineFood = max(0, mineFood - 1); mineLastTick = millis(); }
}

void mineRun() {
  // Выбор мира и режима перед игрой.
  int menu = 0;
  bool inMenu = true;
  mineGenerateWorld(millis());
  while (inMenu) {
    tft.fillScreen(COL_BG);
    mineText("MINE 2.0", 4, 6, COL_RED, COL_BG, 2);
    mineText("NEW WORLD", 12, 38, menu == 0 ? COL_BG : COL_LIGHT, menu == 0 ? COL_WHITE : COL_ITEM_BG);
    mineText("SURVIVAL", 12, 55, menu == 1 ? COL_BG : COL_LIGHT, menu == 1 ? COL_WHITE : COL_ITEM_BG);
    mineText("CREATIVE", 12, 72, menu == 2 ? COL_BG : COL_LIGHT, menu == 2 ? COL_WHITE : COL_ITEM_BG);
    mineText("A:SELECT  B:BACK", 4, 116, COL_GRAY);
    if (digitalRead(BTN_UP) == LOW) { menu = (menu + 2) % 3; beep(700, 15); delay(150); }
    if (digitalRead(BTN_DOWN) == LOW) { menu = (menu + 1) % 3; beep(700, 15); delay(150); }
    if (digitalRead(BTN_B) == LOW) { delay(180); return; }
    if (digitalRead(BTN_A) == LOW) {
      if (menu == 0) mineGenerateWorld(millis());
      else { mineWorldMode = menu == 2; mineGenerateWorld(millis()); inMenu = false; }
      beep(1200, 35); delay(180);
    }
  }

  mineDrawWorld();
  while (true) {
    bool d = digitalRead(BTN_D) == LOW;
    if (d && !mineDWasDown) mineDStarted = millis();
    if (d && millis() - mineDStarted > 450) {
      mineDrawInventory();
      while (digitalRead(BTN_D) == LOW) delay(10);
      while (true) {
        if (digitalRead(BTN_LEFT) == LOW) { mineSelected = (mineSelected + 11) % 12; mineDrawInventory(); delay(130); }
        else if (digitalRead(BTN_RIGHT) == LOW) { mineSelected = (mineSelected + 1) % 12; mineDrawInventory(); delay(130); }
        else if (digitalRead(BTN_A) == LOW) { mineCraft(); mineDrawInventory(); delay(180); }
        else if (digitalRead(BTN_B) == LOW) { delay(180); break; }
      }
      mineDrawWorld();
    }
    mineDWasDown = d;
    if (digitalRead(BTN_UP) == LOW) { mineMove(0, -1); mineDrawWorld(); delay(120); }
    else if (digitalRead(BTN_DOWN) == LOW) { mineMove(0, 1); mineDrawWorld(); delay(120); }
    else if (digitalRead(BTN_LEFT) == LOW) { mineMove(-1, 0); mineDrawWorld(); delay(120); }
    else if (digitalRead(BTN_RIGHT) == LOW) { mineMove(1, 0); mineDrawWorld(); delay(120); }
    else if (digitalRead(BTN_A) == LOW) { mineBreak(); mineDrawWorld(); delay(160); }
    else if (digitalRead(BTN_C) == LOW) { mineBuild(); mineDrawWorld(); delay(160); }
    else if (digitalRead(BTN_B) == LOW) { delay(180); return; }
    if (mineHealth <= 0 || mineFood <= 0) { mineHealth = 100; mineFood = 100; mineDrawWorld(); }
    delay(15);
  }
}

void runMineGame() { mineRun(); }
