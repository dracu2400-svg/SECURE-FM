#!/bin/bash
# TF-M Build Script for NUCLEO-U545RE-Q
# Lab 01 Solution - Automated Build Script

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
TFM_ROOT="${HOME}/tfm-nucleo-u545/trusted-firmware-m"
BUILD_DIR="${TFM_ROOT}/build_nucleo_u545"
PROFILE="${1:-profile_medium}"  # Default to medium profile

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}TF-M Build Script for NUCLEO-U545RE-Q${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Check if TF-M directory exists
if [ ! -d "$TFM_ROOT" ]; then
    echo -e "${RED}Error: TF-M directory not found at $TFM_ROOT${NC}"
    echo "Please clone TF-M first:"
    echo "  git clone https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git ~/tfm-nucleo-u545/trusted-firmware-m"
    exit 1
fi

# Check for ARM toolchain
if ! command -v arm-none-eabi-gcc &> /dev/null; then
    echo -e "${RED}Error: ARM toolchain not found in PATH${NC}"
    echo "Please install ARM GNU toolchain and add to PATH"
    exit 1
fi

echo -e "${YELLOW}ARM Toolchain:${NC}"
arm-none-eabi-gcc --version | head -1
echo ""

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${YELLOW}Configuring CMake...${NC}"
echo "Profile: $PROFILE"
echo ""

# Configure with CMake
cmake "$TFM_ROOT" \
    -G"Unix Makefiles" \
    -DTFM_PLATFORM=stm/nucleo_l552ze_q \
    -DTFM_TOOLCHAIN_FILE="${TFM_ROOT}/toolchain_GNUARM.cmake" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DTFM_ISOLATION_LEVEL=2 \
    -DTFM_PROFILE="$PROFILE" \
    -DTFM_PARTITION_CRYPTO=ON \
    -DTFM_PARTITION_INTERNAL_TRUSTED_STORAGE=ON \
    -DTFM_PARTITION_PROTECTED_STORAGE=ON \
    -DTFM_PARTITION_PLATFORM=ON \
    -DTFM_PARTITION_INITIAL_ATTESTATION=ON \
    -DTFM_PARTITION_FIRMWARE_UPDATE=ON \
    -DBL2=ON \
    -DMCUBOOT_IMAGE_NUMBER=1 \
    -DMCUBOOT_UPGRADE_STRATEGY=SWAP_USING_SCRATCH \
    -DMCUBOOT_HW_KEY=ON \
    -DCRYPTO_HW_ACCELERATOR=ON \
    -DPLATFORM_DEFAULT_UART_STDOUT=ON \
    -DTEST_S=ON \
    -DTEST_NS=ON

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

echo ""
echo -e "${YELLOW}Building TF-M...${NC}"
echo "This may take 3-7 minutes..."
echo ""

# Build
START_TIME=$(date +%s)
cmake --build . -- -j$(nproc)
END_TIME=$(date +%s)
BUILD_TIME=$((END_TIME - START_TIME))

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Build Successful!${NC}"
echo -e "${GREEN}========================================${NC}"
echo "Build time: ${BUILD_TIME} seconds"
echo ""

# Show binary sizes
echo -e "${YELLOW}Binary Sizes:${NC}"
echo ""
cd bin

if [ -f bl2.axf ]; then
    echo "MCUboot Bootloader (BL2):"
    arm-none-eabi-size bl2.axf
    echo ""
fi

if [ -f tfm_s.axf ]; then
    echo "TF-M Secure Firmware:"
    arm-none-eabi-size tfm_s.axf
    echo ""
fi

if [ -f tfm_ns.axf ]; then
    echo "Non-Secure Test Application:"
    arm-none-eabi-size tfm_ns.axf
    echo ""
fi

# List output files
echo -e "${YELLOW}Generated Files:${NC}"
ls -lh *.bin *.axf 2>/dev/null || true
echo ""

echo -e "${GREEN}Build artifacts are in:${NC}"
echo "$BUILD_DIR/bin"
echo ""
echo -e "${YELLOW}Next Steps:${NC}"
echo "1. Flash the binaries to NUCLEO-U545RE-Q:"
echo "   cd $BUILD_DIR/bin"
echo "   st-flash write bl2.bin 0x08000000"
echo "   st-flash write tfm_s_signed.bin 0x0800A000"
echo "   st-flash write tfm_ns_signed.bin 0x0803C000"
echo ""
echo "2. Or use the flash script:"
echo "   ./flash_tfm_nucleo_u545.sh"
echo ""
