#ifndef __SHCUSTOMPROTOCOL_H__
#define __SHCUSTOMPROTOCOL_H__
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Arduino.h>
#include <string.h>
#include "logo.h"

// ── LGFX display config ──────────────────────────────────────────
// Panel: ST7789, physically 320×240, mounted portrait via setRotation(0)
// → logical canvas 240×320
// Pins: MOSI=35  SCK=36  CS=34  DC=33  RST=38
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789  _panel;
    lgfx::Bus_SPI       _bus;
public:
    LGFX(void) {
        {
            auto cfg        = _bus.config();
            cfg.spi_host    = SPI2_HOST;
            cfg.spi_mode    = 0;
            cfg.freq_write  = 40000000;
            cfg.freq_read   = 16000000;
            cfg.spi_3wire   = true;
            cfg.use_lock    = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk    = 36;
            cfg.pin_mosi    = 35;
            cfg.pin_miso    = -1;
            cfg.pin_dc      = 33;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg             = _panel.config();
            cfg.pin_cs           = 34;
            cfg.pin_rst          = 38;
            cfg.pin_busy         = -1;
            cfg.panel_width      = 240;
            cfg.panel_height     = 320;
            cfg.readable         = false;
            cfg.invert           = true;
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;
            _panel.config(cfg);
        }
        setPanel(&_panel);
    }
};

static LGFX tft;

// ── Logical canvas after setRotation(0) on ST7789: 240×320 ──────
static const int W = 240;
static const int H = 320;

// ── Section layout (portrait 240×320) ───────────────────────────
//   Y    H    content
//   0   18   RPM bar
//  19  100   Gear (left W/2) | Speed (right W/2)
// 120   50   Current lap time
// 171   50   Best lap time
// 222   48   Delta
// 271   49   TC | ABS | BB
static const int RPM_Y  = 0,   RPM_H  = 18;
static const int MAIN_Y = 19,  MAIN_H = 100;
static const int CLAP_Y = 120, CLAP_H = 50;
static const int BLAP_Y = 171, BLAP_H = 50;
static const int DELT_Y = 222, DELT_H = 48;
static const int ASST_Y = 271, ASST_H = 49;

// ── Color palette ────────────────────────────────────────────────
static const uint32_t C_BG    = TFT_BLACK;
static const uint32_t C_DIV   = TFT_GREY;
static const uint32_t C_LABEL = TFT_LIGHTGREY;
static const uint32_t C_GOLD  = 0xFFD700;  // RGB888

// ── Per-field dirty tracking (char-level für Ziffern-Updates) ────
struct Field { char prev[32]; int32_t prevColor; };

static Field fGear  = {"*", 0};
static Field fSpd   = {"*", 0};
static Field fCLap  = {"*", 0};
static Field fBLap  = {"*", 0};
static Field fDelt  = {"*", 0};
static Field fTC    = {"*", 0};
static Field fABS   = {"*", 0};
static Field fBB    = {"*", 0};

static bool fieldChanged(Field& f, const String& v, int32_t c) {
    if (strcmp(f.prev, v.c_str()) == 0 && f.prevColor == c) return false;
    strncpy(f.prev, v.c_str(), 31);
    f.prev[31] = '\0';
    f.prevColor = c;
    return true;
}

// ── Bounding-Box der geänderten Zeichen ──────────────────────────
struct CharBox { int x, y, w, h; };

