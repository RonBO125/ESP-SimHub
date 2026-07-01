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
| Encoder 1 CLK/DT    | 12, 13                      |
| Encoder 2 CLK/DT    | 4, 5                        |
| **Shift UP Wippe**  | **14** (KEIN EC11-Button!)  |
| **Shift DOWN Wippe**| **15** (KEIN EC11-Button!)  |
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
  logo.h                — 1bpp PROGMEM-Bitmap Lamborghini-Logo (240×320)
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
- **Button-Zuordnung (physisch getestet):**
  - GPIO 14 = **Shift UP Wippe** → Windows Button 1 — KEIN EC11-Druckknopf
  - GPIO 15 = **Shift DOWN Wippe** → Windows Button 2 — KEIN EC11-Druckknopf
  - Der Kommentar im Code (`GPIO 14 - Button (SW) [Encoder 1]`) ist **falsch und irreführend**
  - `ENCODER1_BUTTON_PIN = -1` und `ENCODER2_BUTTON_PIN = -1` — EC11-Taster werden **nicht** über den Encoder-Handler gelesen
  - **EC11 SW-Pins sind in die Button-Matrix eingeschleift:**
    - EC11 Encoder 1 SW → Matrix-Button 3 → Windows Button 5
    - EC11 Encoder 2 SW → Matrix-Button 4 → Windows Button 6
  - Button-Matrix-Nummerierung: `rowIndex * colCount + colIndex + 1` (Zeile 78 in SHButtonMatrix.h)

---

## Display-Layout (SHCustomProtocol.h)

Das Display wird vollständig über `src/SHCustomProtocol.h` gesteuert — kein SimHub-OLED-Dashboard.
SimHub sendet Telemetrie via Custom Protocol (serielle Pipe), der ESP rendert selbst.

### Display-Hardware (KRITISCH — nicht ändern)

- **Controller:** ST7789 → `lgfx::Panel_ST7789` verwenden. **Niemals Panel_ILI9341** (falscher Chip → Bild versetzt)
- **panel_width=240, panel_height=320** — IC-native Dimensionen (nicht die PCB-Maße 320×240)
- **`tft.setRotation(0)`** — vom Benutzer physisch getestet und festgelegt. **Niemals ändern.**
- **cfg.invert=true, cfg.rgb_order=false** — für korrekte Farben auf diesem Modul
- LGFX-Klasse ist direkt in `SHCustomProtocol.h` eingebettet (kein separates Include). `lgfx_user/LGFX_ESP32_sample.hpp` **niemals einbinden** — enthält `VSPI_HOST` (auf ESP32-S2 nicht vorhanden → Compile-Fehler)

### Logischer Canvas

- **W=240, H=320** — Portrait, entspricht setRotation(0) auf ST7789 mit panel 240×320

### Layout (Portrait 240×320)

#### Layout 1 - Standard (Default)
| Y   | H  | Inhalt                                         |
|-----|----|------------------------------------------------|
| 0   | 18 | RPM-Balken (grün→orange→rot)                   |
| 19  | 95 | Gang links (gelb, Font 4 ×3) \| Speed rechts (grün, Font 6) |
| 115 | 30 | Aktuelle Rundenzeit (Label linksbündig + Zahl, rechtsbündig) |
| 146 | 30 | Beste Rundenzeit (Label linksbündig + Zahl, rechtsbündig) |
| 177 | 28 | Delta (Label linksbündig + Zahl, zentriert)    |
| 206 | 34 | TC \| ABS \| BB (bright wenn aktiv)            |

#### Layout 2 - Rennmodus (Performance)
| Y   | H  | Inhalt                                                         |
|-----|----|-----------------------------------------------------------------|
| 0   | 36 | RPM-Balken (grün→orange→rot, doppelte Höhe)                    |
| 37  | 80 | Gang rechts (Font 4, x=180) \| Speed links (Font 6, x=60)     |
| 118 | 50 | Aktuelle Rundenzeit (weiß / rot bei ungültig)                  |
| 169 | 50 | Beste Rundenzeit (cyan)                                        |
| 220 | 32 | Delta (Font 4)                                                 |
| 253 | 37 | TC \| ABS \| BB                                                |

