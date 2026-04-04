/* MIT License - Copyright (c) 2019-2024 Francis Van Roie
   For full license information read the LICENSE file in the project folder */

#ifndef HASP_GUI_H
#define HASP_GUI_H

#include "hasplib.h"

struct bmp_header_t
{
    uint32_t bfSize;
    uint32_t bfReserved;
    uint32_t bfOffBits;

    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;

    uint32_t bdMask[3]; // RGB
};

struct gui_conf_t
{
    bool show_pointer;
    int8_t backlight_pin;
    uint8_t rotation;
    uint8_t invert_display;

    // XPT2046 touch filtering parameters (ArduinoGFX driver only)
    // Stored in GUI config so users can tune jitter filtering.
    uint8_t  xpt_samples;              // samples averaged per read
    uint8_t  xpt_debounce;             // consecutive samples for state change
    uint16_t xpt_pressure_min;        // min pressure (z) to accept touch
    uint16_t xpt_smoothing_permille; // exponential smoothing weight in permille (0..1000)
#if defined(USER_SETUP_LOADED)
    uint16_t cal_data[5];
#else
    uint16_t cal_data[8];
#endif
};

// Current GUI configuration in RAM (used by touch filtering + GUI pages)
extern gui_conf_t gui_settings;

/* ===== Default Event Processors ===== */
void guiTftInit(void);
void guiSetup(void);
IRAM_ATTR void guiLoop(void);
void guiEverySecond(void);
void guiStart(void);
void guiStop(void);
void gui_hide_pointer(bool hidden);

/* ===== Special Event Processors ===== */
void guiCalibrate(void);
void guiTakeScreenshot(const char* pFileName); // to file
void guiTakeScreenshot(void);                  // webclient
bool guiScreenshotIsDirty();
uint32_t guiScreenshotEtag();

/* ===== Callbacks ===== */
void gui_flush_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
void gui_antiburn_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);

/* ===== Main LVGL Task ===== */
#if HASP_USE_LVGL_TASK == 1
void gui_task(void* args);
#endif

/* ===== Locks ===== */
#ifdef ESP32
IRAM_ATTR bool gui_acquire(TickType_t timeout);
IRAM_ATTR void gui_release(void);
esp_err_t gui_setup_lvgl_task(void);
#endif

/* ===== Read/Write Configuration ===== */
#if HASP_USE_CONFIG > 0
bool guiGetConfig(const JsonObject& settings);
bool guiSetConfig(const JsonObject& settings);
#endif // HASP_USE_CONFIG

#endif // HASP_GUI_H
