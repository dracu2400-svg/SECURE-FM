# Lab 01 Solution: Environment Setup and First Build for NUCLEO-U545RE-Q

**Target Hardware:** NUCLEO-U545RE-Q (STM32U545RET6Q)
**Duration:** 60 minutes
**Difficulty:** Beginner

---

## 📋 Solution Overview

This solution guide provides step-by-step instructions for:
- Setting up the TF-M development environment
- Building TF-M specifically for NUCLEO-U545RE-Q
- Flashing and running TF-M on real hardware
- Verifying successful boot and secure operation

---

## 🎯 Learning Outcomes

After completing this lab, you will understand:
- ✅ TF-M toolchain installation and configuration
- ✅ STM32U5 platform-specific build configuration
- ✅ TF-M build system (CMake) usage
- ✅ Flash memory layout for STM32U545
- ✅ How to flash and debug TF-M on NUCLEO-U545RE-Q
- ✅ Serial console connection and boot verification

---

## 🔧 Hardware Requirements

**Required:**
- NUCLEO-U545RE-Q development board
- USB Mini-B cable (for ST-Link and power)
- Linux development machine (Ubuntu 20.04+ recommended)

**Board Specifications:**
- MCU: STM32U545RET6Q
- Core: ARM Cortex-M33 with TrustZone
- Flash: 512 KB
- RAM: 256 KB
- On-board ST-Link V3 debugger

---

## 📦 Step 1: Install Development Tools

### 1.1 Install System Dependencies

```bash
# Update package list
sudo apt-get update

# Install build essentials
sudo apt-get install -y \
    build-essential \
    git \
    cmake \
    python3 \
    python3-pip \
    ninja-build \
    libssl-dev \
    wget \
    curl \
    libusb-1.0-0-dev \
    pkg-config

# Verify installations
git --version          # Should be 2.25+
cmake --version        # Should be 3.21+
python3 --version      # Should be 3.8+
```

**Expected Output:**
```
git version 2.34.1
cmake version 3.22.1
Python 3.10.6
```

### 1.2 Install ARM GNU Toolchain (Version 13.2)

```bash
# Create tools directory
mkdir -p ~/tools
cd ~/tools

# Download ARM GNU toolchain
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz

# Extract to /opt
sudo mkdir -p /opt/arm-gnu-toolchain
sudo tar xf arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz -C /opt/

# Add to PATH (add to ~/.bashrc for persistence)
export PATH=$PATH:/opt/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi/bin

# Make permanent
echo 'export PATH=$PATH:/opt/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi/bin' >> ~/.bashrc

# Verify
arm-none-eabi-gcc --version
```

**Expected Output:**
```
arm-none-eabi-gcc (Arm GNU Toolchain 13.2.rel1) 13.2.1 20231009
Copyright (C) 2023 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.
```

### 1.3 Install STM32 Tools

```bash
# Install stlink tools for flashing/debugging
sudo apt-get install -y stlink-tools gdb-multiarch

# Verify st-flash
st-flash --version

# Verify st-info
st-info --version
```

**Expected Output:**
```
v1.7.0
```

---

## 📥 Step 2: Clone TF-M Repository and Dependencies

### 2.1 Create Workspace

```bash
# Create TF-M workspace
mkdir -p ~/tfm-nucleo-u545
cd ~/tfm-nucleo-u545

# Clone TF-M main repository
git clone https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git
cd trusted-firmware-m

# Checkout stable version (TF-M v2.1.0)
git checkout v2.1.0

# Check current version
git describe --tags
```

### 2.2 Clone MCUboot (Bootloader)

```bash
# MCUboot is needed for secure boot
cd ~/tfm-nucleo-u545
git clone https://github.com/mcu-tools/mcuboot.git
cd mcuboot
git checkout v2.1.0
```

### 2.3 Clone mbedTLS (Cryptography Library)

```bash
cd ~/tfm-nucleo-u545
git clone https://github.com/Mbed-TLS/mbedtls.git
cd mbedtls
git checkout v3.5.1
```

