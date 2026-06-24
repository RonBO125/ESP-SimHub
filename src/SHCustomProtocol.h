#ifndef __SHCUSTOMPROTOCOL_H__
#define __SHCUSTOMPROTOCOL_H__
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// #include <LGFX_AUTODETECT.hpp>  // クラス"LGFX"を準備します
#include <lgfx_user/LGFX_ESP32_sample.hpp>
#include <Arduino.h>
// #include <TFT_eSPI.h> // Hardware-specific library
// #include <SPI.h>
#include <string.h>


static LGFX tft;


static const int SCREEN_WIDTH = 240;
static const int SCREEN_HEIGHT = 320;
static const int X_CENTER = SCREEN_WIDTH / 2;
static const int Y_CENTER = SCREEN_HEIGHT / 2;
static const int ROWS = 5;
static const int COLS = 3;
static const int CELL_WIDTH = SCREEN_WIDTH / COLS;
static const int HALF_CELL_WIDTH = CELL_WIDTH / 2;
static const int CELL_HIGHT = SCREEN_HEIGHT / ROWS;
static const int HALF_CELL_HIGHT = CELL_HIGHT / 2;
static const int COL[] = {0, CELL_WIDTH, CELL_WIDTH * 2, CELL_WIDTH * 3, CELL_WIDTH * 4, CELL_WIDTH * 6, CELL_WIDTH * 7};
static const int ROW[] = {0, CELL_HIGHT, CELL_HIGHT * 2, CELL_HIGHT * 3, CELL_HIGHT * 4, CELL_HIGHT * 6, CELL_HIGHT * 7};


struct CellState {
	const char* key;
	char prevValue[24];
	int32_t prevColor;
};

static CellState g_cells[] = {
	{"bestLapTime",                 "", 0},
	{"currenLapTime",               "", 0},
	{"speed",                       "", 0},
	{"sessionBestLiveDeltaSeconds", "", 0},
	{"tcTcCut",                     "", 0},
	{"tcLevel",                     "", 0},
	{"absLevel",                    "", 0},
	{"brakeBias",                   "", 0},
};
static const int g_cellsCount = sizeof(g_cells) / sizeof(g_cells[0]);


class SHCustomProtocol
{
private:
	// Global variables
	int rpmPercent = 50;
	int prev_rpmPercent = 50;
	int rpmRedLineSetting = 90;
	String gear = "N";
	String prev_gear;
	int speed = 0;
	String currentLapTime = "00:00.00";
	String lastLapTime = "00:00.00";
	String bestLapTime = "00:00.00";
	String sessionBestLiveDeltaSeconds = "0.000";
	String sessionBestLiveDeltaProgressSeconds = "0.00";
	String tyrePressureFrontLeft = "00.0";
	String tyrePressureFrontRight = "00.0";
	String tyrePressureRearLeft = "00.0";
	String tyrePressureRearRight = "00.0";
	String tcLevel = "0";
	String tcActive = "0";
	String absLevel = "0";
	String absActive = "0";
	String isTCCutNull = "True";
	String tcTcCut = "0  0";
	String brakeBias = "0";
	String brake = "0";
	String lapInvalidated = "False";
	
	

public:
	/*
	CUSTOM PROTOCOL CLASS
	SEE https://github.com/zegreatclan/SimHub/wiki/Custom-Arduino-hardware-support

	GENERAL RULES :
		- ALWAYS BACKUP THIS FILE, reinstalling/updating SimHub would overwrite it with the default version.
		- Read data AS FAST AS POSSIBLE in the read function
		- NEVER block the arduino (using delay for instance)
		- Make sure the data read in "read()" function READS ALL THE DATA from the serial port matching the custom protocol definition
		- Idle function is called hundreds of times per second, never use it for slow code, arduino performances would fall
		- If you use library suspending interrupts make sure to use it only in the "read" function when ALL data has been read from the serial port.
			It is the only interrupt safe place

	COMMON FUNCTIONS :
		- FlowSerialReadStringUntil('\n')
			Read the incoming data up to the end (\n) won't be included
		- FlowSerialReadStringUntil(';')
			Read the incoming data up to the separator (;) separator won't be included
		- FlowSerialDebugPrintLn(string)
			Send a debug message to simhub which will display in the log panel and log file (only use it when debugging, it would slow down arduino in run conditions)

	*/