#### Layout 3 - Grid (Gear-Fokus)
| Y   | H   | Inhalt                                                                      |
|-----|-----|-----------------------------------------------------------------------------|
| 0   | 18  | RPM-Balken (grün→orange→rot)                                                |
| 19  | 208 | 3-Spalten-Raster (4 Zeilen à 52px): Links FL/TC/BB/RL — Mitte Gear — Rechts FR/ABS/RR |
| 228 | 30  | Aktuelle Rundenzeit (weiß / rot bei ungültig)                               |
| 259 | 30  | Delta (grün / rot)                                                          |
| 290 | 30  | Beste Rundenzeit (cyan)                                                     |

### Layout-Skizzen

#### Layout 1 - Standard
```
Y=0   H=18    RPM-Balken (grün→orange→rot)
Y=19  H=152   Hauptbereich:
        | Links: Gang (gelb, Font 4) bei Y=43
        | Rechts: Speed (grün, Font 6) bei Y=87 + "km/h" bei Y=139
Y=172 H=32    Aktuelle Rundenzeit (weiß/rot bei ungültig)
Y=205 H=32    Delta (grün für positiv, rot für negativ)
Y=238 H=32    Beste Rundenzeit (cyan)
Y=271 H=39    Assist-Systeme (TC | ABS | BB) [Drittelbreite]
```

#### Layout 2 - Rennmodus
```
Y=0   H=36    RPM-Balken (grün→orange→rot, doppelte Höhe)
Y=37  H=80    Gang (gelb, Font 4, x=180) rechts | Speed (grün, Font 6, x=60) links
               speedY=MAIN_Y+8=45, kmhY=MAIN_Y+MAIN_H-22=95
Y=118 H=50    Aktuelle Rundenzeit (weiß/rot bei ungültig)
Y=169 H=50    Beste Rundenzeit (cyan)
Y=220 H=32    Delta (Font 4)
Y=253 H=37    Assist-Systeme TC|ABS|BB
```

#### Layout 3 - Grid
```
Y=0   H=18    RPM-Balken (grün→orange→rot)
Y=19  H=208   3-Spalten-Raster, 4 Zeilen à 52px (LAYOUT3_ROW_H):
               Spalte links (x=0–79, cx=40):  FL | TC | BB | RL
               Spalte Mitte (x=80–159, cx=120): Gear (Font 4×Size4, gearY=GRID_Y+65=84)
               Spalte rechts (x=160–239, cx=200): FR | ABS | (leer) | RR
               Zellen: Label font2 bei y0+4, Wert font3 bei y0+24
Y=228 H=30    Aktuelle Rundenzeit (weiß/rot bei ungültig)
Y=259 H=30    Delta
Y=290 H=30    Beste Rundenzeit (cyan)
```

**Hinweise:**
- Alle Y-Positionen beziehen sich auf die obere Kante des Bereichs
- "km/h" wird immer mit Font 2 angezeigt
- Trennlinien werden automatisch zwischen allen Bereichen gezeichnet

### Bildschirmmodi

- **LOGO-Modus:** Lamborghini-Logo (gold auf schwarz) — beim Start und nach 5 Min. ohne Daten
- **DASH-Modus:** vollständiges Dashboard — sobald erstes SimHub-Paket eintrifft
- Timeout-Konstante: `300000UL` ms in `loop()`

### Logo-Bitmap

- Datei: `src/logo.h` — 1bpp PROGMEM-Array, MSB-first, 30 Bytes/Zeile × 320 Zeilen = 9600 Bytes
- Generieren: `python tools/convert_logo.py tools/lamborghini.png` (Standard: 240×320)
- Benötigt: `pip install Pillow`

### Farben

