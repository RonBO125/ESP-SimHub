# ESP-SimHub GT Wheel — Claude-Regeln

## Display: ST7789 TFT (240×320 Portrait)

### Niemals ändern
- **`tft.setRotation(0)`** in `src/SHCustomProtocol.h` — vom Benutzer physisch getestet und festgelegt. Keine Rotation anpassen, auch nicht zur Fehlersuche.

### Konfiguration (korrekt und final)
```cpp
lgfx::Panel_ST7789  _panel;       // ST7789 — nicht ILI9341
cfg.panel_width  = 240;           // IC-Spalten (nicht PCB-Breite)
cfg.panel_height = 320;           // IC-Zeilen
cfg.invert       = true;
cfg.rgb_order    = false;
tft.setRotation(0);               // NICHT ÄNDERN
```

### Logischer Canvas
- W = 240, H = 320 (Portrait)
- Logik: setRotation(0) auf ST7789 mit panel 240×320 → 240×320 Canvas

### Farben
- `C_GOLD = 0xFFD700` als `uint32_t` → RGB888. Nicht `TFT_GOLD` (wäre RGB565 → erscheint als Mintgrün in drawBitmap)

### Logo
- Datei: `src/logo.h` — 1bpp PROGMEM, 30 Bytes/Zeile × 320 Zeilen = 9600 Bytes
- Generieren: `python tools/convert_logo.py tools/lamborghini.png` (Standard 240×320)
- Nach Änderung an logo.h: neu kompilieren und flashen

### Build & Flash
```bash
pio run -e esp32                        # kompilieren
pio run -e esp32 --target upload        # flashen (SimHub vorher schließen)
```

## Pin-Belegung (physisch getestet und final)

| GPIO | Funktion | Windows Gamepad Button |
|------|----------|------------------------|
| 12 | Encoder 1 CLK (A) | — |
| 13 | Encoder 1 DT (B) | — |
| 14 | **Shift UP Wippe** (KEIN EC11-Button!) | Button 1 |
| 15 | **Shift DOWN Wippe** (KEIN EC11-Button!) | Button 2 |
| 4 | Encoder 2 CLK (A) | — |
| 5 | Encoder 2 DT (B) | — |
| 17 | PL9823 LEDs Data (10 LEDs) | — |
| 33 | TFT DC | — |
| 34 | TFT CS | — |
| 35 | TFT MOSI | — |
| 36 | TFT SCK | — |
| 38 | TFT RST | — |
| 39–42 | Button-Matrix Spalten (Col1–4) | — |
| 43–45 | Button-Matrix Zeilen (Row1–3) | — |

**Button-Matrix Nummerierung** (`rowIndex * colCount + colIndex + 1`):
- Matrix-Button 3 = EC11 Encoder 1 SW → **Windows Button 5**
- Matrix-Button 4 = EC11 Encoder 2 SW → **Windows Button 6**
- Die EC11 SW-Pins sind in die Button-Matrix eingeschleift (NICHT direkt an GPIO)

**Kommentar-Warnung:** Der Kommentar im Code `GPIO 14 - Button (SW) [Encoder 1]` ist **falsch**. GPIO 14/15 sind Shift-Wippen, keine EC11-Buttons.

## Layout-Umschaltung

- `switchLayout()` ist **public** in `SHCustomProtocol` — wird aus `main.cpp` aufgerufen
- Auslöser: EC11-Button-Presse → `buttonMatrixStatusChanged` in `main.cpp` (buttonId 3 oder 4, Status=1)
- `switchLayout()` ruft intern `enterDash()` auf — das ist zwingend nötig, um alle Feld-Caches zurückzusetzen (`prevRpm`, `fGear`, `fTFL` etc.), sonst werden Felder nach dem Layout-Wechsel nicht neu gezeichnet
- **Niemals** in `switchLayout()` nur `fillScreen` + `drawChrome()` aufrufen ohne `enterDash()` — Felder bleiben dann leer
- Neue Field-Variablen (`fTFL`, `fTFR`, `fTRL`, `fTRR`) müssen in `enterDash()` mit `rst()` zurückgesetzt werden — sonst zeigt Layout 3 nach Wechsel veraltete Reifentemperaturen

## Layout 3 — Grid-Struktur (aktuell)

```
Y=  0  RPM-Balken (H=18)
Y= 19  3-Spalten-Raster (4 Zeilen à 52px, H=208):
       │  FL    │              │  FR    │  Zeile 0
       │  TC    │   Gear       │  ABS   │  Zeile 1
       │  BB    │   (groß,     │  (leer)│  Zeile 2
       │  RL    │   zentriert) │  RR    │  Zeile 3
Y=228  Cur Lap  (H=30)
Y=259  Delta    (H=30)
Y=290  Best Lap (H=30)  → Y=320
```

- Spalten: links x=0–79 (cx=40), Mitte x=80–159 (cx=120), rechts x=160–239 (cx=200)
- Gear: `gearY = LAYOUT3_GRID_Y + 65`, `gearX = W/2`, `fillRect` nur mittlere Spalte
- Zellen: Label (font 2) bei y0+4, Wert (font 3) bei y0+24, fillRect y0+20 H=28
- Kein Speed in Layout 3 (`drawSpeed()` gibt sofort zurück wenn `currentLayout == 2`)
- `drawChrome()` hat Early-Return für `currentLayout == 2` — zeichnet Gitter separat
- `drawGrid3()` übernimmt TC/ABS/BB und Reifentemperaturen (ersetzt `drawAssist()`)
- Best Lap wird jetzt angezeigt (BLAP_Y=290, nicht mehr -1)

## Häufige Fehler (nicht wiederholen)

| Fehler | Ursache | Fix |
|--------|---------|-----|
| `VSPI_HOST` not declared | LovyanGFX-Sample inkludiert | Sample nie einbinden, LGFX-Klasse inline in SHCustomProtocol.h |
| Bild verschoben (x=60, y=240) | Falscher Panel-Treiber (ILI9341 statt ST7789) | `lgfx::Panel_ST7789` verwenden |
| Logo mintgrün statt gold | `TFT_GOLD` (RGB565) als uint32_t | `0xFFD700` (RGB888) direkt angeben |
| Unteres Viertel fehlt | W/H-Konstanten stimmen nicht mit Rotation überein | W=240, H=320 bei setRotation(0)+ST7789 |
| Speed überlappt Gear (Layout 2+3) | Speed-x immer `W*3/4` → selbe Spalte wie Gear | Layout 2+3: Speed bei `x=W/4` (linke Hälfte), Gear bei `x=W*3/4` |
| Speed im RPM-Balken (Layout 2) | `speedY = RPM_Y + 16 = 16` liegt im Balken (y=0–36) | `speedY = MAIN_Y + 8`, `kmhY = MAIN_Y + MAIN_H - 22` |
| Best Lap zeichnet bei y=2 | `BLAP_Y=-1` → `drawLap` bei y=-1+3=2 (RPM-Balken) | Guard: `if (BLAP_Y >= 0) drawLap(fBLap, ...)` |
| Reifentemps nach Layout-Wechsel falsch | `fTFL/fTFR/fTRL/fTRR` nicht in `enterDash()` zurückgesetzt | Alle 4 Fields in `enterDash()` mit `rst()` aufrufen |
