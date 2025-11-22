#!/bin/bash
# TF-M Flash Script for NUCLEO-U545RE-Q
# Lab 01 Solution - Automated Flash Script

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="${HOME}/tfm-nucleo-u545/trusted-firmware-m/build_nucleo_u545/bin"
ERASE_FIRST="${1:-yes}"  # Default to erasing flash first

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}TF-M Flash Script for NUCLEO-U545RE-Q${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Check for st-flash tool
if ! command -v st-flash &> /dev/null; then
    echo -e "${RED}Error: st-flash not found in PATH${NC}"
    echo "Please install stlink tools:"
    echo "  sudo apt-get install stlink-tools"
    exit 1
fi

echo -e "${YELLOW}ST-Link version:${NC}"
st-flash --version
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found: $BUILD_DIR${NC}"
    echo "Please build TF-M first:"
    echo "  ./build_tfm_nucleo_u545.sh"
    exit 1
fi

cd "$BUILD_DIR"

# Check for required binary files
if [ ! -f bl2.bin ] || [ ! -f tfm_s_signed.bin ] || [ ! -f tfm_ns_signed.bin ]; then
    echo -e "${RED}Error: Required binary files not found in $BUILD_DIR${NC}"
    echo "Expected files:"
    echo "  - bl2.bin"
    echo "  - tfm_s_signed.bin"
    echo "  - tfm_ns_signed.bin"
    exit 1
fi

# Check file sizes
echo -e "${YELLOW}Binary files to flash:${NC}"
echo "BL2 (MCUboot):        $(du -h bl2.bin | cut -f1) (max 40 KB)"
echo "Secure firmware:      $(du -h tfm_s_signed.bin | cut -f1) (max 200 KB)"
echo "Non-secure app:       $(du -h tfm_ns_signed.bin | cut -f1) (max 256 KB)"
echo ""

# Probe ST-Link
echo -e "${YELLOW}Probing ST-Link...${NC}"
if ! st-info --probe > /dev/null 2>&1; then
    echo -e "${RED}Error: Cannot detect ST-Link${NC}"
    echo ""
    echo "Troubleshooting steps:"
    echo "1. Ensure NUCLEO-U545RE-Q is connected via USB"
    echo "2. Check that LED LD1 (power) is on"
    echo "3. Try with sudo: sudo st-info --probe"
    echo "4. Check udev rules in /etc/udev/rules.d/99-stlink.rules"
    exit 1
fi

st-info --probe
echo ""

# Erase flash if requested
if [ "$ERASE_FIRST" == "yes" ]; then
    echo -e "${YELLOW}Erasing flash memory...${NC}"
    st-flash erase
    echo ""
    sleep 1
fi

# Flash BL2 (MCUboot bootloader)
echo -e "${BLUE}[1/3] Flashing BL2 (MCUboot) to 0x08000000...${NC}"
st-flash write bl2.bin 0x08000000
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to flash BL2!${NC}"
    exit 1
fi
echo -e "${GREEN}✓ BL2 flashed successfully${NC}"
echo ""
sleep 1

# Flash TF-M Secure
echo -e "${BLUE}[2/3] Flashing TF-M Secure to 0x0800A000...${NC}"
st-flash write tfm_s_signed.bin 0x0800A000
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to flash TF-M Secure!${NC}"
    exit 1
fi
echo -e "${GREEN}✓ TF-M Secure flashed successfully${NC}"
echo ""
sleep 1

# Flash Non-Secure Application
echo -e "${BLUE}[3/3] Flashing Non-Secure App to 0x0803C000...${NC}"
st-flash write tfm_ns_signed.bin 0x0803C000
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to flash Non-Secure application!${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Non-Secure App flashed successfully${NC}"
echo ""

# Reset board
echo -e "${YELLOW}Resetting board...${NC}"
st-flash reset
echo ""

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Flashing Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${YELLOW}Flash Memory Layout:${NC}"
echo "0x0800_0000 - 0x0800_9FFF : BL2 (MCUboot)         [40 KB]"
echo "0x0800_A000 - 0x0803_BFFF : TF-M Secure           [200 KB]"
echo "0x0803_C000 - 0x0807_BFFF : Non-Secure App        [256 KB]"
echo ""
echo -e "${YELLOW}Next Steps:${NC}"
echo "1. Connect to serial console:"
echo "   screen /dev/ttyACM0 115200"
echo "   # or"
echo "   minicom -D /dev/ttyACM0 -b 115200"
echo ""
echo "2. Press RESET button (B2) on NUCLEO board"
echo ""
echo "3. You should see boot messages:"
echo "   [INF] Starting bootloader"
echo "   [INF] Jumping to the first image slot"
echo "   [Sec Thread] Secure image initializing!"
echo "   Booting TFM v2.1.0"
echo ""
echo -e "${GREEN}Lab 01 Complete!${NC}"
echo ""
