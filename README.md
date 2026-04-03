# openHASP firmware - Sunton 8048S043R Fork

> **⚠️ This is a community fork specifically for the Sunton ESP32-8048S043R (4.3" Resistive Touch Display with 16MB Flash)**
> 
> **🔧 Adds XPT2046 Resistive Touch Support for ArduinoGFX**
> 
> **🐳 Docker-based build system included for reproducible builds**

---

## What's Different in This Fork?

| Feature | Original openHASP | This Fork |
|---------|------------------|-----------|
| **Sunton 8048S043R Support** | ❌ No | ✅ Full support with XPT2046 touch |
| **XPT2046 ArduinoGFX Driver** | ❌ Not available | ✅ Custom driver included |
| **ACE Editor Version** | ❌ 1.43.6 (broken) | ✅ 1.43.3 (working) |
| **Build System** | Local PlatformIO | Docker + Containerized builds |
| **Version** | 0.7.0.1 | 0.7.0.2-NopeNix |

---

## Why This Fork?

The **Sunton ESP32-8048S043R** is a 4.3" 800x480 display with:
- **ESP32-S3** dual-core 240MHz
- **16MB Flash** / **8MB PSRAM**
- **ST7262** RGB display driver
- **XPT2046** resistive touch controller (SPI)

**The Problem:** The original openHASP only supports the **8048S043C** (capacitive touch) variant. The **8048S043R** (resistive touch) uses a different touch controller (XPT2046) that wasn't supported with the ArduinoGFX driver.

**This fork adds:**
1. Complete XPT2046 touch driver for ArduinoGFX
2. Working ACE editor (1.43.3) - 1.43.6 is no longer available
3. Docker-based build system for consistent builds
4. Pre-configured board definition for 8048S043R 16MB

---

## Quick Start - Docker Build (Recommended)

### Prerequisites
- Docker installed on your system
- USB passthrough for flashing (Linux: add user to `dialout` group)

### 1. Clone and Enter Repository

```bash
git clone https://github.com/nopenix/openHASP.git
cd openHASP
```

### 2. Build Using Docker

```bash
# Build the Docker image (first time only)
docker build -t openhasp-builder -f Dockerfile.platformio .

# Start the build container
docker run -d --name openhasp-builder --privileged \
  -v "$(pwd):/workspace" \
  -v "openhasp-pio-cache:/root/.platformio" \
  -w /workspace \
  openhasp-builder tail -f /dev/null

# Build the firmware
docker exec openhasp-builder pio run -e sunton-8048s043r_16MB
```

### 3. Flash to Device

```bash
# Using esptool (outside container)
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x0 .pio/build/sunton-8048s043r_16MB/firmware.bin

# Or copy the merged firmware
cp build_output/firmware/sunton-8048s043r_full_16MB_*.bin /path/to/flash/
```

---

## Alternative: Local Build (PlatformIO)

### Prerequisites
- Python 3.9+
- PlatformIO Core: `pip install platformio`
- Git submodules initialized

### Build Steps

```bash
# Clone repository
git clone https://github.com/nopenix/openHASP.git
cd openHASP

# IMPORTANT: Initialize submodules (required!)
git submodule update --init --recursive

# Build firmware
pio run -e sunton-8048s043r_16MB
```

---

## Configuration

### Touch Calibration

The XPT2046 touch controller may need calibration. Adjust these values in the config or via web UI:

```ini
-D XPT2046_X_MIN=200
-D XPT2046_X_MAX=3700
-D XPT2046_Y_MIN=200
-D XPT2046_Y_MAX=3700
-D XPT2046_XY_SWAP=0
-D XPT2046_X_INV=0
-D XPT2046_Y_INV=0
```

### Available Environments

| Environment | Board | Flash | Touch | Notes |
|-------------|-------|-------|-------|-------|
| `sunton-8048s043r_16MB` | Sunton 8048S043R | 16MB | XPT2046 | **This fork's main target** |
| `sunton-8048s043c_16MB` | Sunton 8048S043C | 16MB | GT911 | Capacitive variant |
| `sunton-8048s050c_16MB` | Sunton 8048S050C | 16MB | GT911 | 5.0" version |
| `sunton-8048s070c_16MB` | Sunton 8048S070C | 16MB | GT911 | 7.0" version |

---

## Technical Changes

### Files Added/Modified

1. **New XPT2046 Touch Driver**
   - `src/drv/touch/touch_driver_xpt2046.cpp`
   - `src/drv/touch/touch_driver_xpt2046.h`

2. **Modified for XPT2046 Support**
   - `src/drv/touch/touch_driver.h` - Added ArduinoGFX XPT2046 selection
   - `src/hasp_gui.cpp` - Added touch guard for no-touch builds
   - `src/drv/old/hasp_drv_touch.cpp` - Conditional XPT2046 handling

3. **Board Configuration**
   - `user_setups/esp32s3/sunton-esp32-s3-tft.ini` - Added 8048s043r environment

4. **Library Changes**
   - Renamed `lib/XPT2046_Touchscreen_ID542` → `lib/XPT2046_Touchscreen`
   - Updated `library.json` for proper PlatformIO integration

5. **Version & UI Fixes**
   - `platformio.ini` - Version 0.7.0.2-NopeNix
   - `data/edit.htm` - ACE editor 1.43.6 → 1.43.3
   - `data/script.js` - ACE editor 1.43.6 → 1.43.3

---

## Troubleshooting

### Touch Not Working
1. Check wiring: SCLK=12, MISO=13, MOSI=11, CS=38, IRQ=18
2. Calibrate XPT2046 values via web UI
3. Check serial logs for "XPT2046 started" message

### Build Fails
1. Ensure submodules are initialized: `git submodule update --init --recursive`
2. Check XPT2046 library is present in `lib/XPT2046_Touchscreen/`
3. Try Docker build for consistent environment

### Web Editor Not Saving
The ACE 1.43.6 CDN was broken. This fork uses 1.43.3 which works correctly.

---

## Original Project

This fork is based on [openHASP](https://github.com/HASwitchPlate/openHASP) by the HASwitchPlate team.

- Original documentation: https://www.openhasp.com/
- Discord support: https://www.openhasp.com/discord
- Original license: MIT

---

## Contributing

Contributions welcome! This fork focuses on:
- Sunton 8048S043R support
- XPT2046 resistive touch
- Docker-based builds
- Bug fixes for broken dependencies

---

## License

MIT License - See [LICENSE](LICENSE) file for details.

---

## Badges

[![GitHub Workflow Status](https://img.shields.io/badge/build-custom%20fork-blue)](https://github.com/nopenix/openHASP)
[![GitHub release](https://img.shields.io/badge/version-0.7.0.2--NopeNix-orange)](https://github.com/nopenix/openHASP/releases)
[![Target Device](https://img.shields.io/badge/device-Sunton%208048S043R-green)]()
[![Touch Controller](https://img.shields.io/badge/touch-XPT2046-yellow)]()
