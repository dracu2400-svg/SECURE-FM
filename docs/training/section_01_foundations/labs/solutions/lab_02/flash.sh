#!/bin/bash
################################################################################
# Flash Script for Lab 02 - TrustZone Basics
# Hardware: NUCLEO-U545RE-Q (STM32U545RET6Q)
################################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="lab_02_trustzone_basics"
BUILD_DIR="build"
BINARY="${BUILD_DIR}/${PROJECT_NAME}.bin"
FLASH_ADDRESS="0x08000000"

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Lab 02: TrustZone Basics - Flash Script${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Check if binary exists
if [ ! -f "${BINARY}" ]; then
    echo -e "${RED}✗ Error: Binary not found: ${BINARY}${NC}"
    echo -e "${YELLOW}Please run ./build.sh first${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Binary found: ${BINARY}${NC}"
ls -lh ${BINARY}
echo ""

# Detect programmer
echo -e "${BLUE}[1/4]${NC} Detecting programmer..."
echo "────────────────────────────────────────────────────────────────"

PROGRAMMER=""

# Check for ST-LINK (via st-flash)
if command -v st-flash &> /dev/null; then
    PROGRAMMER="st-flash"
    echo -e "${GREEN}✓ Found ST-LINK programmer (st-flash)${NC}"
elif command -v STM32_Programmer_CLI &> /dev/null; then
    PROGRAMMER="stm32cubeprog"
    echo -e "${GREEN}✓ Found STM32CubeProgrammer CLI${NC}"
elif command -v openocd &> /dev/null; then
    PROGRAMMER="openocd"
    echo -e "${GREEN}✓ Found OpenOCD${NC}"
else
    echo -e "${RED}✗ No programmer found!${NC}"
    echo ""
    echo "Please install one of the following:"
    echo "  • ST-LINK Tools:      sudo apt-get install stlink-tools"
    echo "  • STM32CubeProg:      https://www.st.com/en/development-tools/stm32cubeprog.html"
    echo "  • OpenOCD:            sudo apt-get install openocd"
    exit 1
fi

echo ""

# Flash based on available programmer
case ${PROGRAMMER} in
    st-flash)
        echo -e "${BLUE}[2/4]${NC} Erasing flash..."
        st-flash erase
        echo ""

        echo -e "${BLUE}[3/4]${NC} Programming flash..."
        st-flash --reset write ${BINARY} ${FLASH_ADDRESS}
        echo ""

        echo -e "${BLUE}[4/4]${NC} Verifying flash..."
        echo -e "${GREEN}✓ Flash programming complete${NC}"
        ;;

    stm32cubeprog)
        echo -e "${BLUE}[2/4]${NC} Connecting to ST-LINK..."
        STM32_Programmer_CLI -c port=SWD -w ${BINARY} ${FLASH_ADDRESS} -v -rst
        echo ""

        echo -e "${GREEN}✓ Flash programming complete${NC}"
        ;;

    openocd)
        echo -e "${BLUE}[2/4]${NC} Programming with OpenOCD..."

        # Create temporary OpenOCD config
        cat > /tmp/flash_lab02.cfg <<EOF
source [find interface/stlink.cfg]
source [find target/stm32u5x.cfg]

init
reset halt
flash write_image erase ${BINARY} ${FLASH_ADDRESS}
verify_image ${BINARY} ${FLASH_ADDRESS}
reset run
shutdown
EOF

        openocd -f /tmp/flash_lab02.cfg
        rm /tmp/flash_lab02.cfg

        echo ""
        echo -e "${GREEN}✓ Flash programming complete${NC}"
        ;;
esac

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}  ✓ Lab 02 flashed successfully to NUCLEO-U545RE-Q${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo "What to do next:"
echo ""
echo -e "${YELLOW}1. Connect Serial Terminal:${NC}"
echo "   screen /dev/ttyACM0 115200"
echo "   OR"
echo "   minicom -D /dev/ttyACM0 -b 115200"
echo ""
echo -e "${YELLOW}2. Press RESET button${NC} on the NUCLEO board"
echo ""
echo -e "${YELLOW}3. Observe the LED patterns:${NC}"
echo "   • LD1 (Green)  = Secure world execution"
echo "   • LD2 (Blue)   = Non-Secure world execution"
echo "   • LD3 (Red)    = Security fault detected"
echo ""
echo -e "${YELLOW}4. Interact via serial console:${NC}"
echo "   • Press User Button (B1) to trigger context switches"
echo "   • Type commands:"
echo "     - help              Show available commands"
echo "     - test_violation    Trigger security fault"
echo "     - show_memory       Display memory layout"
echo "     - call_secure       Call secure function"
echo "     - read_counter      Read secure counter"
echo "     - increment         Increment counter"
echo "     - hash_test         Test secure hashing"
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${GREEN}Happy TrustZone exploring! 🔒${NC}"
echo ""
