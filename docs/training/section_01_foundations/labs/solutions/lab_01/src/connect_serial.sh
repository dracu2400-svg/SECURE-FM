#!/bin/bash
# Serial Console Connection Script for NUCLEO-U545RE-Q
# Lab 01 Solution - Easy UART connection

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}NUCLEO-U545RE-Q Serial Console${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Find the serial port
if [ -e /dev/ttyACM0 ]; then
    SERIAL_PORT="/dev/ttyACM0"
elif [ -e /dev/ttyACM1 ]; then
    SERIAL_PORT="/dev/ttyACM1"
elif [ -e /dev/ttyUSB0 ]; then
    SERIAL_PORT="/dev/ttyUSB0"
else
    echo -e "${RED}Error: No serial port found${NC}"
    echo ""
    echo "Available serial devices:"
    ls -l /dev/tty{ACM,USB}* 2>/dev/null || echo "  (none found)"
    echo ""
    echo "Troubleshooting:"
    echo "1. Ensure NUCLEO board is connected via USB"
    echo "2. Check dmesg for USB connection:"
    echo "   dmesg | grep tty | tail -5"
    echo "3. Check permissions:"
    echo "   ls -l /dev/ttyACM0"
    echo "4. Add yourself to dialout group:"
    echo "   sudo usermod -a -G dialout $USER"
    echo "   (then log out and back in)"
    exit 1
fi

echo -e "${YELLOW}Serial Port:${NC} $SERIAL_PORT"
echo -e "${YELLOW}Baud Rate:${NC} 115200"
echo -e "${YELLOW}Format:${NC} 8N1"
echo ""

# Check permissions
if [ ! -r "$SERIAL_PORT" ] || [ ! -w "$SERIAL_PORT" ]; then
    echo -e "${YELLOW}Warning: Permission denied on $SERIAL_PORT${NC}"
    echo "Running with sudo..."
    SUDO="sudo"
else
    SUDO=""
fi

# Choose terminal program
if command -v screen &> /dev/null; then
    echo -e "${GREEN}Using screen...${NC}"
    echo ""
    echo -e "${YELLOW}Controls:${NC}"
    echo "  Ctrl+A, K, Y  - Exit screen"
    echo "  Ctrl+A, [     - Enter scroll mode (use arrow keys, Enter to exit)"
    echo ""
    sleep 2
    $SUDO screen "$SERIAL_PORT" 115200
elif command -v minicom &> /dev/null; then
    echo -e "${GREEN}Using minicom...${NC}"
    echo ""
    echo -e "${YELLOW}Controls:${NC}"
    echo "  Ctrl+A, X     - Exit minicom"
    echo "  Ctrl+A, Z     - Help menu"
    echo ""
    sleep 2
    $SUDO minicom -D "$SERIAL_PORT" -b 115200
elif command -v picocom &> /dev/null; then
    echo -e "${GREEN}Using picocom...${NC}"
    echo ""
    echo -e "${YELLOW}Controls:${NC}"
    echo "  Ctrl+A, Ctrl+X - Exit picocom"
    echo ""
    sleep 2
    $SUDO picocom -b 115200 "$SERIAL_PORT"
else
    echo -e "${RED}Error: No terminal program found${NC}"
    echo "Please install one:"
    echo "  sudo apt-get install screen"
    echo "  # or"
    echo "  sudo apt-get install minicom"
    echo "  # or"
    echo "  sudo apt-get install picocom"
    exit 1
fi

echo ""
echo -e "${GREEN}Serial console disconnected${NC}"