	// Called when starting the arduino (setup method in main sketch)
	void setup()
	{
		tft.init();
		tft.setRotation(0);
		tft.fillScreen(TFT_BLACK);
	}

	// Called when new data is coming from computer
	void read()
	{
		String full = "";

		speed = FlowSerialReadStringUntil(';').toInt();
		gear = FlowSerialReadStringUntil(';');
		rpmPercent = FlowSerialReadStringUntil(';').toInt();
		rpmRedLineSetting = FlowSerialReadStringUntil(';').toInt();
		currentLapTime = FlowSerialReadStringUntil(';');
		lastLapTime = FlowSerialReadStringUntil(';');
		bestLapTime = FlowSerialReadStringUntil(';');
		sessionBestLiveDeltaSeconds = FlowSerialReadStringUntil(';');
		sessionBestLiveDeltaProgressSeconds = FlowSerialReadStringUntil(';');
		tyrePressureFrontLeft  = FlowSerialReadStringUntil(';');
		tyrePressureFrontRight  = FlowSerialReadStringUntil(';');
		tyrePressureRearLeft  = FlowSerialReadStringUntil(';');
		tyrePressureRearRight  = FlowSerialReadStringUntil(';');
		tcLevel  = FlowSerialReadStringUntil(';');
		tcActive  = FlowSerialReadStringUntil(';');
		absLevel  = FlowSerialReadStringUntil(';');
		absActive  = FlowSerialReadStringUntil(';');
		isTCCutNull  = FlowSerialReadStringUntil(';');
		tcTcCut  = FlowSerialReadStringUntil(';');
		brakeBias  = FlowSerialReadStringUntil(';');
		brake  = FlowSerialReadStringUntil(';');
		lapInvalidated  = FlowSerialReadStringUntil(';');

		const String rest = FlowSerialReadStringUntil('\n');
	}

	// Called once per arduino loop, timing can't be predicted,
	// but it's called between each command sent to the arduino
	void loop()
	{
		//drawRpmMeter(0, 0, SCREEN_WIDTH, HALF_CELL_HIGHT);
		//drawGear(COL[2], COL[1]);
		drawGear(COL[1], COL[0]);
		
		// First+Second Column (Lap times)
		//drawCell(COL[0], ROW[1], bestLapTime, "bestLapTime", "Best Lap", "left");
		drawCell(COL[0], ROW[3], bestLapTime, "bestLapTime", "Best Lap", "left");
		//drawCell(COL[0], ROW[2], lastLapTime, "lastLapTime", "Last Lap", "left");
		drawCell(COL[0], ROW[2], currentLapTime, "currenLapTime", "Current Lap", "left", lapInvalidated == "True" ? TFT_RED : TFT_WHITE);


		// Third Column (speed)
		drawCell(SCREEN_WIDTH, ROW[2], String(speed), "speed", "Speed","speed",TFT_GREEN);

		// Fourth+Fifth Column (delta)
		//drawCell(SCREEN_WIDTH, ROW[1], sessionBestLiveDeltaSeconds, "sessionBestLiveDeltaSeconds", "Delta", "right", sessionBestLiveDeltaSeconds.indexOf('-') >= 0 ? TFT_GREEN : TFT_RED);
		drawCell(SCREEN_WIDTH, ROW[3], sessionBestLiveDeltaSeconds, "sessionBestLiveDeltaSeconds", "Delta", "right", sessionBestLiveDeltaSeconds.indexOf('-') >= 0 ? TFT_GREEN : TFT_RED);		
		//drawCell(SCREEN_WIDTH, ROW[2], sessionBestLiveDeltaProgressSeconds, "sessionBestLiveDeltaProgressSeconds", "Delta P", "right", sessionBestLiveDeltaProgressSeconds.indexOf('-') >= 0 ? TFT_GREEN : TFT_RED);
		

		// (TC, ABS, BB)
		if (isTCCutNull == "False")
			drawCell(COL[0], ROW[4], tcTcCut, "tcTcCut", "TC TC2", "center", TFT_YELLOW);
		else
			drawCell(COL[0], ROW[4], tcLevel, "tcLevel", "TC", "center", TFT_YELLOW);
		drawCell(COL[1], ROW[4], absLevel, "absLevel", "ABS", "center", TFT_BLUE);
		drawCell(COL[2], ROW[4], brakeBias, "brakeBias", "BB", "center", TFT_MAGENTA);

		// (tyre pressure)
		// drawCell(COL[3], ROW[3], tyrePressureFrontLeft, "tyrePressureFrontLeft", "FL", "center", TFT_CYAN);
		// drawCell(COL[4], ROW[3], tyrePressureFrontRight, "tyrePressureFrontRight", "FR", "center", TFT_CYAN);
		// drawCell(COL[3], ROW[4], tyrePressureRearLeft, "tyrePressureRearLeft", "RL", "center", TFT_CYAN);
		// drawCell(COL[4], ROW[4], tyrePressureRearRight, "tyrePressureRearRight", "RR", "center", TFT_CYAN);
	}

