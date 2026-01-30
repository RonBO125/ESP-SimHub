#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

// ESP32-S2 Configuration for LovyanGFX with ST7789 Display

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789      _panel_instance;
  lgfx::Bus_I2C           _bus_instance;   // I2C bus for OLED
  lgfx::Light_PWM         _light_instance;

public:

  LGFX(void)
  {
    { // I2C Bus Configuration
      auto cfg = _bus_instance.config();
      
      cfg.i2c_port    = 0;          // I2C Port (0 or 1)
      cfg.freq_write  = 400000;     // Write clock
      cfg.freq_read   = 400000;     // Read clock
      cfg.pin_sda     = 21;         // SDA pin (GPIO 21)
      cfg.pin_scl     = 22;         // SCL pin (GPIO 22)
      cfg.i2c_addr    = 0x3C;       // I2C device address

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    { // Display Panel Configuration
      auto cfg = _panel_instance.config();

      cfg.pin_cs           =     5;  // CS pin (GPIO 5)
      cfg.pin_rst          =     4;  // RST pin (GPIO 4)
      cfg.pin_busy         =    -1;  // BUSY pin (-1 = disable)

      cfg.panel_width      =   240;  // Display width
      cfg.panel_height     =   320;  // Display height
      cfg.offset_x         =     0;  // X offset
      cfg.offset_y         =     0;  // Y offset
      cfg.offset_rotation  =     0;  // Rotation offset (0~7)
      cfg.dummy_read_pixel =     8;  // Dummy read bits before pixel read
      cfg.dummy_read_bits  =     1;  // Dummy read bits before non-pixel read
      cfg.readable         =  true;  // Data read support
      cfg.invert           = false;  // Invert display brightness
      cfg.rgb_order        = false;  // RGB/BGR order
      cfg.dlen_16bit       = false;  // 16-bit data length for parallel/SPI
      cfg.bus_shared       =  true;  // Shared bus with SD card

      _panel_instance.config(cfg);
    }

    { // Backlight Configuration
      auto cfg = _light_instance.config();

      cfg.pin_bl = 32;              // Backlight pin (GPIO 32 if used)
      cfg.invert = false;           // Invert brightness
      cfg.freq   = 44100;           // PWM frequency
      cfg.pwm_channel = 7;          // PWM channel

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};