**Directory Structure After Clone:**
```
~/tfm-nucleo-u545/
├── trusted-firmware-m/    (TF-M main repository)
├── mcuboot/               (Secure bootloader)
└── mbedtls/               (Crypto library)
```

---

## 🐍 Step 3: Install Python Dependencies

### 3.1 Upgrade pip

```bash
pip3 install --user --upgrade pip setuptools wheel
```

### 3.2 Install TF-M Requirements

```bash
cd ~/tfm-nucleo-u545/trusted-firmware-m

# Install Python dependencies
pip3 install --user -r tools/requirements.txt

# Install additional tools for STM32
pip3 install --user pyocd intelhex
```

### 3.3 Verify Python Packages

```bash
# Verify critical packages
python3 -c "import cryptography; print('cryptography:', cryptography.__version__)"
python3 -c "import cbor2; print('cbor2:', cbor2.__version__)"
python3 -c "import click; print('click:', click.__version__)"
python3 -c "import pyocd; print('pyocd:', pyocd.__version__)"

# Verify imgtool (MCUboot image signing tool)
python3 -c "import imgtool.main; print('imgtool: OK')"
```

**Expected Output:**
```
cryptography: 41.0.7
cbor2: 5.6.2
click: 8.1.7
pyocd: 0.36.0
imgtool: OK
```

---

## 🔨 Step 4: Configure TF-M Build for STM32U545

### 4.1 Understand STM32U545 Memory Layout

The STM32U545RET6Q has the following memory configuration:

```
Flash Memory (512 KB):
┌─────────────────────────────────────┐ 0x0800_0000
│  BL2 (MCUboot Bootloader)    40 KB  │
├─────────────────────────────────────┤ 0x0800_A000
│  Primary Slot - Secure       200 KB │ (TF-M Secure)
├─────────────────────────────────────┤ 0x0803_C000
│  Primary Slot - Non-Secure   256 KB │ (Application)
├─────────────────────────────────────┤ 0x0807_C000
│  Secondary Slot - Secure     200 KB │ (OTA Update)
├─────────────────────────────────────┤ 0x080A_E000
│  Secondary Slot - Non-Secure 256 KB │ (OTA Update)
├─────────────────────────────────────┤ 0x080E_E000
│  Scratch Area                 32 KB │ (Swap buffer)
├─────────────────────────────────────┤ 0x080F_6000
│  ITS (Internal Trusted Store) 16 KB │
├─────────────────────────────────────┤ 0x080F_A000
│  PS (Protected Storage)       16 KB │
├─────────────────────────────────────┤ 0x080F_E000
│  NV Counters                   8 KB │
└─────────────────────────────────────┘ 0x0810_0000

RAM Memory (256 KB):
┌─────────────────────────────────────┐ 0x2000_0000
│  Secure RAM                  128 KB │
├─────────────────────────────────────┤ 0x2002_0000
│  Non-Secure RAM              128 KB │
└─────────────────────────────────────┘ 0x2004_0000
```

### 4.2 Create Build Configuration Script

```bash
# Navigate to TF-M directory
cd ~/tfm-nucleo-u545/trusted-firmware-m

# Create build directory
mkdir build_nucleo_u545 && cd build_nucleo_u545
```

### 4.3 Configure CMake for NUCLEO-U545RE-Q

```bash
cmake .. \
    -G"Unix Makefiles" \
    -DTFM_PLATFORM=stm/nucleo_l552ze_q \
    -DTFM_TOOLCHAIN_FILE=../toolchain_GNUARM.cmake \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DTFM_ISOLATION_LEVEL=2 \
    -DTFM_PROFILE=profile_medium \
    -DTFM_PARTITION_CRYPTO=ON \
    -DTFM_PARTITION_INTERNAL_TRUSTED_STORAGE=ON \
    -DTFM_PARTITION_PROTECTED_STORAGE=ON \
    -DTFM_PARTITION_PLATFORM=ON \
    -DTFM_PARTITION_INITIAL_ATTESTATION=ON \
    -DBL2=ON \
    -DMCUBOOT_IMAGE_NUMBER=1 \
    -DMCUBOOT_UPGRADE_STRATEGY=SWAP_USING_SCRATCH \
    -DMCUBOOT_HW_KEY=ON \
    -DCRYPTO_HW_ACCELERATOR=ON \
    -DPLATFORM_DEFAULT_UART_STDOUT=ON
```

