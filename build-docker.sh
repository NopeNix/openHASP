#!/bin/bash
# Docker build script for openHASP Sunton 8048S043R

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== openHASP Docker Build Script ===${NC}"
echo ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo -e "${RED}Error: Docker is not installed${NC}"
    exit 1
fi

# Build Docker image if needed
echo -e "${YELLOW}Building Docker image...${NC}"
docker build -t openhasp-builder -f Dockerfile.platformio .

# Run build container
echo -e "${YELLOW}Starting build container...${NC}"
docker run -d --name openhasp-builder --privileged \
    -v "$(pwd):/workspace" \
    -v "openhasp-pio-cache:/root/.platformio" \
    -w /workspace \
    openhasp-builder tail -f /dev/null 2>/dev/null || true

# Clean previous build
echo -e "${YELLOW}Cleaning previous build...${NC}"
docker exec openhasp-builder pio run -e sunton-8048s043r_16MB --target clean 2>/dev/null || true

# Build firmware
echo -e "${YELLOW}Building firmware for sunton-8048s043r_16MB...${NC}"
docker exec -it openhasp-builder pio run -e sunton-8048s043r_16MB

# Show results
echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo "Firmware files:"
ls -lh build_output/firmware/sunton-8048s043r*.bin 2>/dev/null || echo "No output files found"
echo ""
echo -e "${YELLOW}To flash:${NC}"
echo "  esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x0 build_output/firmware/sunton-8048s043r_full_16MB_*.bin"
echo ""
