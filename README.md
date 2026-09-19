# ESP32-S3 Weather Dashboard

Weather dashboard for the ESP32-S3-DevKitC-1 with:

- ST7735S 128x160 TFT over SPI;
- SSD1306 128x64 OLED over I2C;
- Wi-Fi and the public Open-Meteo API (no API key);
- automatic refresh every 15 minutes;
- metric/imperial temperature toggle;
- optional SD-card initialization on the shared SPI bus.

## Setup

1. Install PlatformIO.
2. Copy `src/secrets.h.example` to `src/secrets.h`.
3. Set `WIFI_SSID`, `WIFI_PASSWORD`, and the location coordinates in `src/secrets.h`.
4. Build and upload with `pio run -t upload`.

`src/secrets.h` is intentionally not committed. Add it to `.gitignore` if you create it locally.

## Controls

- **A**: next screen
- **C**: previous screen
- **B**: download weather now
- **D**: switch Celsius/Fahrenheit
- **VOL**: show Wi-Fi status

The SD card is initialized with CS on GPIO 10 and remains compatible with a future `/DANDY` games folder.