**Configuration Explanation:**

| Parameter | Value | Description |
|-----------|-------|-------------|
| `TFM_PLATFORM` | `stm/nucleo_l552ze_q` | STM32 platform (note: use L552 as template for U545) |
| `TFM_TOOLCHAIN_FILE` | `toolchain_GNUARM.cmake` | Use ARM GCC toolchain |
| `CMAKE_BUILD_TYPE` | `RelWithDebInfo` | Optimized with debug symbols |
| `TFM_ISOLATION_LEVEL` | `2` | Level 2 isolation (PSA Level 2) |
| `TFM_PROFILE` | `profile_medium` | Balanced security/size profile |
| `TFM_PARTITION_CRYPTO` | `ON` | Enable PSA Crypto service |
| `TFM_PARTITION_INTERNAL_TRUSTED_STORAGE` | `ON` | Enable ITS |
| `TFM_PARTITION_PROTECTED_STORAGE` | `ON` | Enable PS |
| `TFM_PARTITION_INITIAL_ATTESTATION` | `ON` | Enable attestation |
| `BL2` | `ON` | Enable MCUboot bootloader |
| `MCUBOOT_IMAGE_NUMBER` | `1` | Single image mode |
| `MCUBOOT_UPGRADE_STRATEGY` | `SWAP_USING_SCRATCH` | Use scratch area for updates |
| `CRYPTO_HW_ACCELERATOR` | `ON` | Use STM32 crypto hardware |

---

## 🏗️ Step 5: Build TF-M

### 5.1 Build All Components

```bash
# Build using all CPU cores
cmake --build . -- -j$(nproc)
```

**Build Time:** 3-7 minutes (depending on CPU)

**Expected Build Output:**
```
[  1%] Building C object bl2/ext/mcuboot/CMakeFiles/bl2_mcuboot.dir/bootutil/src/boot_record.c.obj
[  2%] Building C object bl2/ext/mcuboot/CMakeFiles/bl2_mcuboot.dir/bootutil/src/bootutil_misc.c.obj
...
[ 12%] Linking C executable bl2.axf
[ 12%] Built target bl2
...
[ 45%] Building C object secure_fw/CMakeFiles/tfm_s.dir/spm/core/tfm_boot_data.c.obj
[ 56%] Building C object secure_fw/CMakeFiles/tfm_s.dir/spm/core/tfm_core_utils.c.obj
...
[ 78%] Linking C executable tfm_s.axf
[ 78%] Built target tfm_s
...
[ 89%] Signing secure image with ECDSA-P256
[ 89%] Built target tfm_s_signed_bin
...
[100%] Linking C executable tfm_ns.axf
[100%] Signing non-secure image
[100%] Built target tfm_ns_signed_bin
```

### 5.2 Verify Build Artifacts

```bash
# Navigate to output directory
cd bin

# List generated files
ls -lh

# Key files generated:
# bl2.axf              - MCUboot bootloader (ELF with symbols)
# bl2.bin              - MCUboot bootloader (raw binary)
# tfm_s.axf            - TF-M Secure firmware (ELF with symbols)
# tfm_s_signed.bin     - Signed secure firmware
# tfm_ns.axf           - Non-secure application (ELF)
# tfm_ns_signed.bin    - Signed non-secure application
```

### 5.3 Check Memory Usage

```bash
# Check bootloader size
arm-none-eabi-size bl2.axf

# Check secure firmware size
arm-none-eabi-size tfm_s.axf

# Check non-secure size (test application)
arm-none-eabi-size tfm_ns.axf
```

