# ESP-SimHub - Projekt-Memory

## Projekt-Informationen

**Name:** ESP-SimHub GT Wheel  
**Zweck:** Firmware für ESP32/Arduino-Controller für Sim Racing Wheels  
**Remote:** https://github.com/RonBO125/ESP-SimHub.git  
**Framework:** PlatformIO (Arduino)  
**Primärer MCU:** ESP32 (auch Arduino Micro/Mega unterstützt)  
**Version:** 'j' (J revision)  

## Architektur

### Kommunikationsprotokoll
- **Standard:** Eigenes ARQ-basiertes Serial-Protokoll (19200 Baud)
- **Verbindung:** USB-Serial zum PC (SimHub)
- **Bridge-Option:** ESP-Now Bridge für drahtlose Verbindung

### Hauptkomponenten
- `main.cpp` - Haupt firmware mit allen Hardware-Konfigurationen
- `SHCustomProtocol.h` - Benutzerdefinierte Protokoll-Unterstützung
- `SHCommands.h` - SimHub Command Handler
- `ArqSerial.h` - ARQ Serial Protokoll Implementierung

## Unterstützte Hardware

### Displays & 7-Segment Module
- **TM1638** - 7-Segment Module mit Tastatur (bis zu mehrere Module)
- **TM1637** - 7-Segment Module (4-6 Digit)
- **TM1637 6-Digit** - Spezielle 6-Digit Variante
- **MAX7219/MAX7221** - 7-Segment und LED-Matrix Module
- **74HC595** - Schieberegister für Ganganzeige
- **6c595** - Alternative Ganganzeige

### I2C Displays
- **OLED GLCD** - SSD1306 0.96" (I2C)
- **Nokia GLCD** - PCD8544 (SPI)
- **I2C LCD** - PCF8574 AT/T (20x4, 16x2)
- **Adafruit HT16K33** - 7-Segment Backpack, Bi-Color Matrix, Single-Color Matrix

### RGB LEDs
- **WS2812B** - Neopixel/NeoPixelBus Support
- **WS2801** - SPI RGB LEDs
- **PL9823** - High-Brightness RGB LEDs
- **DM163 Matrix** - 8x8 RGB Matrix (Sunfounder/Colorduino)
- **Sunfounder SH104P** - I2C RGB 8x8 Matrix
- **NeopixelBus** - Alternative Library mit FastLED Support

### Input Devices
- **Rotary Encoder** - Bis zu 8 Encoder (CLK/DT/SW)
- **Direkte Buttons** - Bis zu 12 Buttons
- **Button Matrix** - Bis zu 8x8 Matrix
- **Gamepad-Achsen** - Throttle, Accelerator, Brake (analog)
- **USB Gamepad** - ESP32 als USB-Gamepad (bis zu 12 Encoder + Buttons als Gamepad-Inputs)

### Shake-It Force Feedback
- **Adafruit Motor Shield V2** - Bis zu 3 Shields (1900Hz PWM)
- **DK Motor Shield** - Deprecated
- **L298N Board** - Dual H-Bridge
- **MotoMonster** - VNH2SP30 basierend
- **Dual VNH5019** - Dual Motor Driver Board
- **PWM Outputs** - Bis zu 4 direkte PWM-Ausgänge (für Motoren)
- **PWM Fans** - Bis zu 4 PWM-Fan-Ausgänge (25kHz) mit Relay-Steuerung

### After-Market Gauges
- **Tachometer** - After-Market Drehzahlmesser
- **Speedometer** - After-Market Tachometer
- **Boost Gauge** - After-Market Ladedrucksensor
- **Wassertemperatur** - E36 Temperaturmesser (deprecated)
- **Kraftstoff** - E36 Kraftstoffmesser (deprecated)
- **Verbrauch** - After-Market Verbrauchsmesser (deprecated)

## Konfiguration

### Connection Types
- `SERIAL` - Direkte USB-Serial Verbindung
- `ESP_NOW` - Drahtlose ESP-Now Verbindung
- `TCP` - TCP/IP Bridge

