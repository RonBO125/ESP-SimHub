# Projektregeln — ESP-SimHub GT Wheel

## Projektübersicht

DIY-Simracing-Lenkrad-Controller auf Basis ESP32-S2-SAOLA-1.
Firmware läuft unter dem Arduino-Framework (PlatformIO), kommuniziert per USB-Serial mit SimHub auf dem PC.
Das Gerät meldet sich als USB-HID-Gamepad an (TinyUSB, `ARDUINO_USB_MODE=0`).

---

## Build-Umgebung

- **Aktive Umgebung:** `env:esp32` in [platformio.ini](../platformio.ini)
- **Board:** ESP32-S2-SAOLA-1
- **Upload-Port:** COM14
- **Build-Typ:** release (`-O2`, kein Debug)
- **Framework:** Arduino via pioarduino platform v51.03.07
- **Baud:** 19200 (SimHub-Protokoll), 115200 (Monitor)

Firmware nur für `env:esp32` bauen und flashen. Die Umgebungen `esp8266` und `espnow-bridge` sind Alternativen und aktuell nicht aktiv.

---

## Pin-Belegung (ESP32-S2)

| Funktion          | GPIO(s)                        |
|-------------------|-------------------------------|
| TFT MOSI (SDA)    | 35                            |
| TFT SCK (SCL)     | 36                            |
| TFT CS            | 34                            |
| TFT DC            | 33                            |
| TFT RST           | 38                            |
| Button-Matrix Col | 39, 40, 41, 42                |
| Button-Matrix Row | 43, 44, 45                    |
| Encoder 1 CLK/DT/SW | 12, 13, 14                  |
| Encoder 2 CLK/DT/SW | 4, 5, 15                    |
| RGB-LEDs (PL9823) | 17                            |
| Frei              | 0, 2                          |

**Hinweis:** GPIO 34–39 sind reine Input-Pins (kein interner Pull-up möglich).

---

## Speicher-Regeln (kritisch)

Der ESP32-S2 hat ca. 300 KB nutzbaren Heap. Heap-Fragmentierung durch häufige
kleine Allokationen führt zu nicht reproduzierbaren Neustarts.

### Verboten in Hot-Path-Code (loop, idle, drawCell, read):

- `std::map`, `std::vector`, `std::string` oder andere STL-Container mit
  dynamischem Speicher
- Arduino `String`-Objekte **per Value** als Funktionsparameter
- Arduino `String`-Objekte als Mitgliedsvariablen, wenn der Wert ganzzahlig ist
- Implizite `int` → `String` → `int`-Konvertierungsketten

### Vorgaben:

- Funktionsparameter vom Typ `String` immer als `const String&` übergeben
- Integer-Telemetriewerte (Geschwindigkeit, RPM etc.) als `int` speichern,
  erst bei der Anzeige in `String(wert)` konvertieren
- Zustandsspeicherung für Display-Zellen über das statische `g_cells[]`-Array
  in [SHCustomProtocol.h](../src/SHCustomProtocol.h) — kein Heap
- Neue Display-Zellen in `g_cells[]` eintragen (max. `char[24]` pro Wert)
- String-Vergleiche auf `const char*` mit `strcmp()`, nicht mit `==`

---

## Datei-Struktur

```
src/
  main.cpp              — Einstiegspunkt, Hardware-Init, setup(), loop()
  SHCustomProtocol.h    — TFT-Darstellung, SimHub-Datenempfang (read/loop/idle)
  SHButtonMatrix.h      — 4×3-Tastenmatrix-Scan
  SHRotaryEncoder.h     — EC11-Drehgeber-Zustandsmaschine
  SHButton.h            — Einzeltaster-Handler
  SHCommands.h          — SimHub-Befehlsverarbeitung
  lgfx_user/            — LovyanGFX Display-Konfiguration (ST7735)
lib/
  ECrowneJoystick/      — USB-HID-Gamepad-Implementierung
  EspSimHub/            — SimHub-Kernbibliothek
```

---

## SimHub-Protokoll

Datenpaket kommt als semikolon-getrennter String, abgeschlossen mit `\n`.
Reihenfolge der Felder (in `SHCustomProtocol::read()`):

```
speed ; gear ; rpmPercent ; rpmRedLineSetting ;
currentLapTime ; lastLapTime ; bestLapTime ;
sessionBestLiveDeltaSeconds ; sessionBestLiveDeltaProgressSeconds ;
tyrePressureFL ; tyrePressureFR ; tyrePressureRL ; tyrePressureRR ;
tcLevel ; tcActive ; absLevel ; absActive ;
isTCCutNull ; tcTcCut ; brakeBias ; brake ; lapInvalidated \n
```

**Regel:** `read()` muss **alle** Felder lesen — auch ungenutzte — damit der
Puffer vollständig geleert wird. Kein `delay()` in `read()`, `loop()` oder `idle()`.

---

## Feature-Flags

Aktiv gesetzte Defines in [main.cpp](../src/main.cpp):

| Define                  | Funktion                          |
|-------------------------|-----------------------------------|
| `INCLUDE_BUTTONS`       | 2 direkte Taster (Encoder-SW)     |
| `INCLUDE_BUTTONMATRIX`  | 4×3 Button-Matrix                 |
| `INCLUDE_ENCODERS`      | 2× EC11 Drehgeber                 |
| `INCLUDE_PL9823`        | 10× PL9823 RGB-LEDs               |
| `INCLUDE_GAMEPAD`       | USB-HID-Gamepad-Modus             |
| `CONNECTION_TYPE SERIAL`| USB-Serial zu SimHub              |

Neue Hardware-Features nur über die vorhandenen `#define INCLUDE_*`-Mechanismen
aktivieren. Keine direkte Initialisierung außerhalb von `setup()`.

---

## Code-Stil

- Kommentare auf Deutsch oder Englisch — kein Mischen innerhalb einer Funktion
- Kein `delay()` im Hauptprogramm (blockiert Watchdog-Fütterung und Serial)
- Watchdog wird implizit durch den Arduino-Idle-Task gefüttert — keine
  manuelle `esp_task_wdt_reset()`-Aufrufe nötig, solange `loop()` nicht blockiert
- Neue Pins immer in der Pin-Übersicht am Anfang von `main.cpp` dokumentieren
- `build_type = release` bleibt erhalten — kein Wechsel auf `debug` im Commit

---

## Bekannte Probleme / Hintergrund

- **Neustarts im Betrieb:** Ursache war Heap-Fragmentierung durch `std::map<String>`.
  Behoben durch statisches `g_cells[]`-Array + `const String&`-Parameter.
  Bei erneuten Neustarts zuerst `ESP.getFreeHeap()` in `loop()` loggen.
- **USB nicht verfügbar nach Neustart:** TinyUSB-Re-Enumerierung kann fehlschlagen,
  wenn der PC den USB-Port nicht freigibt. USB-Kabel trennen/stecken als Workaround.
- **Encoder-Button-Pins:** `ENCODER1_BUTTON_PIN -1` und `ENCODER2_BUTTON_PIN -1` —
  die Encoder-Taster sind als separate Buttons 1 & 2 (`BUTTON_PIN_1 = 14`,
  `BUTTON_PIN_2 = 15`) konfiguriert, nicht direkt über den Encoder-Handler.
