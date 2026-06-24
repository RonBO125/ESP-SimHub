#ifndef __SHBUTTONMATRIX_H__
#define __SHBUTTONMATRIX_H__

#include <Arduino.h>
#include "SHDebouncer.h"

typedef void(*SHButtonMatrixChanged) (int, byte);

class SHButtonMatrix {

private:

	FastDigitalPin button;
	uint8_t buttonState;
	int lastPressedButton;
	unsigned long buttonLastStateChanged;

	SHButtonMatrixChanged shButtonChangedCallback;
	SHDebouncer debouncer;

	byte rowCount;
	byte colCount;
	byte * colPins;
	byte * rowPins; 
public:

	void begin(byte cols, byte rows, byte * col, byte * row, SHButtonMatrixChanged changedcallback) {

		debouncer.begin(10);
		rowCount = rows;
		colCount = cols;
		colPins = col;
		rowPins = row;

		// Set all row pins as INPUT_PULLUP (they stay this way)
		for (int x = 0; x < rowCount; x++) {
			if (rowPins[x] < 0) {
				continue;
			}
			pinMode(rowPins[x], INPUT_PULLUP);
		}

		// Set all column pins as INPUT_PULLUP initially
		for (int x = 0; x < colCount; x++) {
			if (colPins[x] < 0) {
				continue;
			}
			pinMode(colPins[x], INPUT_PULLUP);
		}

		shButtonChangedCallback = changedcallback;
	}

	void read() {
		if (debouncer.Debounce()) {
			int pressedButton = -1;
			if (millis() - buttonLastStateChanged > 50) {
				for (int colIndex = 0; colIndex < colCount; colIndex++) {

					byte curCol = colPins[colIndex];
					if (curCol < 0) {
						continue;
					}
					
					// Set current column to OUTPUT LOW
					pinMode(curCol, OUTPUT);
					digitalWrite(curCol, LOW);
					delayMicroseconds(10); // Small delay for signal to stabilize

					for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
						byte rowPin = rowPins[rowIndex];
						if (rowPin < 0) {
							continue;
						}
						
						// Read row pin (with pull-up, LOW means button pressed)
						if (digitalRead(rowPin) == LOW) {
							pressedButton = rowIndex * colCount + colIndex + 1;
							break; // Found a pressed button, stop scanning rows
						}
					}

					// Set column back to INPUT_PULLUP
					pinMode(curCol, INPUT_PULLUP);
					
					if (pressedButton != -1) {
						break; // Found a pressed button, stop scanning columns
					}
				}

				if (pressedButton != lastPressedButton) {

					if (lastPressedButton != -1) {
						shButtonChangedCallback(lastPressedButton, 0);
					}
					if (pressedButton != -1) {
						shButtonChangedCallback(pressedButton, 1);
					}
					buttonLastStateChanged = millis();
				}

				lastPressedButton = pressedButton;
			}
		}

		return;

	}
};

#endif