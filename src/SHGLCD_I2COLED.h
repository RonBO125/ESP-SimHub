#ifndef __SHGLCD_I2COLED_H__
#define __SHGLCD_I2COLED_H__

#include <Arduino.h>
#include "Adafruit_GFX.h"
#include <Adafruit_ST7789.h>
#include "SHGLCD_base.h"


// ST7789 TFT Display resolution
#define SCREEN_WIDTH 240 // TFT width
#define SCREEN_HEIGHT 320 // TFT height

// SPI TFT Display (pin definitions from main.cpp)
#ifndef OLED_MOSI
#define OLED_MOSI 35
#endif
#ifndef OLED_CLK
#define OLED_CLK 36
#endif
#ifndef OLED_DC
#define OLED_DC 33
#endif
#ifndef OLED_CS
#define OLED_CS 34
#endif
#ifndef OLED_RESET
#define OLED_RESET 38
#endif

// SPI Constructor for ST7789: (cs, dc, mosi, sclk, rst)
Adafruit_ST7789 glcd1 = Adafruit_ST7789(OLED_CS, OLED_DC, OLED_MOSI, OLED_CLK, OLED_RESET);
Adafruit_ST7789 * oled[] = { &glcd1 };

class SHGLCD_I2COLED : public SHGLCD_Base
{
public:

	void Init() {
		// Initialize ST7789VW (240x320)
		glcd1.init(SCREEN_WIDTH, SCREEN_HEIGHT);
		glcd1.setRotation(1); // Landscape mode
		glcd1.fillScreen(ST77XX_BLACK);
		glcd1.setTextSize(3);
		glcd1.setTextColor(ST77XX_WHITE);
		glcd1.setCursor(50, 140);
		glcd1.print("SimHub");
	}

	void Display(int idx) {
		// ST7735 draws directly, no buffer to flush
	}

	void ClearDisplay(int idx) {
		oled[idx]->fillScreen(ST77XX_BLACK);
	}

	void SetContrast(int idx, int c) {
		//nokiaLCDs[idx]->setContrast(c);
	}

	int GetScreenCount() {
		return 1;
	}

	Adafruit_GFX * GetScreen(int idx) {
		return oled[idx];
	}

};
#endif