| Konstante | Wert      | Farbe                                        |
|-----------|-----------|----------------------------------------------|
| C_BG      | TFT_BLACK | Hintergrund                                  |
| C_DIV     | TFT_GREY  | Trennlinien                                  |
| C_LABEL   | TFT_LIGHTGREY | Beschriftungen                           |
| C_GOLD    | 0xFFD700  | Logo-Gold — **RGB888**, nicht TFT_GOLD (RGB565 → erscheint mintgrün in drawBitmap) |

### Häufige Fehler

| Fehler | Ursache | Fix |
|--------|---------|-----|
| Bild versetzt (x≈60, y≈240) | Panel_ILI9341 statt ST7789 | `lgfx::Panel_ST7789` verwenden |
| Logo mintgrün statt gold | `TFT_GOLD` (RGB565) als uint32_t | `0xFFD700` (RGB888) direkt angeben |
| Unteres Viertel fehlt | W/H-Konstanten falsch | W=240, H=320 bei setRotation(0)+ST7789 |
| VSPI_HOST Compile-Fehler | lgfx_user/LGFX_ESP32_sample.hpp inkludiert | Niemals einbinden, LGFX inline in SHCustomProtocol.h |

---

## Layout-Umschaltung

`switchLayout()` ist **public** in `SHCustomProtocol` und wird aus `main.cpp` aufgerufen.

### Auslöser

- EC11 Encoder 1 SW → Matrix-Button 3 → `buttonMatrixStatusChanged(3, 1)` → `shCustomProtocol.switchLayout()`
- EC11 Encoder 2 SW → Matrix-Button 4 → `buttonMatrixStatusChanged(4, 1)` → `shCustomProtocol.switchLayout()`
- Beide EC11-Druckknöpfe schalten durch; der Gangschalter (GPIO 14/15) ist **nicht** beteiligt

### Kritische Implementierungsregel

`switchLayout()` **muss** intern `enterDash()` aufrufen — nicht nur `fillScreen()` + `drawChrome()`.

**Grund:** `enterDash()` setzt alle Feld-Caches zurück (`prevRpm = -1`, `strcpy(f.prev, "*")` für alle Felder). Ohne diesen Reset glauben `drawGear()`, `drawSpeed()` etc., dass sich nichts geändert hat, und zeichnen den neuen Layout-Inhalt **nicht**. Das Display zeigt dann nach dem Wechsel nur leere Chrome-Linien.

```cpp
// RICHTIG:
void switchLayout() {
    currentLayout = (currentLayout + 1) % 3;
    saveLayout();
    if (_mode == Mode::DASH) enterDash();  // ← Cache-Reset zwingend
}

// FALSCH (Felder bleiben leer):
// tft.fillScreen(C_BG);
// drawChrome();
```

### Persistenz

Layout wird via `Preferences`-API in NVS gespeichert (`namespace "display"`, key `"layout"`).
NVS-Schreibvorgänge können ~5–20 ms dauern — kein `delay()` drumherum nötig.

---

## Flash / Upload (kein Boot-Button nötig)

### Voraussetzung

SimHub muss beim Flashen **geschlossen** sein (blockiert sonst den COM-Port).

### Ablauf (automatisch via `upload_reset.py`)

1. Script erkennt ESP32 automatisch über Espressif VID `0x303A` — unabhängig von der COM-Nummer
2. Sendet 1200bps Touch → ESP32 resettet in Bootloader
3. Wartet bis COM-Port wieder verfügbar (Bootloader = gleicher Port)
4. Ruft esptool direkt auf

### Konfiguration in `platformio.ini`

```ini
upload_protocol = custom
upload_port = COM8          ; Fallback — Script erkennt Port automatisch
extra_scripts = post:upload_reset.py
```

### COM-Ports (können sich nach Neustart ändern)

| Port | Gerät                  |
|------|------------------------|
| COM8 | ESP32-S2 (TinyUSB CDC, VID 0x303A) — App + Bootloader |
| COM4 | MOZA SRP Pedals        |
| COM3 | SimHub Controller Remapper Bridge |

