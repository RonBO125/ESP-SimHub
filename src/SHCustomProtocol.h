#ifndef __SHCUSTOMPROTOCOL_H__
#define __SHCUSTOMPROTOCOL_H__
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Arduino.h>
#include <string.h>
#include "logo.h"

// ── LGFX display config — ESP32-S2, SPI TFT 240×320 ────────────
// Pins: MOSI=35  SCK=36  CS=34  DC=33  RST=38
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel;
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

// ── Display size (portrait 240×320) ─────────────────────────────
static const int W = 240;
static const int H = 320;

// ── Section layout ───────────────────────────────────────────────
//  Y    H    content
//   0   18   RPM bar
//  19  100   Gear (left half) | Speed (right half)
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
static const uint32_t C_DIV   = 0x2945;  // dark gray — dividers
static const uint32_t C_LABEL = 0x8410;  // mid gray  — labels
static const uint32_t C_GOLD  = 0xFEA0;  // #FFD700   — logo

// ── Per-field dirty tracking ─────────────────────────────────────
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
        int x = (W - LOGO_W) / 2;
        int y = (H - LOGO_H) / 2;
        tft.drawBitmap(x, y, LOGO_BITMAP, LOGO_W, LOGO_H, C_GOLD, C_BG);
    }

    // ── Dashboard chrome — drawn once when entering dash mode ────
    void drawChrome() {
        // Horizontal dividers
        tft.drawFastHLine(0, RPM_Y + RPM_H, W, C_DIV);
        tft.drawFastHLine(0, CLAP_Y - 1,    W, C_DIV);
        tft.drawFastHLine(0, BLAP_Y - 1,    W, C_DIV);
        tft.drawFastHLine(0, DELT_Y - 1,    W, C_DIV);
        tft.drawFastHLine(0, ASST_Y - 1,    W, C_DIV);

        // Vertical dividers
        tft.drawFastVLine(W / 2,     MAIN_Y, MAIN_H, C_DIV);
        tft.drawFastVLine(W / 3,     ASST_Y, ASST_H, C_DIV);
        tft.drawFastVLine(W * 2 / 3, ASST_Y, ASST_H, C_DIV);

        // Section labels
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
        uint32_t col = (rpmPct >= rpmRL)      ? TFT_RED    :
                       (rpmPct >= rpmRL - 5)  ? TFT_ORANGE : 0x07E0;
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
        if (!fieldChanged(fSpd, s, TFT_GREEN)) return;
        tft.fillRect(W / 2 + 1, MAIN_Y + 1, W / 2 - 2, MAIN_H - 22, C_BG);
        tft.setTextColor(TFT_GREEN, C_BG);
        tft.drawCentreString(s, W * 3 / 4, MAIN_Y + 16, 6);
        tft.setTextColor(C_LABEL, C_BG);
        tft.drawCentreString("km/h", W * 3 / 4, MAIN_Y + MAIN_H - 18, 2);
    }

    // ── Lap time ─────────────────────────────────────────────────
    void drawLap(Field& f, const String& val, int y, int h, int32_t color) {
        if (!fieldChanged(f, val, color)) return;
        tft.fillRect(0, y + 21, W, h - 22, C_BG);
        tft.setTextColor(color, C_BG);
        tft.drawRightString(val, W - 6, y + 22, 4);
    }

    // ── Delta ────────────────────────────────────────────────────
    void drawDelta() {
        int32_t col = (delta.indexOf('-') >= 0) ? TFT_GREEN : TFT_RED;
        if (!fieldChanged(fDelt, delta, col)) return;
        tft.fillRect(0, DELT_Y + 21, W, DELT_H - 22, C_BG);
        tft.setTextColor(col, C_BG);
        tft.drawCentreString(delta, W / 2, DELT_Y + 21, 4);
    }

    // ── TC / ABS / BB ────────────────────────────────────────────
    void drawAssist(Field& f, const String& val, int32_t col, int column) {
        if (!fieldChanged(f, val, col)) return;
        int x0 = (column - 1) * (W / 3) + 1;
        int cx = (column - 1) * (W / 3) + W / 6;
        tft.fillRect(x0, ASST_Y + 22, W / 3 - 2, ASST_H - 23, C_BG);
        tft.setTextColor(col, C_BG);
        tft.drawCentreString(val, cx, ASST_Y + 22, 4);
    }

public:
    void setup() {
        tft.init();
        tft.setRotation(3);
        drawLogo(); // show logo before SimHub connects
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
        if (_mode != Mode::DASH) enterDash(); // switch from logo to dashboard
    }

    void loop() {
        // After 5 minutes without data → back to logo
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