**Expected Memory Usage (Approximate):**
```
   text    data     bss     dec     hex filename
  38456    2148    8192   48796    bead bl2.axf
  76234    3456   16384   96074   176fa tfm_s.axf
   8912     512    2048   11472    2cd0 tfm_ns.axf
```

**Memory Usage Breakdown:**
- **text**: Code in flash (instructions)
- **data**: Initialized data in flash (copied to RAM at boot)
- **bss**: Uninitialized data in RAM (zero-initialized at boot)
- **dec**: Total size in decimal bytes
- **hex**: Total size in hexadecimal

---

## 🔌 Step 6: Connect NUCLEO-U545RE-Q Hardware

### 6.1 Board Setup

1. **Connect USB cable** to NUCLEO-U545RE-Q board:
   - Connect USB Mini-B cable to the ST-Link connector (CN1)
   - Connect other end to your Linux machine
   - LED LD1 (power) should illuminate

2. **Verify ST-Link Detection:**

```bash
# Check ST-Link connection
st-info --probe

# Expected output:
# Found 1 stlink programmers
#   version:    V3J10M3
#   serial:     066DFF505254877067094816
#   flash:      524288 (pagesize: 2048)
#   sram:       262144
#   chipid:     0x0482
#   dev-type:   STM32U5xx_U545
```

3. **Configure udev Rules (if permission denied):**

```bash
# Create udev rule for ST-Link
sudo tee /etc/udev/rules.d/99-stlink.rules > /dev/null <<EOF
# ST-Link V2/V3
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3752", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374e", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374f", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3753", MODE="0666"
EOF

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Reconnect USB cable
```

---

## 📤 Step 7: Flash TF-M to NUCLEO-U545RE-Q

### 7.1 Erase Flash (Optional but Recommended)

```bash
# Erase entire flash
st-flash erase
```

**Expected Output:**
```
st-flash 1.7.0
2024-01-15T10:30:45 INFO common.c: STM32U5xx_U545: 512 KiB SRAM, 524 KiB flash in at least 8 KiB pages.
Mass erasing...
```

### 7.2 Flash MCUboot Bootloader (BL2)

```bash
# Flash BL2 to address 0x08000000
cd ~/tfm-nucleo-u545/trusted-firmware-m/build_nucleo_u545/bin

st-flash write bl2.bin 0x08000000
```

**Expected Output:**
```
st-flash 1.7.0
2024-01-15T10:31:12 INFO common.c: STM32U5xx_U545: 512 KiB SRAM, 524 KiB flash
2024-01-15T10:31:12 INFO common.c: Flash page at addr 0x08000000 erased
2024-01-15T10:31:13 INFO common.c: Finished erasing 20 pages
2024-01-15T10:31:13 INFO common.c: Starting Flash write for F4
2024-01-15T10:31:13 INFO flash_loader.c: Successfully loaded flash loader in sram
2024-01-15T10:31:14 INFO flash_loader.c: Flash written and verified! jolly good!
```

### 7.3 Flash Secure Firmware

```bash
# Flash TF-M Secure to primary slot at 0x0800A000
st-flash write tfm_s_signed.bin 0x0800A000
```

### 7.4 Flash Non-Secure Application

```bash
# Flash application to primary slot at 0x0803C000
st-flash write tfm_ns_signed.bin 0x0803C000
```

### 7.5 Reset Board

```bash
# Reset the target
st-flash reset
```

---

## 📡 Step 8: Connect to Serial Console

### 8.1 Identify Serial Port

The NUCLEO-U545RE-Q has a virtual COM port via ST-Link:

```bash
# Find the serial port
ls /dev/ttyACM*

# Should show: /dev/ttyACM0 (or similar)

# Check port details
dmesg | grep tty | tail -5
```

### 8.2 Connect with Screen or Minicom

**Option 1: Using screen**
```bash
# Connect to UART (115200 baud, 8N1)
screen /dev/ttyACM0 115200

# To exit: Ctrl+A, then K, then Y
```