### Wichtige Defines in main.cpp
- `ENABLED_BUTTONS_COUNT` - Anzahl direkter Buttons (max 12)
- `ENABLED_ENCODERS_COUNT` - Anzahl Rotary Encoder (max 8)
- `ENABLED_BUTTONMATRIX` - Button Matrix aktiviert
- `BMATRIX_COLS/ROWS` - Matrix Dimensionen (max 8x8)
- `ADAMOTORS_SHIELDSCOUNT` - Adafruit Shields (max 3)
- `SHAKEITPWM_ENABLED_MOTORS` - PWM Motoren (max 4)
- `SHAKEITPWMFANS_ENABLED_MOTORS` - PWM Fans (max 4)
- `GAMEPAD_AXIS_01/02/03_ENABLED` - Gamepad Achsen

## Pin-Konfiguration (Beispiele ESP32)

### I2C
- SDA: GPIO 20 (Mega) / GPIO 2 (Leonardo) / GPIO 18 (328)
- SCL: GPIO 21 (Mega) / GPIO 3 (Leonardo) / GPIO 19 (328)

### OLED (SPI)
- MOSI: GPIO 35
- CLK: GPIO 36
- DC: GPIO 33
- CS: GPIO 34
- RESET: GPIO 38

### Gamepad
- Vendor ID: 0xXXXX (konfigurierbar)
- Product ID: 0xXXXX (konfigurierbar)
- Device Name: Konfigurierbar
- Manufacturer: Konfigurierbar

## Dateistruktur

```
ESP-SimHub-main/
├── .gitignore
├── platformio.ini
├── README.md
├── MEMORY.md
├── lib/
│   ├── ECrowneJoystick/      # Gamepad Library
│   ├── ESPNowSerialBridge/   # ESP-Now Bridge
│   ├── ESPNowSerialProtocol/ # ESP-Now Protokoll
│   ├── EspSimHub/            # SimHub Core
│   ├── FullLoopbackStream/   # Loopback Stream
│   └── TcpSerialBridge2/     # TCP Bridge
└── src/
    ├── main.cpp              # Haupt-Firmware
    ├── main-espnow-bridge.cpp # ESP-Now Bridge
    ├── SH*.h                 # SimHub Module
    └── AcHubCustomFonts/     # Custom Fonts
```

## Wichtige Module (SH* = SimHub)

| Modul | Funktion |
|-------|----------|
| SHButton | Einzelne Button-Steuerung |
| SHButtonMatrix | Button Matrix Handling |
| SHRotaryEncoder | Rotary Encoder Leselogik |
| SHGLCD_I2COLED | OLED Display Controller |
| SHGLCD_NOKIA | Nokia Display Controller |
| SHI2CLcd_* | I2C LCD Controller |
| SHRGBLeds* | LED Strip Controller |
| SHMAX72217* | MAX7219/MAX7221 Controller |
| SHTM1638 | TM1638 Module Controller |
| SHTM1637 | TM1637 Module Controller |
| SHShakeit* | Force Feedback Shield Controller |
| SHGamepadAxis | Gamepad Analog Axis |
| SHLedsBackpack | Adafruit LED Backpack |
| SHCommands | SimHub Command Handler |
| SHCustomProtocol | Benutzerdefinierte Protokolle |

## Build & Deployment

### PlatformIO Commands
```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor Serial
pio device monitor --baud 19200

# Clean
pio run --target clean
```

### Firmware Version
- Aktuelle Version: 'j' (J revision)
- Commit: 8aa21d4
- Branch: main

## Notes

- Alle Hardware-Einstellungen erfolgen über `#define` Makros in `main.cpp`
- Die Konfiguration ist modular - nur benötigte Module aktivieren
- Gamepad-Output nur auf ESP32 verfügbar (USB-HID)
- ESP-Now Bridge benötigt separate Konfiguration in `main-espnow-bridge.cpp`
- Shake-It Module erfordern separate Stromversorgung für Motoren
- I2C-Geschwindigkeit standardmäßig 100kHz (kann erhöht werden)

