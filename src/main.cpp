#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

// ST7735 + SD share the VSPI bus.
constexpr int TFT_CS = 5;
constexpr int TFT_RST = 17;
constexpr int TFT_DC = 16;
constexpr int TFT_SDA = 11; // MOSI
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

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);

struct Weather {
  bool valid = false;
  float temperature = NAN;
  float feelsLike = NAN;
  float humidity = NAN;
  float wind = NAN;
  int code = -1;
  String sunrise;
  String sunset;
  String updated;
};

Weather weather;
uint32_t lastFetch = 0;
uint8_t page = 0;
bool fahrenheit = false;
bool sdAvailable = false;

const int buttonPins[] = {BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT,
                          BUTTON_A, BUTTON_B, BUTTON_C, BUTTON_D, BUTTON_VOL};
bool previousButtons[9] = {};

String conditionName(int code) {
  if (code == 0) return "Clear";
  if (code <= 3) return "Cloudy";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Showers";
  if (code >= 85 && code <= 86) return "Snowfall";
  if (code >= 95) return "Storm";
  return "Unknown";
}

String shortTime(const char* value) {
  String s(value ? value : "--:--");
  int separator = s.indexOf('T');
  return separator >= 0 ? s.substring(separator + 1, separator + 6) : s;
}

String temperatureText(float value) {
  if (isnan(value)) return "--";
  if (fahrenheit) value = value * 9.0f / 5.0f + 32.0f;
  return String(value, 1) + (fahrenheit ? " F" : " C");
}

void showStatus(const String& line1, const String& line2 = "") {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0); oled.println("WEATHER DASHBOARD");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setCursor(0, 20); oled.println(line1);
  oled.setCursor(0, 35); oled.println(line2);
  oled.display();
}

void drawOLED() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0); oled.println(WEATHER_LOCATION);
  oled.drawLine(0, 9, 127, 9, SSD1306_WHITE);
  if (WiFi.status() == WL_CONNECTED) {
    oled.setCursor(0, 14); oled.print("WiFi OK  "); oled.println(WiFi.localIP());
  } else {
    oled.setCursor(0, 14); oled.println("WiFi offline");
  }
  oled.setCursor(0, 29);
  oled.print(weather.valid ? temperatureText(weather.temperature) : "No data");
  oled.print("  "); oled.println(weather.valid ? conditionName(weather.code) : "-");
  oled.setCursor(0, 44); oled.print("A/C page  B refresh");
  oled.display();
}

void drawTFT() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextColor(ST77XX_CYAN); tft.setTextSize(1);
  tft.setCursor(4, 4); tft.print(WEATHER_LOCATION);
  tft.setCursor(105, 4); tft.print(String(page + 1) + "/3");
  tft.drawFastHLine(0, 16, 128, ST77XX_BLUE);

  if (!weather.valid) {
    tft.setTextColor(ST77XX_WHITE); tft.setCursor(10, 55); tft.println("Waiting for");
    tft.setCursor(10, 70); tft.println("weather data...");
    return;
  }

  tft.setTextColor(ST77XX_WHITE);
  if (page == 0) {
    tft.setTextSize(2); tft.setCursor(6, 27); tft.println(temperatureText(weather.temperature));
    tft.setTextSize(1); tft.setCursor(6, 58); tft.println(conditionName(weather.code));
    tft.setCursor(6, 78); tft.print("Feels: "); tft.println(temperatureText(weather.feelsLike));
    tft.setCursor(6, 94); tft.print("Humidity: "); tft.print(weather.humidity, 0); tft.println("%");
    tft.setCursor(6, 110); tft.print("Wind: "); tft.print(weather.wind, 1); tft.println(" km/h");
  } else if (page == 1) {
    tft.setCursor(6, 30); tft.println("Today");
    tft.setCursor(6, 48); tft.print("Sunrise: "); tft.println(weather.sunrise);
    tft.setCursor(6, 66); tft.print("Sunset:  "); tft.println(weather.sunset);
    tft.setCursor(6, 96); tft.println("Data from");
    tft.setCursor(6, 112); tft.println("Open-Meteo");
  } else {
    tft.setCursor(6, 30); tft.println("Controls");
    tft.setCursor(6, 48); tft.println("A/C  change page");
    tft.setCursor(6, 64); tft.println("B    refresh now");
    tft.setCursor(6, 80); tft.println("D    C/F units");
    tft.setCursor(6, 96); tft.println("VOL  WiFi status");
    tft.setCursor(6, 120); tft.println("Auto refresh: 15m");
  }
}

bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return false;
  showStatus("Downloading data...", WEATHER_LOCATION);
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(WEATHER_LATITUDE, 4) +
               "&longitude=" + String(WEATHER_LONGITUDE, 4) +
               "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m" +
               "&daily=sunrise,sunset&timezone=" + WEATHER_TIMEZONE;
  HTTPClient http;
  http.setTimeout(12000);
  http.begin(url);
  int status = http.GET();
  if (status != HTTP_CODE_OK) { http.end(); return false; }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, http.getStream());
  http.end();
  if (error) return false;

  JsonObject current = doc["current"];
  weather.temperature = current["temperature_2m"] | NAN;
  weather.feelsLike = current["apparent_temperature"] | NAN;
  weather.humidity = current["relative_humidity_2m"] | NAN;
  weather.wind = current["wind_speed_10m"] | NAN;
  weather.code = current["weather_code"] | -1;
  weather.sunrise = shortTime(doc["daily"]["sunrise"][0] | "");
  weather.sunset = shortTime(doc["daily"]["sunset"][0] | "");
  weather.updated = String(millis() / 1000);
  weather.valid = true;
  lastFetch = millis();
  drawOLED(); drawTFT();
  return true;
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  showStatus("Connecting WiFi...", WEATHER_LOCATION);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
  }
}

bool pressed(uint8_t index) {
  bool current = digitalRead(buttonPins[index]) == LOW;
  bool result = current && !previousButtons[index];
  previousButtons[index] = current;
  return result;
}

void setup() {
  Serial.begin(115200);
  for (int pin : buttonPins) pinMode(pin, INPUT_PULLUP);
  pinMode(TFT_LED, OUTPUT); digitalWrite(TFT_LED, HIGH);
  pinMode(SD_CS, OUTPUT); digitalWrite(SD_CS, HIGH);

  SPI.begin(TFT_SCK, SD_MISO, TFT_SDA);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  Wire.begin(OLED_SDA, OLED_SCL);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  showStatus("Starting...", WEATHER_LOCATION);

  sdAvailable = SD.begin(SD_CS, SPI);
  if (sdAvailable) Serial.println("SD ready; /DANDY can be used for game files.");
  connectWiFi();
  if (!fetchWeather()) showStatus("Weather unavailable", "Press B to retry");
  drawOLED(); drawTFT();
}

void loop() {
  if (pressed(4) || pressed(6)) { page = (page + (pressed(4) ? 1 : 2)) % 3; drawOLED(); drawTFT(); }
  if (pressed(5)) { fetchWeather(); }
  if (pressed(7)) { fahrenheit = !fahrenheit; drawOLED(); drawTFT(); }
  if (pressed(8)) { showStatus(WiFi.status() == WL_CONNECTED ? "WiFi connected" : "WiFi offline", WiFi.localIP().toString()); delay(800); drawOLED(); }

  if (millis() - lastFetch > 900000UL) fetchWeather();
  delay(25);
}