// Finde die bounding box aller geänderten Zeichen zwischen oldStr und newStr.
// baseX ist die Referenz-x-Position, baselineY ist die untere Kante des Textes.
// fontSize = TextSize (4 oder 6).
static CharBox findChangedBox(const String& oldStr, const String& newStr,
                              int baseX, int baselineY, int fontSize,
                              bool rightAligned = false, bool centerAligned = false)
{
    CharBox out = {0, 0, 0, 0};
    int oldLen = oldStr.length();
    int newLen = newStr.length();
    int minLen = min(oldLen, newLen);

    // Erste und letzte geänderte Position finden
    int firstDiff = -1, lastDiff = -1;
    for (int i = 0; i < minLen; i++) {
        if (oldStr[i] != newStr[i]) {
            if (firstDiff < 0) firstDiff = i;
            lastDiff = i;
        }
    }
    if (firstDiff < 0 && oldLen != newLen) {
        // Länge hat sich geändert, aber Inhalt bis minLen gleich
        firstDiff = minLen;
        lastDiff = max(oldLen, newLen) - 1;
    }

    if (firstDiff < 0) return out; // Keine Änderung

    // Breite pro Zeichen in Pixeln (Font-abhängig)
    int charW;
    if (fontSize <= 2) charW = 4;
    else if (fontSize == 3 || fontSize == 4) charW = 7;
    else if (fontSize == 5 || fontSize == 6) charW = 10;
    else charW = 6;

    // Berechne x-Position jedes Zeichens
    int totalW = newLen * charW;
    int startX;
    if (rightAligned)
        startX = baseX - totalW; // baseX = rechte Kante
    else if (centerAligned)
        startX = baseX - totalW / 2; // baseX = Mitte
    else
        startX = baseX;

    // Bounding-box der geänderten Zeichen
    int changedStartX = startX + firstDiff * charW;
    int changedLen = lastDiff - firstDiff + 1;
    out.x = changedStartX;
    out.w = max(1, changedLen * charW);
    out.y = baselineY - (fontSize <= 2 ? 8 : fontSize <= 4 ? 12 : 16);
    out.h = (fontSize <= 2 ? 8 : fontSize <= 4 ? 12 : 16);
    return out;
}


class SHCustomProtocol {
private:
    // ── Telemetry data ───────────────────────────────────────────
    int    rpmPct    = 0;
    int    prevRpm   = -1;
    int    rpmRL     = 90;
    String gear      = "N";
    int    speed     = 0;
    String curLap    = "--:--.--";
    String bestLap   = "--:--.--";
    String delta     = "---.---";
    String tcLvl     = "0";
    String tcAct     = "0";
    String absLvl    = "0";
    String absAct    = "0";
    String tcCutNull = "True";
    String tcCut     = "0  0";
    String bb        = "0";
    String lapInv    = "False";

    // ── Screen state ─────────────────────────────────────────────
    enum class Mode : uint8_t { LOGO, DASH };
    Mode          _mode       = Mode::LOGO;
    unsigned long _lastDataMs = 0;

    // ── Logo screen ──────────────────────────────────────────────
    void drawLogo() {
        tft.fillScreen(C_BG);
        tft.drawBitmap(0, 0, LOGO_BITMAP, LOGO_W, LOGO_H, C_GOLD, C_BG);
    }

    // ── Dashboard chrome (drawn once on entering dash mode) ──────
    void drawChrome() {
        tft.drawFastHLine(0, RPM_Y + RPM_H, W, C_DIV);
        tft.drawFastHLine(0, CLAP_Y - 1,    W, C_DIV);
        tft.drawFastHLine(0, BLAP_Y - 1,    W, C_DIV);
        tft.drawFastHLine(0, DELT_Y - 1,    W, C_DIV);
        tft.drawFastHLine(0, ASST_Y - 1,    W, C_DIV);

        tft.drawFastVLine(W / 2,     MAIN_Y, MAIN_H, C_DIV);
        tft.drawFastVLine(W / 3,     ASST_Y, ASST_H, C_DIV);
        tft.drawFastVLine(W * 2 / 3, ASST_Y, ASST_H, C_DIV);

        tft.setTextColor(C_LABEL, C_BG);
        tft.drawString     ("CUR LAP",  4,       CLAP_Y + 3, 2);
        tft.drawString     ("BEST LAP", 4,       BLAP_Y + 3, 2);
        tft.drawCentreString("DELTA",   W / 2,   DELT_Y + 3, 2);
        tft.drawCentreString("TC",      W / 6,   ASST_Y + 5, 2);
        tft.drawCentreString("ABS",     W / 2,   ASST_Y + 5, 2);
        tft.drawCentreString("BB",      W * 5/6, ASST_Y + 5, 2);
    }

    // ── Mode transitions ─────────────────────────────────────────
    void enterDash() {
        _mode = Mode::DASH;
        tft.fillScreen(C_BG);
        drawChrome();
        prevRpm = -1;
        auto rst = [](Field& f) { strcpy(f.prev, "*"); f.prevColor = 0; };
        rst(fGear); rst(fSpd); rst(fCLap); rst(fBLap);
        rst(fDelt); rst(fTC);  rst(fABS);  rst(fBB);
    }

