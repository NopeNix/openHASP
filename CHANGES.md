# Changes in This Fork

## Summary

This fork adds support for the **Sunton ESP32-8048S043R** (4.3" resistive touch display) to openHASP, including XPT2046 touch driver support for the ArduinoGFX display driver.

## Detailed Changes

### 1. New XPT2046 Touch Driver for ArduinoGFX

**Files Added:**
- `src/drv/touch/touch_driver_xpt2046.cpp`
- `src/drv/touch/touch_driver_xpt2046.h`

**Features:**
- Full XPT2046 resistive touch support via SPI
- Configurable calibration (X_MIN, X_MAX, Y_MIN, Y_MAX)
- XY swap and inversion support
- Rotation handling
- Integrated with ArduinoGFX driver stack

### 2. Board Configuration

**Modified:**
- `user_setups/esp32s3/sunton-esp32-s3-tft.ini`
  - Added `sunton-8048s043r_16MB` environment
  - SPI pins: SCLK=12, MISO=13, MOSI=11, CS=38, IRQ=18
  - Default calibration values for 800x480 display

- `user_setups/esp32s3/_esp32s3.ini`
  - Added `[xpt2046]` library definition
  - Added lib_ignore for clean builds

### 3. Driver Selection Logic

**Modified:**
- `src/drv/touch/touch_driver.h`
  - Added conditional for ArduinoGFX XPT2046: `#if TOUCH_DRIVER == 0x2046 && defined(HASP_USE_ARDUINOGFX)`
  - Falls back to TFT_eSPI driver if `USER_SETUP_LOADED` is defined
  - Falls back to LovyanGFX if `LGFX_USE_V1` is defined

- `src/drv/old/hasp_drv_touch.cpp`
  - Added guard to prevent conflicts with new ArduinoGFX driver
  - Old driver only used when `!defined(HASP_USE_ARDUINOGFX)`

- `src/hasp_gui.cpp`
  - Added conditional compilation for touch preferences
  - Only compiles touch code when `TOUCH_DRIVER != -1 || USE_MONITOR`

### 4. Library Updates

**Modified:**
- `lib/XPT2046_Touchscreen/library.json`
  - Updated with proper PlatformIO configuration
  - Added build settings for ESP32 compatibility

- `lib/XPT2046_Touchscreen/` (renamed from `XPT2046_Touchscreen_ID542`)
  - Renamed folder for cleaner library references

### 5. Web Editor Fix

**Modified:**
- `data/edit.htm`
  - ACE editor: 1.43.6 → 1.43.3 (1.43.6 no longer available on CDN)

- `data/script.js`
  - ACE editor: 1.43.6 → 1.43.3

**Reason:** ACE 1.43.6 was removed from CDN, causing config changes to fail via web UI and FTP.

### 6. Version Update

**Modified:**
- `platformio.ini`
  - Version: 0.7.0.1 → 0.7.0.2-NopeNix
  - Clearly identifies this fork from upstream

### 7. Build System

**Added:**
- `Dockerfile.platformio`
  - Containerized PlatformIO build environment
  - Includes FreeType support
  - Pre-installs ESP32 platform

- `build-docker.sh`
  - One-command build script
  - Handles Docker setup and firmware compilation

- `.github/workflows/build-docker.yaml`
  - GitHub Actions CI/CD for Docker builds

## Testing

Tested on:
- Sunton ESP32-8048S043R 16MB
- Display: ST7262 800x480 RGB
- Touch: XPT2046 SPI

## Known Issues

None currently.

## Upstream Compatibility

These changes are designed to be non-breaking:
- XPT2046 driver only activates with explicit `TOUCH_DRIVER=0x2046` + `HASP_USE_ARDUINOGFX`
- Other touch controllers (GT911, FT6336U, etc.) unaffected
- Other boards not using XPT2046 will continue to work normally