**Option 2: Using minicom**
```bash
# Install minicom if not present
sudo apt-get install -y minicom

# Configure and connect
minicom -D /dev/ttyACM0 -b 115200

# To exit: Ctrl+A, then X
```

### 8.3 Reset Board and Observe Boot

Press the **RESET button (B2)** on the NUCLEO board.

---

## ✅ Step 9: Verify TF-M Boot Output

### 9.1 Expected Console Output

After reset, you should see the following boot sequence:

```
[INF] Starting bootloader
[INF] Beginning BL2 provisioning
[WRN] TFM_DUMMY_PROVISIONING is not suitable for production!
[INF] Swap type: none
[INF] Image index: 0, Swap type: none
[INF] Bootloader chainload address offset: 0xa000
[INF] Jumping to the first image slot
[Sec Thread] Secure image initializing!
TF-M isolation level is: 0x00000002
Booting TFM v2.1.0

#### Execute test suites for the Secure area ####
Running Test Suite PSA protected storage S interface tests (TFM_PS_TEST_2XXX)...

> Executing 'TFM_PS_TEST_2001'
  Description: 'Set interface'
  TEST: TFM_PS_TEST_2001 - PASSED!

> Executing 'TFM_PS_TEST_2002'
  Description: 'Get interface'
  TEST: TFM_PS_TEST_2002 - PASSED!

...

> Executing 'TFM_CRYPTO_TEST_6001'
  Description: 'Secure Hash SHA256 test'
  TEST: TFM_CRYPTO_TEST_6001 - PASSED!

...

*** Secure test suites summary ***
Test suite 'PSA protected storage S interface tests' has PASSED
Test suite 'PSA internal trusted storage S interface tests' has PASSED
Test suite 'PSA Crypto interface tests' has PASSED

*** End of Secure test suites ***
```

### 9.2 Boot Sequence Explanation

**1. MCUboot Bootloader (BL2) Stage:**
```
[INF] Starting bootloader
[INF] Beginning BL2 provisioning
```
- MCUboot starts at reset
- Provisions dummy keys (development mode)
- Verifies image signatures

**2. Image Verification:**
```
[INF] Swap type: none
[INF] Image index: 0, Swap type: none
```
- Checks primary and secondary slots
- No pending OTA update (swap type = none)
- Images validated successfully

**3. Chainload to TF-M:**
```
[INF] Bootloader chainload address offset: 0xa000
[INF] Jumping to the first image slot
```
- MCUboot transfers control to TF-M Secure at 0x0800A000
- TrustZone is configured
- Secure firmware starts

**4. TF-M Initialization:**
```
[Sec Thread] Secure image initializing!
TF-M isolation level is: 0x00000002
Booting TFM v2.1.0
```
- SPM (Secure Partition Manager) initializes
- Isolation Level 2 confirmed (PSA Level 2)
- All secure partitions start

**5. Test Execution:**
```
Running Test Suite PSA protected storage S interface tests...
TEST: TFM_PS_TEST_2001 - PASSED!
```
- Automated tests verify each PSA service
- Protected Storage, ITS, Crypto all tested
- All tests should PASS

---

## 🔍 Step 10: Verification and Analysis

### 10.1 Success Criteria Checklist

Verify the following:

- [x] **Build completed without errors**
- [x] **Binary files generated** (bl2.bin, tfm_s_signed.bin, tfm_ns_signed.bin)
- [x] **ST-Link detected** by st-info --probe
- [x] **Flash programming successful** (all three images)
- [x] **Serial console connected** at 115200 baud
- [x] **Boot messages appear** on serial console
- [x] **MCUboot starts** and verifies images
- [x] **TF-M boots** and shows isolation level 2
- [x] **All PSA tests PASS** (Storage, Crypto, etc.)

### 10.2 Understanding the Build Artifacts

```bash
# Return to build directory
cd ~/tfm-nucleo-u545/trusted-firmware-m/build_nucleo_u545

# Examine ELF sections
arm-none-eabi-objdump -h bin/tfm_s.axf | less

# View symbol table
arm-none-eabi-nm bin/tfm_s.axf | grep psa_

# Disassemble specific function (e.g., psa_hash_compute)
arm-none-eabi-objdump -d bin/tfm_s.axf | grep -A 50 "psa_hash_compute"
```