    // ── RPM bar ──────────────────────────────────────────────────
    void drawRpm() {
        if (rpmPct == prevRpm) return;
        int bw  = max(0, min(W, (W * rpmPct) / 100));
        int bh  = RPM_H - 4;
        uint32_t col = (rpmPct >= rpmRL)     ? TFT_RED    :
                       (rpmPct >= rpmRL - 5) ? TFT_ORANGE : 0x07E0;
        if (prevRpm > rpmPct)
            tft.fillRect(bw, 2, W - bw, bh, C_BG);
        if (bw > 0)
            tft.fillRect(0, 2, bw, bh, col);
        if (prevRpm < 0)
            tft.drawRect(0, 0, W, RPM_H, C_DIV);
        prevRpm = rpmPct;
    }

    // ── Gear ─────────────────────────────────────────────────────
    void drawGear() {
        if (!fieldChanged(fGear, gear, TFT_YELLOW)) return;
        tft.fillRect(1, MAIN_Y + 1, W / 2 - 2, MAIN_H - 2, C_BG);
        tft.setTextSize(3);
        tft.setTextColor(TFT_YELLOW, C_BG);
        tft.drawCentreString(gear, W / 4, MAIN_Y + 12, 4);
        tft.setTextSize(1);
        tft.drawFastVLine(W / 2, MAIN_Y, MAIN_H, C_DIV);
    }

    // ── Speed ────────────────────────────────────────────────────
    void drawSpeed() {
        String s = String(speed);

        // Erst auf Änderung prüfen (ohne fSpd.prev zu modifizieren)
        if (strcmp(fSpd.prev, s.c_str()) == 0 && fSpd.prevColor == TFT_GREEN) return;

        // Bounding-box der geänderten Zeichen MITTELS des ALTEN fSpd.prev finden
        CharBox box = findChangedBox(fSpd.prev, s, W * 3 / 4, MAIN_Y + 16 + 16, 6, false, true);

        // Nur geänderten Bereich löschen
        if (box.w > 0 && box.h > 0) {
            tft.fillRect(box.x, box.y, box.w, box.h, C_BG);
        }

        // Kompletten Text neu zeichnen
        tft.setTextColor(TFT_GREEN, C_BG);
        tft.drawCentreString(s, W * 3 / 4, MAIN_Y + 16, 6);
        tft.setTextColor(C_LABEL, C_BG);
        tft.drawCentreString("km/h", W * 3 / 4, MAIN_Y + MAIN_H - 18, 2);

        // fSpd.prev auf neuen Wert setzen
        strncpy(fSpd.prev, s.c_str(), 31);
        fSpd.prev[31] = '\0';
        fSpd.prevColor = TFT_GREEN;
    }

    // ── Lap time (partial update) ────────────────────────────────
    void drawLap(Field& f, const String& val, int y, int h, int32_t color) {
        // Erst auf Änderung prüfen (ohne f.prev zu modifizieren)
        if (strcmp(f.prev, val.c_str()) == 0 && f.prevColor == color) return;

        // Bounding-box der geänderten Zeichen MITTELS des ALTEN f.prev finden
        CharBox box = findChangedBox(f.prev, val, W - 6, y + 32, 4, true, false);

        // Nur geänderten Bereich löschen
        if (box.w > 0 && box.h > 0) {
            tft.fillRect(box.x, box.y, box.w, box.h, C_BG);
        }

        // Kompletten Text neu zeichnen
        tft.setTextColor(color, C_BG);
        tft.drawRightString(val, W - 6, y + 22, 4);

        // f.prev auf neuen Wert setzen
        strncpy(f.prev, val.c_str(), 31);
        f.prev[31] = '\0';
        f.prevColor = color;
    }