	// Called once between each byte read on arduino,
	// THIS IS A CRITICAL PATH :
	// AVOID ANY TIME CONSUMING ROUTINES !!!
	// PREFER READ OR LOOP METHOS AS MUCH AS POSSIBLE
	// AVOID ANY INTERRUPTS DISABLE (serial data would be lost!!!)
	void idle()
	{
	}

	void drawGear(int32_t x, int32_t y)
	{
		// draw gear only when it changes
		if (gear != prev_gear)
		{
			// tft.loadFont("Formula1_Regular_web_072pt7b", SPIFFS);
			tft.setTextColor(TFT_YELLOW, TFT_BLACK);
			tft.setTextSize(14);
			tft.setTextDatum(MC_DATUM);
			//tft.setCursor(x + 12, y + HALF_CELL_HIGHT);
			tft.setCursor(x, y + CELL_HIGHT);
			tft.print(gear);
			tft.setTextSize(1);
			tft.setTextDatum(TL_DATUM);

			prev_gear = gear;
		}
	}

	boolean isDrawGearRpmRedRec()
	{
		if (rpmPercent >= rpmRedLineSetting)
		{
			return true;
		}
		return false;
	}

	void drawRpmMeter(int32_t x, int32_t y, int width, int height)
	{
		int meterWidth = (width * rpmPercent) / 100;

		int yPlusOne = y + 1;
		int innerWidth = width - meterWidth - 1;
		int innerHeight = height - 4;

		if (prev_rpmPercent > rpmPercent)
		{
			tft.fillRect(meterWidth, yPlusOne, innerWidth, innerHeight, TFT_BLACK); // clear the part after the current rect width
		}

		if (rpmPercent >= rpmRedLineSetting)
		{
			tft.fillRect(x, yPlusOne, meterWidth - 2, innerHeight, TFT_RED);
		}
		else if (rpmPercent >= rpmRedLineSetting - 5)
		{
			tft.fillRect(x, yPlusOne, meterWidth - 2, innerHeight, TFT_ORANGE);
		}
		else
		{
			tft.fillRect(x, yPlusOne, meterWidth - 2, innerHeight, TFT_GREEN);
		}

		// draw the frame only if it ont there
		if (prev_rpmPercent == 50) tft.drawRect(x, y, width, height-2, TFT_WHITE);
		
		prev_rpmPercent = rpmPercent;
	}

