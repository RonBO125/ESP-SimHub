

#pragma once

#include "USBHID.h"
#define MAX_BUTTONS 18

struct JoystickDescription {
  uint16_t vendorId;
  uint16_t productId;
  const char *name;
  const char *manufacturer;
};

class ECrowneJoystick : public USBHIDDevice {
private:
  USBHID hid;
  uint8_t _buttons[3]; 
  bool write();

public:
  ECrowneJoystick(void);
  void begin(JoystickDescription description);
  void end(void);

  // Set button as pressed and send state
  bool pressButton(uint8_t button);
  // Set button as released and send state
  bool releaseButton(uint8_t button);
  // Set button state without sending state
  bool setButton(uint8_t button, bool state);
  // Send state
  bool sendState();

  // internal use
  uint16_t _onGetDescriptor(uint8_t *buffer);
};

// https://www.usb.org/sites/default/files/hut1_5.pdf
#define HID_USAGE_SIMULATION_THROTTLE 0xBB
#define HID_USAGE_SIMULATION_ACCELERATOR 0xC4
#define HID_USAGE_SIMULATION_BRAKE 0xC5

/**
 * This is how we're defining the report descriptor for the joystick.
 * 
 * We're using the same pattern as the espressif gamepad library and reusing some of the constants there
 * 
 * The report descriptor is a byte array that describes the data that the device is sending to the host.
 * The report itself is also a byte array that conforms to this descriptor contract
 */
#define TUD_HID_REPORT_DESC_ECROWNE_JOYSTICK(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_JOYSTICK )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    \
    /* 12 bit Button Map */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN      ( 1                                      ) ,\
    HID_USAGE_MAX      ( MAX_BUTTONS                            ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 1                                      ) ,\
    HID_REPORT_SIZE    ( 1                                      ) ,\
    HID_REPORT_COUNT   ( MAX_BUTTONS                            ) ,\
    HID_UNIT_EXPONENT  ( 0                                      ) ,\
    HID_UNIT           ( 0                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 6 bit padding to align to byte boundary (18+6=24 bits = 3 bytes) */ \
    HID_REPORT_SIZE    ( 1                                      ) ,\
    HID_REPORT_COUNT   ( 6                                      ) ,\
    HID_INPUT          ( HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END
    
    



typedef struct TU_ATTR_PACKED
{
  uint8_t buttons[3];  ///< Buttons mask for currently pressed buttons, supports 18 (3x8 = 24, using 18)
} hid_joystick_report_t;
