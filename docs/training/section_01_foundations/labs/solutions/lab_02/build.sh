#!/bin/bash
################################################################################
# Build Script for Lab 02 - TrustZone Basics
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
TARGET_MCU="STM32U545xx"
BOARD="NUCLEO-U545RE-Q"
BUILD_DIR="build"
SRC_DIR="src"

# Toolchain
ARM_TOOLCHAIN="arm-none-eabi"
CC="${ARM_TOOLCHAIN}-gcc"
OBJCOPY="${ARM_TOOLCHAIN}-objcopy"
SIZE="${ARM_TOOLCHAIN}-size"

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  Lab 02: TrustZone Basics - Build Script${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "Target:    ${GREEN}${BOARD}${NC}"
echo -e "MCU:       ${GREEN}${TARGET_MCU}${NC}"
echo -e "Toolchain: ${GREEN}${ARM_TOOLCHAIN}${NC}"
echo ""

# Check if toolchain is installed
if ! command -v ${CC} &> /dev/null; then
    echo -e "${RED}✗ Error: ARM GCC toolchain not found!${NC}"
    echo -e "${YELLOW}Please install: sudo apt-get install gcc-arm-none-eabi${NC}"
    exit 1
fi

echo -e "${GREEN}✓ ARM GCC toolchain found: $(${CC} --version | head -n1)${NC}"
echo ""

# Create build directory
echo -e "${BLUE}[1/6]${NC} Creating build directory..."
mkdir -p ${BUILD_DIR}
echo -e "${GREEN}✓ Build directory created${NC}"
echo ""

# Build Secure World
echo -e "${BLUE}[2/6]${NC} Building Secure world..."
echo "────────────────────────────────────────────────────────────────"

SECURE_CFLAGS="-mcpu=cortex-m33 \
               -mthumb \
               -mfloat-abi=hard \
               -mfpu=fpv5-sp-d16 \
               -mcmse \
               -Wall \
               -Wextra \
               -O2 \
               -g3 \
               -ffunction-sections \
               -fdata-sections \
               -DSTM32U545xx \
               -DUSE_HAL_DRIVER \
               -I./include \
               -I./CMSIS/Include \
               -I./CMSIS/Device/ST/STM32U5xx/Include \
               -I./HAL_Driver/Inc"

# Compile Secure files
echo "  Compiling main_s.c..."
${CC} ${SECURE_CFLAGS} -c ${SRC_DIR}/main_s.c -o ${BUILD_DIR}/main_s.o

echo "  Compiling nsc_functions.c..."
${CC} ${SECURE_CFLAGS} -c ${SRC_DIR}/nsc_functions.c -o ${BUILD_DIR}/nsc_functions.o

# Link Secure world
echo "  Linking Secure world..."
${CC} ${SECURE_CFLAGS} \
    -T linker/stm32u545_s.ld \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -Wl,-Map=${BUILD_DIR}/secure.map \
    -Wl,--cmse-implib \
    -Wl,--out-implib=${BUILD_DIR}/secure_nsclib.o \
    ${BUILD_DIR}/main_s.o \
    ${BUILD_DIR}/nsc_functions.o \
    -o ${BUILD_DIR}/secure.elf

# Generate Secure binary
${OBJCOPY} -O binary ${BUILD_DIR}/secure.elf ${BUILD_DIR}/secure.bin

echo -e "${GREEN}✓ Secure world built successfully${NC}"
${SIZE} ${BUILD_DIR}/secure.elf
echo ""

# Build Non-Secure World
echo -e "${BLUE}[3/6]${NC} Building Non-Secure world..."
echo "────────────────────────────────────────────────────────────────"

NS_CFLAGS="-mcpu=cortex-m33 \
           -mthumb \
           -mfloat-abi=hard \
           -mfpu=fpv5-sp-d16 \
           -Wall \
           -Wextra \
           -O2 \
           -g3 \
           -ffunction-sections \
           -fdata-sections \
           -DSTM32U545xx \
           -DUSE_HAL_DRIVER \
           -I./include \
           -I./CMSIS/Include \
           -I./CMSIS/Device/ST/STM32U5xx/Include \
           -I./HAL_Driver/Inc"

# Compile Non-Secure files
echo "  Compiling main_ns.c..."
${CC} ${NS_CFLAGS} -c ${SRC_DIR}/main_ns.c -o ${BUILD_DIR}/main_ns.o

# Link Non-Secure world (using import library from Secure build)
echo "  Linking Non-Secure world..."
${CC} ${NS_CFLAGS} \
    -T linker/stm32u545_ns.ld \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -Wl,-Map=${BUILD_DIR}/nonsecure.map \
    ${BUILD_DIR}/main_ns.o \
    ${BUILD_DIR}/secure_nsclib.o \
    -o ${BUILD_DIR}/nonsecure.elf

# Generate Non-Secure binary
${OBJCOPY} -O binary ${BUILD_DIR}/nonsecure.elf ${BUILD_DIR}/nonsecure.bin

echo -e "${GREEN}✓ Non-Secure world built successfully${NC}"
${SIZE} ${BUILD_DIR}/nonsecure.elf
echo ""

# Merge binaries
echo -e "${BLUE}[4/6]${NC} Merging Secure and Non-Secure images..."
echo "────────────────────────────────────────────────────────────────"

# Memory layout:
# 0x08000000 - 0x0803FFFF : Secure Flash (256KB)
# 0x0803E000 - 0x0803FFFF : NSC Region (8KB)
# 0x08040000 - 0x0807FFFF : Non-Secure Flash (256KB)

# Create combined binary
dd if=${BUILD_DIR}/secure.bin of=${BUILD_DIR}/${PROJECT_NAME}.bin bs=1k
dd if=${BUILD_DIR}/nonsecure.bin of=${BUILD_DIR}/${PROJECT_NAME}.bin bs=1k seek=256 conv=notrunc

echo -e "${GREEN}✓ Combined binary created: ${PROJECT_NAME}.bin${NC}"
ls -lh ${BUILD_DIR}/${PROJECT_NAME}.bin
echo ""

# Generate HEX file
echo -e "${BLUE}[5/6]${NC} Generating HEX file..."
${OBJCOPY} -O ihex ${BUILD_DIR}/${PROJECT_NAME}.bin ${BUILD_DIR}/${PROJECT_NAME}.hex
echo -e "${GREEN}✓ HEX file created: ${PROJECT_NAME}.hex${NC}"
echo ""

# Summary
echo -e "${BLUE}[6/6]${NC} Build Summary"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo -e "${GREEN}✓ Build completed successfully!${NC}"
echo ""
echo "Output files:"
echo "  • ${BUILD_DIR}/secure.elf       - Secure world ELF (debug)"
echo "  • ${BUILD_DIR}/nonsecure.elf    - Non-Secure world ELF (debug)"
echo "  • ${BUILD_DIR}/${PROJECT_NAME}.bin - Combined binary"
echo "  • ${BUILD_DIR}/${PROJECT_NAME}.hex - Combined HEX file"
echo ""
echo "Memory maps:"
echo "  • ${BUILD_DIR}/secure.map       - Secure memory layout"
echo "  • ${BUILD_DIR}/nonsecure.map    - Non-Secure memory layout"
echo ""
echo "Next steps:"
echo -e "  ${YELLOW}1.${NC} Flash to board:  ${GREEN}./flash.sh${NC}"
echo -e "  ${YELLOW}2.${NC} Open serial:     ${GREEN}screen /dev/ttyACM0 115200${NC}"
echo -e "  ${YELLOW}3.${NC} Press RESET button on NUCLEO board"
echo -e "  ${YELLOW}4.${NC} Observe LED patterns and serial output"
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
