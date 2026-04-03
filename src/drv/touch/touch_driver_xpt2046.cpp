/* MIT License - Copyright (c) 2019-2024 Francis Van Roie
   For full license information read the LICENSE file in the project folder */

#if defined(ARDUINO) && (TOUCH_DRIVER == 0x2046) && defined(HASP_USE_ARDUINOGFX)
#include <Arduino.h>
#include "ArduinoLog.h"
#include "hasp_conf.h"
#include "touch_driver_xpt2046.h"

#include <SPI.h>

// Define the SPI object expected by XPT2046_Touchscreen library
SPIClass xpt2046_spi(HSPI);

#include "../../lib/XPT2046_Touchscreen/XPT2046_Touchscreen.h"

#include "touch_driver.h" // base class

#include "../../hasp/hasp.h" // for hasp_sleep_state
extern uint8_t hasp_sleep_state;

// XPT2046 touch controller instance
// CS pin is TOUCH_CS, IRQ pin is TOUCH_IRQ (can be 255 if not used)
XPT2046_Touchscreen xpt2046_ts(TOUCH_CS, TOUCH_IRQ);

// Calibration values - can be adjusted via web interface
#ifndef XPT2046_X_MIN
#define XPT2046_X_MIN 200
#endif
#ifndef XPT2046_X_MAX
#define XPT2046_X_MAX 3700
#endif
#ifndef XPT2046_Y_MIN
#define XPT2046_Y_MIN 200
#endif
#ifndef XPT2046_Y_MAX
#define XPT2046_Y_MAX 3700
#endif
#ifndef XPT2046_XY_SWAP
#define XPT2046_XY_SWAP 0
#endif
#ifndef XPT2046_X_INV
#define XPT2046_X_INV 0
#endif
#ifndef XPT2046_Y_INV
#define XPT2046_Y_INV 0
#endif

namespace dev {

IRAM_ATTR bool TouchXpt2046::read(lv_indev_drv_t* indev_driver, lv_indev_data_t* data)
{
    bool touched = xpt2046_ts.touched();
    
    if(touched) {
        if(hasp_sleep_state != HASP_SLEEP_OFF) hasp_update_sleep_state(); // update Idle

        TS_Point p = xpt2046_ts.getPoint();
        
        // Apply calibration
        int16_t x = p.x;
        int16_t y = p.y;
        
        // Swap X/Y if needed
        #if XPT2046_XY_SWAP != 0
        int16_t swap_tmp = x;
        x = y;
        y = swap_tmp;
        #endif
        
        // Apply min/max calibration
        if(x < _min_x) x = _min_x;
        if(x > _max_x) x = _max_x;
        if(y < _min_y) y = _min_y;
        if(y > _max_y) y = _max_y;
        
        // Map to screen coordinates
        x = map(x, _min_x, _max_x, 0, TFT_WIDTH - 1);
        y = map(y, _min_y, _max_y, 0, TFT_HEIGHT - 1);
        
        // Invert if needed
        #if XPT2046_X_INV != 0
        x = TFT_WIDTH - 1 - x;
        #endif
        #if XPT2046_Y_INV != 0
        y = TFT_HEIGHT - 1 - y;
        #endif
        
        // Apply rotation
        switch(_rotation) {
            case 0:
                data->point.x = x;
                data->point.y = y;
                break;
            case 1: // 90 degrees
                data->point.x = TFT_HEIGHT - 1 - y;
                data->point.y = x;
                break;
            case 2: // 180 degrees
                data->point.x = TFT_WIDTH - 1 - x;
                data->point.y = TFT_HEIGHT - 1 - y;
                break;
            case 3: // 270 degrees
                data->point.x = y;
                data->point.y = TFT_WIDTH - 1 - x;
                break;
            default:
                data->point.x = x;
                data->point.y = y;
                break;
        }
        
        data->state = LV_INDEV_STATE_PR;
        hasp_set_sleep_offset(0); // Reset the offset

    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    /*Return `false` because we are not buffering and no more data to read*/
    return false;
}

void TouchXpt2046::init(int w, int h)
{
    // Initialize SPI for XPT2046
    // XPT2046 uses SPI mode 0, MSB first
    SPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    
    if(xpt2046_ts.begin()) {
        LOG_INFO(TAG_DRVR, "XPT2046 %s", D_SERVICE_STARTED);
    } else {
        LOG_WARNING(TAG_DRVR, "XPT2046 %s", D_SERVICE_START_FAILED);
    }
    
    // Set initial rotation
    _rotation = 0;
    xpt2046_ts.setRotation(_rotation);
    
    // Set default calibration values
    _min_x = XPT2046_X_MIN;
    _max_x = XPT2046_X_MAX;
    _min_y = XPT2046_Y_MIN;
    _max_y = XPT2046_Y_MAX;
}

void TouchXpt2046::set_rotation(uint8_t rotation)
{
    _rotation = rotation % 4;
    xpt2046_ts.setRotation(_rotation);
}

void TouchXpt2046::set_calibration(uint16_t* calData)
{
    // calData format: [min_x, max_x, min_y, max_y, flags]
    // Only apply if values are valid (non-zero and min < max)
    
    if(calData[0] != 0 && calData[1] != 0 && calData[0] < calData[1]) {
        _min_x = calData[0];
        _max_x = calData[1];
        LOG_INFO(TAG_DRVR, "XPT2046 X calibration: %d - %d", _min_x, _max_x);
    }
    
    if(calData[2] != 0 && calData[3] != 0 && calData[2] < calData[3]) {
        _min_y = calData[2];
        _max_y = calData[3];
        LOG_INFO(TAG_DRVR, "XPT2046 Y calibration: %d - %d", _min_y, _max_y);
    }
}

void TouchXpt2046::calibrate(uint16_t* calData)
{
    // Calibration is done via web UI or config file
    // This function just applies the calibration values
    set_calibration(calData);
    
    LOG_INFO(TAG_DRVR, "XPT2046 calibration applied");
}

} // namespace dev

dev::TouchXpt2046 haspTouch;

#endif // ARDUINO