### 10.3 Flash Memory Verification

```bash
# Read back bootloader area
st-flash read bl2_readback.bin 0x08000000 0xA000

# Verify it matches original
cmp bl2.bin bl2_readback.bin
# Should output nothing (files identical)
```

---

## 🐛 Troubleshooting

### Problem 1: ST-Link Not Detected

**Symptoms:**
```
Error: No ST-Link detected
```

**Solutions:**
```bash
# 1. Check USB connection
lsusb | grep STMicro

# 2. Verify udev rules
cat /etc/udev/rules.d/99-stlink.rules

# 3. Reload udev
sudo udevadm control --reload-rules
sudo udevadm trigger

# 4. Try with sudo (if udev rules not working)
sudo st-info --probe
```

### Problem 2: Build Fails - Platform Not Found

**Symptoms:**
```
CMake Error: TFM_PLATFORM 'stm/nucleo_u545re_q' not found
```

**Solution:**
The STM32U545 platform support may not be in upstream TF-M yet. Use STM32L552 as a template:

```bash
# Use L552 platform (similar Cortex-M33 with TrustZone)
cmake .. -DTFM_PLATFORM=stm/nucleo_l552ze_q ...
```

Alternatively, create custom platform configuration (covered in Lab 07).

### Problem 3: Flash Write Fails

**Symptoms:**
```
Error: Flash loader run error
```

**Solutions:**
```bash
# 1. Erase flash completely first
st-flash erase

# 2. Try slower flash speed (not available in st-flash, use openocd)

# 3. Check board power (ensure LED LD1 is on)

# 4. Try resetting before flash
st-flash reset
sleep 1
st-flash write bl2.bin 0x08000000
```

### Problem 4: No Serial Output

**Symptoms:**
- Serial port opens but no text appears
- Blank screen after reset

**Solutions:**
```bash
# 1. Verify baud rate
minicom -D /dev/ttyACM0 -b 115200
# Press Ctrl+A, then Z, then O to configure

# 2. Check that PLATFORM_DEFAULT_UART_STDOUT=ON was set in CMake

# 3. Rebuild with UART debugging enabled
cmake .. -DPLATFORM_DEFAULT_UART_STDOUT=ON ...

# 4. Verify correct serial port
dmesg | grep tty
# Should show ttyACM0 when board is connected
```

### Problem 5: Tests Fail

**Symptoms:**
```
TEST: TFM_PS_TEST_2001 - FAILED!
```

**Solutions:**
```bash
# 1. Ensure all three images were flashed correctly
st-flash read bl2_check.bin 0x08000000 0xA000
ls -lh bl2_check.bin  # Should be ~40KB

# 2. Try erasing and reflashing
st-flash erase
st-flash write bl2.bin 0x08000000
st-flash write tfm_s_signed.bin 0x0800A000
st-flash write tfm_ns_signed.bin 0x0803C000

# 3. Rebuild with tests disabled (just boot check)
cmake .. -DTEST_S=OFF -DTEST_NS=OFF ...
```

---

## 📚 Understanding the Output

### Boot Flow Diagram

```
Power-On Reset
     |
     v
┌─────────────────────┐
│   MCUboot (BL2)     │ Address: 0x0800_0000
│   - Verify images   │ Size: 40 KB
│   - Check swap      │
│   - Set TrustZone   │
└─────────────────────┘
     |
     | (Chainload)
     v
┌─────────────────────┐
│   TF-M Secure (SPM) │ Address: 0x0800_A000
│   - Init partitions │ Size: ~200 KB
│   - Start services  │
│   - Crypto, ITS, PS │
└─────────────────────┘
     |
     | (NSPE entry)
     v
┌─────────────────────┐
│   Non-Secure App    │ Address: 0x0803_C000
│   - Test suites     │ Size: ~256 KB
│   - PSA API calls   │
└─────────────────────┘
```