### esptool-Pfad

`~/.platformio/packages/tool-esptoolpy/esptool.py`
**Nicht** `python -m esptool` — Modul nicht im PlatformIO venv.

---

## Änderungsprotokoll

| Datum      | Änderung |
|------------|----------|
| 2026-06-24 | Projekt initialisiert |
| 2026-06-24 | MEMORY.md erstellt |
| 2026-06-24 | `.gitignore` erweitert — Python-Cache (`__pycache__/`, `*.pyc`, `*.pyo`) hinzugefügt |
| 2026-06-24 | `build_src_filter` vereinfacht — `.git/**`, `__pycache__/`, `*.pyc` entfernt |
| 2026-06-24 | PL9823 Test-LEDs: Helligkeit von 100% (255) auf 10% (25) reduziert |
| 2026-06-25 | `SHCustomProtocol.h` überarbeitet — neues Layout mit RPM-Balken, LGFX inline mit SPI2_HOST |
| 2026-06-25 | `src/logo.h` + `tools/convert_logo.py` — Lamborghini-Logo (gold/schwarz), 5-Min-Timeout |
| 2026-06-26 | `upload_reset.py` — automatisches Flashen via 1200bps Touch |
| 2026-06-26 | Display-Inversion (`cfg.invert = true`) und Rotation korrigiert |
| 2026-06-26 | Panel auf ST7789 umgestellt (ILI9341 war falsch → Bild versetzt) |
| 2026-06-26 | `cfg.rgb_order=false`, `setRotation(0)` (vom Benutzer bestätigt), `C_GOLD=0xFFD700` (RGB888) |
| 2026-06-29 | Layout-Umschaltfunktion hinzugefügt — 3 alternative Anordnungen (Standard, Rennmodus, Minimalistisch) |
| 2026-06-29 | Layout-Dokumentation in rule.md ergänzt mit detaillierten Tabellen für alle 3 Modi |
| 2026-06-29 | Persistente Speicherung des Layouts via Preferences-API |
| 2026-06-29 | `switchLayout()` public in SHCustomProtocol — aufgerufen aus `buttonMatrixStatusChanged` in main.cpp |
| 2026-06-29 | Layout-Umschaltung via EC11-Taster (Matrix-Button 3 oder 4, Windows Button 5/6) — nicht via GPIO14 (Shift-Wippe) |
| 2026-06-29 | Fix: `switchLayout()` ruft `enterDash()` auf (nicht nur `fillScreen`+`drawChrome`) — zwingend für Cache-Invalidierung |
| 2026-07-01 | Fix Layout 2+3: Speed-x-Position war `W*3/4` (selbe Spalte wie Gear) → jetzt `W/4` (linke Hälfte) |
| 2026-07-01 | Fix Layout 2: `speedY=RPM_Y+16=16` lag im RPM-Balken → jetzt `MAIN_Y+8=45`, `kmhY=MAIN_Y+MAIN_H-22=95` |
| 2026-07-01 | Fix: Best Lap wurde in Layout 3 bei y=2 gezeichnet (BLAP_Y=-1) → Guard `if (BLAP_Y >= 0)` |
| 2026-07-01 | Layout 3 komplett neu: 3-Spalten-Raster (FL/TC/BB/RL links, Gear Mitte, FR/ABS/RR rechts) + CurLap/Delta/BestLap unten |
| 2026-07-01 | Neue Field-Variablen `fTFL/fTFR/fTRL/fTRR` für Reifentemperaturen in Layout 3 Grid |
| 2026-07-01 | `drawGrid3()` — neue Funktion für Layout 3 (ersetzt `drawAssist()` in loop) |
| 2026-07-01 | `drawChrome()` — Early-Return für Layout 3 mit eigener Gitter-Chrome-Logik |
| 2026-07-01 | Doku + CLAUDE.md aktualisiert: Layout 3 Grid-Struktur, neue Häufige-Fehler-Einträge |