	void drawCell(int32_t x, int32_t y, const String& data, const char* id, const char* name = "Data", const char* align = "center", int32_t color = TFT_WHITE, int fontSize = 4)
	{
		CellState* cell = nullptr;
		for (int i = 0; i < g_cellsCount; i++) {
			if (strcmp(g_cells[i].key, id) == 0) {
				cell = &g_cells[i];
				break;
			}
		}
		if (!cell) return;

		const static int titleHeight = 27;
		const static int hPadding = 5;
		const static int vPadding = 1;

		tft.setTextColor(color, TFT_BLACK);

		const bool dataChanged  = (strcmp(cell->prevValue, data.c_str()) != 0);
		const bool colorChanged = (cell->prevColor != color);

		if (dataChanged) {

			if (strcmp(align, "left") == 0)
			{
				//if (colorChanged) tft.drawRoundRect(x, y, CELL_WIDTH * 2 - 1, CELL_HIGHT - 2, 5, color);		// Rectangle
				if (colorChanged) tft.drawRoundRect(x, y, CELL_WIDTH * 1.5 - 1, CELL_HIGHT - 2, 5, color);
				if (colorChanged) tft.drawString(name, x + hPadding, y + vPadding, 2);						// Title
				tft.drawString(data, x + hPadding, y + titleHeight, fontSize); // Data
			}
			else if (strcmp(align, "right") == 0)
			{
				//if (colorChanged) tft.drawRoundRect(x - (CELL_WIDTH * 2), y, CELL_WIDTH * 2 - 1, CELL_HIGHT - 2, 5, color); // Rectangle
				if (colorChanged) tft.drawRoundRect(x - (CELL_WIDTH * 1.5), y, CELL_WIDTH * 1.5 - 1, CELL_HIGHT - 2, 5, color);
				if (colorChanged) tft.drawRightString(name, x - hPadding, y + vPadding, 2);					// Title
				tft.drawRightString(data, x - hPadding, y + titleHeight, fontSize);	  // Data
			}
			else if (strcmp(align, "speed") == 0)
			{
				//if (colorChanged) tft.drawRoundRect(x - (CELL_WIDTH * 2), y, CELL_WIDTH * 2 - 1, CELL_HIGHT - 2, 5, color); // Rectangle
				if (colorChanged) tft.drawRoundRect(x - (CELL_WIDTH * 1.5), y, CELL_WIDTH * 1.5 - 1, CELL_HIGHT - 2, 5, color);
				if (colorChanged) tft.drawCentreString(name, x - HALF_CELL_WIDTH - 15, y + vPadding, 2);				// Title
				tft.drawCentreString(data, x - HALF_CELL_WIDTH - 15, y + titleHeight, fontSize); // Data
			}
			else // "center"
			{
				if (colorChanged) tft.drawRoundRect(x, y, CELL_WIDTH - 2, CELL_HIGHT - 2, 5, color);	 // Rectangle
				if (colorChanged) tft.drawCentreString(name, x + HALF_CELL_WIDTH, y + vPadding, 2);		 // Title
				tft.drawCentreString(data, x + HALF_CELL_WIDTH, y + titleHeight, fontSize); // Data
			}

			// Clear previous value if it was wider
			if (strlen(cell->prevValue) > data.length())
			{
				tft.setTextColor(TFT_BLACK, TFT_BLACK);
				if (strcmp(align, "left") == 0)
				{
					tft.drawString(cell->prevValue, x + hPadding, y + titleHeight, fontSize);
				}
				else if (strcmp(align, "right") == 0)
				{
					tft.drawRightString(cell->prevValue, x - hPadding, y + titleHeight, fontSize);
				}
				else if (strcmp(align, "speed") == 0)
				{
					tft.drawCentreString(cell->prevValue, x - HALF_CELL_WIDTH - 15, y + titleHeight, fontSize);
				}
				else
				{
					tft.drawCentreString(cell->prevValue, x + HALF_CELL_WIDTH, y + titleHeight, fontSize);
				}
			}

			strncpy(cell->prevValue, data.c_str(), sizeof(cell->prevValue) - 1);
			cell->prevValue[sizeof(cell->prevValue) - 1] = '\0';
			cell->prevColor = color;
		}

	}

};

#endif
