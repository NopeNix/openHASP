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
#include "hasp_gui.h" // gui_settings
#include "../tft/tft_driver_arduinogfx.h" // for haspTft

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

// Touch filtering defaults (overridable via GUI config)
#define XPT2046_SAMPLES_DEFAULT      5   // Number of samples to average
#define XPT2046_DEBOUNCE_DEFAULT     3   // Consecutive samples required for state change
#define XPT2046_PRESSURE_MIN_DEFAULT 10  // Minimum pressure (z) to register touch
#define XPT2046_SMOOTHING_DEFAULT_PERMILLE 700 // Exponential smoothing weight in permille (0..1000)

IRAM_ATTR bool TouchXpt2046::read(lv_indev_drv_t* indev_driver, lv_indev_data_t* data)
{
    static uint8_t touch_count = 0;      // Consecutive touch samples
    static uint8_t release_count = 0;    // Consecutive release samples
    static bool touch_active = false;    // Current debounced touch state
    static int16_t last_x = 0, last_y = 0; // Last reported coordinates for smoothing
    
    // Touch filtering parameters (from GUI config)
    uint8_t samples = (gui_settings.xpt_samples == 0) ? XPT2046_SAMPLES_DEFAULT : gui_settings.xpt_samples;
    uint8_t debounce = (gui_settings.xpt_debounce == 0) ? XPT2046_DEBOUNCE_DEFAULT : gui_settings.xpt_debounce;
    uint16_t pressure_min = (gui_settings.xpt_pressure_min == 0) ? XPT2046_PRESSURE_MIN_DEFAULT : gui_settings.xpt_pressure_min;
    float smoothing_permille = (gui_settings.xpt_smoothing_permille == 0) ? XPT2046_SMOOTHING_DEFAULT_PERMILLE
                                                                            : gui_settings.xpt_smoothing_permille;
    float smoothing = smoothing_permille / 1000.0f;

    uint16_t pressure_ref = gui_settings.xpt_pressure_ref;
    int32_t pressure_x_coeff = gui_settings.xpt_pressure_x_coeff;
    int32_t pressure_y_coeff = gui_settings.xpt_pressure_y_coeff;

    // Prevent invalid values
    if(samples < 2) samples = 2;
    if(samples > 10) samples = 10;
    if(debounce < 1) debounce = 1;
    if(debounce > 10) debounce = 10;
    if(pressure_min < 1) pressure_min = 1;

    // Check if touched with pressure validation
    bool raw_touched = xpt2046_ts.touched();
    bool valid_touch = false;
    
    if(raw_touched) {
        // Quick check of pressure to filter out noise
        TS_Point p = xpt2046_ts.getPoint();
        if(p.z >= pressure_min) {
            valid_touch = true;
        }
    }
    
    // Debounce touch state
    if(valid_touch) {
        touch_count++;
        release_count = 0;
    } else {
        release_count++;
        touch_count = 0;
    }
    
    // Update debounced state
    if(touch_count >= debounce && !touch_active) {
        touch_active = true;
    } else if(release_count >= debounce && touch_active) {
        touch_active = false;
    }
    
    // Process touch if active
    if(touch_active) {
        if(hasp_sleep_state != HASP_SLEEP_OFF) hasp_update_sleep_state();
        
        // Collect multiple samples for averaging
        int32_t sum_x = 0, sum_y = 0, sum_z = 0;
        uint16_t valid_samples = 0;
        
        for(int i = 0; i < samples; i++) {
            if(xpt2046_ts.touched()) {
                TS_Point p = xpt2046_ts.getPoint();
                if(p.z >= pressure_min) {
                    sum_x += p.x;
                    sum_y += p.y;
                    sum_z += p.z;
                    valid_samples++;
                }
            }
            // Small delay between samples for stability
            if(i < samples - 1) delayMicroseconds(100);
        }
        
        // Only process if we got enough valid samples
        if(valid_samples >= (samples / 2)) {
            // Calculate average
            int16_t x = sum_x / valid_samples;
            int16_t y = sum_y / valid_samples;
            uint16_t z_avg = sum_z / valid_samples;
            
            // Apply calibration
            #if XPT2046_XY_SWAP != 0
            int16_t swap_tmp = x;
            x = y;
            y = swap_tmp;
            #endif
            
            // Clamp to calibration range
            if(x < _min_x) x = _min_x;
            if(x > _max_x) x = _max_x;
            if(y < _min_y) y = _min_y;
            if(y > _max_y) y = _max_y;
            
            // Map to screen coordinates
            x = map(x, _min_x, _max_x, 0, TFT_WIDTH - 1);
            y = map(y, _min_y, _max_y, 0, TFT_HEIGHT - 1);
            
            // Apply inversion
            #if XPT2046_X_INV != 0
            x = TFT_WIDTH - 1 - x;
            #endif
            #if XPT2046_Y_INV != 0
            y = TFT_HEIGHT - 1 - y;
            #endif

            // Pressure compensation: shift opposite to pressure-induced bias
            // shift_px = (z_avg - pressure_ref) * coeff / 1000
            if(pressure_x_coeff != 0 || pressure_y_coeff != 0) {
                int32_t dz = (int32_t)z_avg - (int32_t)pressure_ref;
                int32_t x_shift = (dz * pressure_x_coeff) / 1000;
                int32_t y_shift = (dz * pressure_y_coeff) / 1000;
                x = (int16_t)(x - x_shift);
                y = (int16_t)(y - y_shift);
            }

            // Clamp
            if(x < 0) x = 0;
            if(x > (TFT_WIDTH - 1)) x = TFT_WIDTH - 1;
            if(y < 0) y = 0;
            if(y > (TFT_HEIGHT - 1)) y = TFT_HEIGHT - 1;
            
            // Apply exponential smoothing to reduce jitter
            if(last_x != 0 || last_y != 0) {
                x = (int16_t)(smoothing * x + (1.0f - smoothing) * last_x);
                y = (int16_t)(smoothing * y + (1.0f - smoothing) * last_y);
            }
            
            // Apply rotation
            switch(_rotation) {
                case 0:
                    data->point.x = x;
                    data->point.y = y;
                    break;
                case 1:
                    data->point.x = TFT_HEIGHT - 1 - y;
                    data->point.y = x;
                    break;
                case 2:
                    data->point.x = TFT_WIDTH - 1 - x;
                    data->point.y = TFT_HEIGHT - 1 - y;
                    break;
                case 3:
                    data->point.x = y;
                    data->point.y = TFT_WIDTH - 1 - x;
                    break;
                default:
                    data->point.x = x;
                    data->point.y = y;
                    break;
            }
            
            // Store for next smoothing
            last_x = x;
            last_y = y;
            
            data->state = LV_INDEV_STATE_PR;
            hasp_set_sleep_offset(0);
            return false;
        }
    }
    
    // Touch released or invalid
    data->state = LV_INDEV_STATE_REL;
    last_x = 0;
    last_y = 0;
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
    // All variables must be declared at the top (C++ requirement)
    // Color values (RGB565) - use different names to avoid conflicts with macros
    uint16_t col_black = 0x0000;
    uint16_t col_white = 0xFFFF;
    uint16_t col_magenta = 0xF81F;
    int margin = 20;
    int radius = 5;
    int16_t points[4][2];
    uint16_t rawX[4] = {0, 0, 0, 0};
    uint16_t rawY[4] = {0, 0, 0, 0};
    int i, x, y;
    uint32_t sumX, sumY;
    uint16_t samples;
    uint32_t startTime;
    uint16_t xMin, xMax, yMin, yMax;
    
    LOG_INFO(TAG_DRVR, "Starting XPT2046 visual calibration...");
    
    // Initialize calibration points
    points[0][0] = margin; points[0][1] = margin;                           // Top-left
    points[1][0] = TFT_WIDTH - margin - 1; points[1][1] = margin;           // Top-right
    points[2][0] = TFT_WIDTH - margin - 1; points[2][1] = TFT_HEIGHT - margin - 1; // Bottom-right
    points[3][0] = margin; points[3][1] = TFT_HEIGHT - margin - 1;          // Bottom-left
    
    // Clear screen
    haspTft.tft->fillScreen(col_black);
    
    // Show instruction text
    haspTft.tft->setTextSize(2);
    haspTft.tft->setTextColor(col_white, col_black);
    haspTft.tft->setCursor(10, TFT_HEIGHT / 2 - 20);
    haspTft.tft->println("Touch each corner");
    haspTft.tft->setCursor(10, TFT_HEIGHT / 2 + 10);
    haspTft.tft->println("as indicated");
    
    delay(1000);
    
    // Clear screen again
    haspTft.tft->fillScreen(col_black);
    
    // Calibrate each corner
    for(i = 0; i < 4; i++) {
        // Draw calibration point (crosshair + circle)
        x = points[i][0];
        y = points[i][1];
        
        // Draw circle
        haspTft.tft->fillCircle(x, y, radius, col_magenta);
        // Draw cross
        haspTft.tft->drawLine(x - radius - 3, y, x + radius + 3, y, col_white);
        haspTft.tft->drawLine(x, y - radius - 3, x, y + radius + 3, col_white);
        
        // Wait for touch
        LOG_INFO(TAG_DRVR, "Touch point %d at (%d, %d)", i + 1, x, y);
        
        // Wait for touch press
        while(!xpt2046_ts.touched()) {
            delay(10);
        }
        
        // Collect multiple samples for stability
        sumX = 0;
        sumY = 0;
        samples = 0;
        startTime = millis();
        
        // Sample for 500ms while touch is held
        while(xpt2046_ts.touched() && (millis() - startTime) < 500) {
            TS_Point p = xpt2046_ts.getPoint();
            sumX += p.x;
            sumY += p.y;
            samples++;
            delay(10);
        }
        
        if(samples > 0) {
            rawX[i] = sumX / samples;
            rawY[i] = sumY / samples;
            LOG_INFO(TAG_DRVR, "Point %d: raw X=%d, Y=%d", i + 1, rawX[i], rawY[i]);
        }
        
        // Clear the point (draw black over it)
        haspTft.tft->fillCircle(x, y, radius + 5, col_black);
        
        // Wait for release
        while(xpt2046_ts.touched()) {
            delay(10);
        }
        
        delay(200); // Debounce
    }
    
    // Calculate calibration values from corner points
    // X values from left/right points (may be inverted)
    uint16_t xLeft = (rawX[0] + rawX[3]) / 2;   // Average of left points
    uint16_t xRight = (rawX[1] + rawX[2]) / 2;  // Average of right points
    
    // Y values from top/bottom points
    uint16_t yTop = (rawY[0] + rawY[1]) / 2;    // Average of top points
    uint16_t yBottom = (rawY[2] + rawY[3]) / 2; // Average of bottom points
    
    LOG_INFO(TAG_DRVR, "Averages - xLeft=%d, xRight=%d, yTop=%d, yBottom=%d", xLeft, xRight, yTop, yBottom);
    
    // Determine min/max (handles inverted axes)
    _min_x = (xLeft < xRight) ? xLeft : xRight;
    _max_x = (xLeft < xRight) ? xRight : xLeft;
    _min_y = (yTop < yBottom) ? yTop : yBottom;
    _max_y = (yTop < yBottom) ? yBottom : yTop;
    
    LOG_INFO(TAG_DRVR, "Min/Max - _min_x=%d, _max_x=%d, _min_y=%d, _max_y=%d", _min_x, _max_x, _min_y, _max_y);
    
    // Check that we have valid ranges
    if(_max_x > _min_x && _max_y > _min_y) {
        // Store in calData for saving to config
        calData[0] = _min_x;
        calData[1] = _max_x;
        calData[2] = _min_y;
        calData[3] = _max_y;
        calData[4] = 0;
        
        LOG_INFO(TAG_DRVR, "Calibration complete!");
        LOG_INFO(TAG_DRVR, "X: %d - %d (left=%d, right=%d)", _min_x, _max_x, xLeft, xRight);
        LOG_INFO(TAG_DRVR, "Y: %d - %d (top=%d, bottom=%d)", _min_y, _max_y, yTop, yBottom);
        
        // Show success message
        haspTft.tft->fillScreen(col_black);
        haspTft.tft->setTextSize(2);
        haspTft.tft->setTextColor(col_white, col_black);
        haspTft.tft->setCursor(TFT_WIDTH / 2 - 60, TFT_HEIGHT / 2 - 10);
        haspTft.tft->println("Calibration");
        haspTft.tft->setCursor(TFT_WIDTH / 2 - 40, TFT_HEIGHT / 2 + 20);
        haspTft.tft->println("Complete!");
    } else {
        LOG_ERROR(TAG_DRVR, "Calibration failed - invalid values");
        
        // Show error message
        haspTft.tft->fillScreen(col_black);
        haspTft.tft->setTextSize(2);
        haspTft.tft->setTextColor(0xF800, col_black); // Red
        haspTft.tft->setCursor(TFT_WIDTH / 2 - 50, TFT_HEIGHT / 2 - 10);
        haspTft.tft->println("Calibration");
        haspTft.tft->setCursor(TFT_WIDTH / 2 - 40, TFT_HEIGHT / 2 + 20);
        haspTft.tft->println("Failed!");
    }
    
    delay(1000);
}

} // namespace dev

dev::TouchXpt2046 haspTouch;

#endif // ARDUINO