## TFT-Display (SHCustomProtocol.h)

Das Display wird vollständig über `src/SHCustomProtocol.h` gesteuert — kein SimHub-OLED-Dashboard.
SimHub sendet Telemetrie via Custom Protocol (serielle Pipe), der ESP rendert selbst.

### Wichtiger Fix: VSPI_HOST auf ESP32-S2 nicht verfügbar
`VSPI_HOST` existiert im ESP32-S2 SDK nicht → Compile-Fehler.
**Lösung:** LGFX-Klasse direkt in `SHCustomProtocol.h` eingebettet mit `SPI2_HOST`.
Die LovyanGFX-Sampledatei `lgfx_user/LGFX_ESP32_sample.hpp` **niemals einbinden** — sie enthält `VSPI_HOST`.

### Display-Layout (Portrait 240×320)

| Y | H | Inhalt |
|---|---|---|
| 0 | 18 | RPM-Balken (grün→orange→rot) |
| 19 | 100 | Gang links (gelb, Font 4 ×3) \| Speed rechts (grün, Font 6) |
| 120 | 50 | Aktuelle Rundenzeit (weiß / rot bei ungültig) |
| 171 | 50 | Beste Rundenzeit (cyan) |
| 222 | 48 | Delta (grün wenn negativ, rot wenn positiv) |
| 271 | 49 | TC \| ABS \| BB (bright wenn aktiv) |

### Bildschirmmodi
- **LOGO-Modus:** Lamborghini-Logo (gold `C_GOLD=0xFEA0` auf schwarz) — beim Start und nach 5 Min. ohne Daten
- **DASH-Modus:** vollständiges Dashboard — sobald erstes SimHub-Paket eintrifft
- Timeout-Konstante: `300000UL` ms in `loop()`

### Logo-Bitmap
- Datei: `src/logo.h` — 1bpp PROGMEM-Array, MSB-first, Zeilenauffüllung auf Bytegrenze
- Aktuell: Platzhalter (alle Null → leerer Bildschirm)
- Generieren: `python tools/convert_logo.py <bild.png>` (benötigt `pip install Pillow`)
- Zielgröße: 200×210 px (zentriert auf 240×320)

### Farben (RGB565)
| Konstante | Wert | Farbe |
|---|---|---|
| C_BG | TFT_BLACK | Hintergrund |
| C_DIV | 0x2945 | Trennlinien (dunkelgrau) |
| C_LABEL | 0x8410 | Beschriftungen (mittelgrau) |
| C_GOLD | 0xFEA0 | Logo-Gold (#FFD700) |

## Änderungsprotokoll

| Datum | Änderung | Autor |
|-------|----------|-------|
| 2026-06-24 | Projekt initialisiert | Ron |
| 2026-06-24 | MEMORY.md erstellt | Ron |
| 2026-06-24 | `.gitignore` erweitert — Python-Cache (`__pycache__/`, `*.pyc`, `*.pyo`) hinzugefügt | Ron |
| 2026-06-24 | `build_src_filter` vereinfacht — `.git/**`, `__pycache__/`, `*.pyc` entfernt (über `.gitignore` abgedeckt) | Ron |
| 2026-06-24 | PL9823 Test-LEDs: Helligkeit von 100% (255) auf 10% (25) reduziert in [src/SHRGBLedsNeoPixel.h](src/SHRGBLedsNeoPixel.h) | Ron |
| 2026-06-25 | `SHCustomProtocol.h` komplett überarbeitet — neues Layout mit RPM-Balken, Dirty-Tracking, LGFX inline mit SPI2_HOST | Claude |
| 2026-06-25 | `src/logo.h` + `tools/convert_logo.py` hinzugefügt — Lamborghini-Logo (gold/schwarz), Platzhalter, 5-Min-Timeout | Claude |
