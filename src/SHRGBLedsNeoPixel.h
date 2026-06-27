#ifndef __SHRGBLEDSNEOPIXEL_H__
#define __SHRGBLEDSNEOPIXEL_H__

#include <Arduino.h>
#include "SHRGBLedsBase.h"
#include <Adafruit_NeoPixel.h>

class SHRGBLedsNeoPixel : public SHRGBLedsBase {
private:
	unsigned long lastRead = 0;
	unsigned long _testStartMs = 0;
	bool _testModeActive = false;
	unsigned long _testTimeoutMs = 10000UL; // 10 Sekunden

protected:
	Adafruit_NeoPixel * NeoPixel_strip;
public:

	void begin(Adafruit_NeoPixel * strip,int maxLeds, int righttoleft, bool testMode) {
		SHRGBLedsBase::begin(maxLeds, righttoleft);
		NeoPixel_strip = strip;
		NeoPixel_strip->begin();
		NeoPixel_strip->show();

		if (testMode > 0) {
			for (int i = 0; i < maxLeds; i++) {
				NeoPixel_strip->setPixelColor(i, 25, 0, 0);  // 25% Helligkeit (0.1 * 255 = 25)
				NeoPixel_strip->show();
			}
			_testStartMs = millis();
			_testModeActive = true;
		}
	}

	void show() {
		// Test-Modus nach Timeout oder bei SimHub-Daten deaktivieren
		if (_testModeActive) {
			if (millis() - _testStartMs >= _testTimeoutMs || lastRead > 0) {
				for (int i = 0; i < _maxLeds; i++) {
					NeoPixel_strip->setPixelColor(i, 0, 0, 0);
				}
				NeoPixel_strip->show();
				_testModeActive = false;
			}
		}
		NeoPixel_strip->show();
	}

protected:
	void setPixelColor(uint8_t lednumber, uint8_t r, uint8_t g, uint8_t b) {
		NeoPixel_strip->setPixelColor(lednumber, r, g, b);
	}
	
public:
	void onDataReceived() override {
		lastRead = millis();
	}
	
	void checkTestTimeout() {
		if (_testModeActive) {
			if (millis() - _testStartMs >= _testTimeoutMs) {
				for (int i = 0; i < _maxLeds; i++) {
					NeoPixel_strip->setPixelColor(i, 0, 0, 0);
				}
				NeoPixel_strip->show();
				_testModeActive = false;
			}
		}
	}
};

#endif