### TrustZone Memory Attribution

```
SAU (Security Attribution Unit) Configuration:

Region 0: 0x0800_0000 - 0x0803_BFFF (Secure Code)
Region 1: 0x2000_0000 - 0x2001_FFFF (Secure RAM)
Region 2: 0x0803_C000 - 0x080F_FFFF (Non-Secure Code)
Region 3: 0x2002_0000 - 0x2003_FFFF (Non-Secure RAM)

IDAU (Implementation Defined Attribution Unit):
- Enforces STM32U5 security policy
- Watermark-based Flash attribution
- SRAM1/SRAM2 secure/non-secure split
```

---

## 🎓 Challenge Exercises

### Challenge 1: Build Profile Comparison

Build TF-M with different profiles and compare sizes:

```bash
# Build with profile_small
mkdir build_small && cd build_small
cmake .. -DTFM_PROFILE=profile_small ...
cmake --build . -- -j$(nproc)
arm-none-eabi-size bin/tfm_s.axf

# Build with profile_medium
mkdir ../build_medium && cd ../build_medium
cmake .. -DTFM_PROFILE=profile_medium ...
cmake --build . -- -j$(nproc)
arm-none-eabi-size bin/tfm_s.axf

# Build with profile_large
mkdir ../build_large && cd ../build_large
cmake .. -DTFM_PROFILE=profile_large ...
cmake --build . -- -j$(nproc)
arm-none-eabi-size bin/tfm_s.axf
```

**Document:**
- Size differences (text, data, bss)
- Features enabled/disabled in each profile
- Boot time differences (if measurable)

### Challenge 2: Disable Hardware Crypto Acceleration

Compare performance with/without STM32 crypto hardware:

```bash
# Build without hardware crypto
cmake .. -DCRYPTO_HW_ACCELERATOR=OFF ...
cmake --build . -- -j$(nproc)

# Flash and observe test execution time
# Note: Crypto tests will take significantly longer
```

### Challenge 3: Explore CMake Configuration Options

```bash
# Use ccmake to interactively explore options
cd build_nucleo_u545
ccmake ..

# Navigate with arrow keys, toggle options with Enter
# Press 'c' to configure, 'g' to generate
# Experiment with different isolation levels, partition selections
```

### Challenge 4: Measure Boot Time

Add timing measurements to understand boot performance:

```bash
# Measure from reset to "Booting TFM" message
# Use logic analyzer on GPIO pin toggled at boot stages
# Or use serial timestamp capture with Python script
```

---

## 📊 Lab Summary

### What You Accomplished

✅ **Installed complete TF-M toolchain** for STM32U5
✅ **Configured and built TF-M** for NUCLEO-U545RE-Q
✅ **Flashed MCUboot + TF-M** to real hardware
✅ **Verified secure boot** chain operation
✅ **Tested PSA services** (Crypto, Storage)
✅ **Understood memory layout** and TrustZone configuration

### Key Takeaways

1. **TF-M is modular** - Services can be enabled/disabled via CMake
2. **MCUboot provides secure boot** - Image verification with ECDSA signatures
3. **TrustZone isolates Secure/Non-Secure** - SAU/IDAU/MPU enforce separation
4. **PSA APIs are standardized** - Same API across all TF-M platforms
5. **Hardware crypto acceleration** significantly improves performance

### Next Steps

- **Lab 02:** Explore TrustZone memory protection in detail
- **Lab 03:** Use PSA Crypto API for hash, MAC, and cipher operations
- **Lab 04:** Store secrets securely using ITS and PS

---

## 📎 References

- [TF-M Documentation](https://tf-m-user-guide.trustedfirmware.org/)
- [STM32U5 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0456-stm32u5-series-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [PSA Certified API Specifications](https://arm-software.github.io/psa-api/)
- [MCUboot Documentation](https://docs.mcuboot.com/)

---

**Lab 01 Complete!** ✅

You now have a working TF-M development environment for NUCLEO-U545RE-Q.