    // ── Delta (partial update) ───────────────────────────────────
    void drawDelta() {
        int32_t col = (delta.indexOf('-') >= 0) ? TFT_GREEN : TFT_RED;

        // Erst auf Änderung prüfen (ohne fDelt.prev zu modifizieren)
        if (strcmp(fDelt.prev, delta.c_str()) == 0 && fDelt.prevColor == col) return;

        // Bounding-box der geänderten Zeichen MITTELS des ALTEN fDelt.prev finden
        CharBox box = findChangedBox(fDelt.prev, delta, W / 2, DELT_Y + 32, 4, false, true);

        // Nur geänderten Bereich löschen
        if (box.w > 0 && box.h > 0) {
            tft.fillRect(box.x, box.y, box.w, box.h, C_BG);
        }

        // Kompletten Text neu zeichnen
        tft.setTextColor(col, C_BG);
        tft.drawCentreString(delta, W / 2, DELT_Y + 21, 4);

        // fDelt.prev auf neuen Wert setzen
        strncpy(fDelt.prev, delta.c_str(), 31);
        fDelt.prev[31] = '\0';
        fDelt.prevColor = col;
    }

    // ── TC / ABS / BB ────────────────────────────────────────────
    void drawAssist(Field& f, const String& val, int32_t col, int column) {
        if (!fieldChanged(f, val, col)) return;
        int x0 = (column - 1) * (W / 3) + 1;
        int cx = (column - 1) * (W / 3) + W / 6;
        // Only clear the text area, not the entire background
        tft.fillRect(x0, ASST_Y + 22, W / 3 - 2, ASST_H - 23, C_BG);
        tft.setTextColor(col, C_BG);
        tft.drawCentreString(val, cx, ASST_Y + 22, 4);
    }

public:
    void setup() {
        tft.init();
        tft.setRotation(0);
        drawLogo();
    }

    void read() {
        speed      = FlowSerialReadStringUntil(';').toInt();
        gear       = FlowSerialReadStringUntil(';');
        rpmPct     = FlowSerialReadStringUntil(';').toInt();
        rpmRL      = FlowSerialReadStringUntil(';').toInt();
        curLap     = FlowSerialReadStringUntil(';');
        FlowSerialReadStringUntil(';'); // lastLap (unused)
        bestLap    = FlowSerialReadStringUntil(';');
        delta      = FlowSerialReadStringUntil(';');
        FlowSerialReadStringUntil(';'); // deltaProgress
        FlowSerialReadStringUntil(';'); // tyrePressFL
        FlowSerialReadStringUntil(';'); // tyrePressFF
        FlowSerialReadStringUntil(';'); // tyrePressRL
        FlowSerialReadStringUntil(';'); // tyrePressRR
        tcLvl      = FlowSerialReadStringUntil(';');
        tcAct      = FlowSerialReadStringUntil(';');
        absLvl     = FlowSerialReadStringUntil(';');
        absAct     = FlowSerialReadStringUntil(';');
        tcCutNull  = FlowSerialReadStringUntil(';');
        tcCut      = FlowSerialReadStringUntil(';');
        bb         = FlowSerialReadStringUntil(';');
        FlowSerialReadStringUntil(';'); // brake (unused)
        lapInv     = FlowSerialReadStringUntil(';');
        FlowSerialReadStringUntil('\n');

        _lastDataMs = millis();
        if (_mode != Mode::DASH) enterDash();
    }

    void loop() {
        if (_mode == Mode::DASH && (millis() - _lastDataMs > 300000UL)) {
            _mode = Mode::LOGO;
            drawLogo();
            return;
        }
        if (_mode != Mode::DASH) return;

        drawRpm();
        drawGear();
        drawSpeed();

        int32_t lapCol = (lapInv == "True") ? TFT_RED : TFT_WHITE;
        drawLap(fCLap, curLap,  CLAP_Y, CLAP_H, lapCol);
        drawLap(fBLap, bestLap, BLAP_Y, BLAP_H, TFT_CYAN);

        drawDelta();

        String  tcStr  = (tcCutNull == "False") ? tcCut : tcLvl;
        int32_t tcCol  = (tcAct  == "1") ? TFT_YELLOW : 0xC600;
        int32_t absCol = (absAct == "1") ? 0x001F     : 0x0018;

        drawAssist(fTC,  tcStr,  tcCol,       1);
        drawAssist(fABS, absLvl, absCol,      2);
        drawAssist(fBB,  bb,     TFT_MAGENTA, 3);
    }

    void idle() {}
};

#endif
