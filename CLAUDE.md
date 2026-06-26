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

## Häufige Fehler (nicht wiederholen)

| Fehler | Ursache | Fix |
|--------|---------|-----|
| `VSPI_HOST` not declared | LovyanGFX-Sample inkludiert | Sample nie einbinden, LGFX-Klasse inline in SHCustomProtocol.h |
| Bild verschoben (x=60, y=240) | Falscher Panel-Treiber (ILI9341 statt ST7789) | `lgfx::Panel_ST7789` verwenden |
| Logo mintgrün statt gold | `TFT_GOLD` (RGB565) als uint32_t | `0xFFD700` (RGB888) direkt angeben |
| Unteres Viertel fehlt | W/H-Konstanten stimmen nicht mit Rotation überein | W=240, H=320 bei setRotation(0)+ST7789 |
