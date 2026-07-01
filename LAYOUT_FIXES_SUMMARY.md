# Layout-Höhenkorrekturen - Zusammenfassung

## Problem
Die Höhen der einzelnen Werte passten nicht korrekt. Beispielsweise wurden in Layout 2 die Geschwindigkeit und das Label auf der Drehzahlanzeige angezeigt, statt an den richtigen Positionen.

## Ursache
Die Konstanten für die Layout-Positionen waren falsch benannt und referenziert:
- `LAYOUT2_SPEED_RPM_Y` und `LAYOUT2_SPEED_RPM_H` wurden verwendet, obwohl sie nicht definiert waren
- Die tatsächlichen Konstanten hießen `LAYOUT2_RPM_Y` und `LAYOUT2_RPM_H`
- Die drawGear() und drawSpeed() Funktionen verwendeten falsche Y-Positionen

## Lösungen

### 1. Konstantennamen korrigiert (Zeile ~90)
```cpp
// BEFORE:
static const int LAYOUT2_SPEED_RPM_Y = 0,   LAYOUT2_SPEED_RPM_H = 36;
static const int LAYOUT2_GEAR_Y     = 37,  LAYOUT2_GEAR_H     = 80;

// AFTER:
static const int LAYOUT2_RPM_Y      = 0,   LAYOUT2_RPM_H      = 36;
static const int LAYOUT2_GEAR_SPEED_Y = 37, LAYOUT2_GEAR_SPEED_H = 80;
```

### 2. #define-Makros aktualisiert (Zeile ~115)
Alle Referenzen auf die alten Konstantennamen wurden korrigiert:
- `LAYOUT2_SPEED_RPM_Y` → `LAYOUT2_RPM_Y`
- `LAYOUT2_SPEED_RPM_H` → `LAYOUT2_RPM_H`
- `LAYOUT2_GEAR_Y` → `LAYOUT2_GEAR_SPEED_Y`
- `LAYOUT2_GEAR_H` → `LAYOUT2_GEAR_SPEED_H`

### 3. drawGear() Funktion überarbeitet (Zeile ~265)
- Layout-spezifische Positionierung korrigiert
- Hintergrundlöschung optimiert
- Vertikale Trennlinie nur in Layout 1 und 3 gezeichnet

### 4. drawSpeed() Funktion überarbeitet (Zeile ~352)
- Layout-spezifische Y-Positionen nach Dokumentation:
  - **Layout 1**: speedY = MAIN_Y + 42, kmhY = MAIN_Y + 94
  - **Layout 2**: speedY = RPM_Y + 16, kmhY = RPM_Y + RPM_H - 18
  - **Layout 3**: speedY = MAIN_Y + 16, kmhY = MAIN_Y + MAIN_H - 18

## Ergebnisse nach der Korrektur

### Layout 2 (Rennmodus) - Jetzt korrekt:
- **RPM-Balken**: Y=0, H=36 (doppelte Höhe)
- **Gang**: Y=72 (Mitte des Gang-Bereichs), rechts
- **Speed**: Y=16 (unten im Speed/RPM-Bereich), links
- **km/h Label**: Y=18 (unten im Speed/RPM-Bereich)
- **Aktuelle Rundenzeit**: Y=118, H=50
- **Beste Rundenzeit**: Y=169, H=50
- **Delta**: Y=220, H=32
- **Assist-Systeme**: Y=253, H=37 (reduzierte Größe)

### Layout 1 und 3:
- Bleiben unverändert und funktionieren weiterhin korrekt

## Compilation-Ergebnis
✅ Erfolgreich kompiliert mit PlatformIO
- RAM: 18.0% (59040 bytes von 327680 bytes)
- Flash: 80.1% (1050174 bytes von 1310720 bytes)

## Testempfehlung
Die Layout-Umschaltung kann durch Drücken der EC11-Taster (Matrix-Button 3 oder 4) getestet werden, um sicherzustellen, dass alle drei Layouts korrekt angezeigt werden.