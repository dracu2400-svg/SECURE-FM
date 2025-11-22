# TF-M Training Labs 5-10: Build System and Configuration

**Lab Series:** Build System Deep Dive, Configuration, Debugging, Performance
**Duration:** 12-15 hours
**Prerequisites:** Labs 1-4, PART1 completed

---

## Lab 5: Build System Deep Dive

**Duration:** 2-3 hours
**Difficulty:** Intermediate
**Goal:** Understand TF-M's CMake build system and create custom configurations

### Learning Objectives

By the end of this lab, you will be able to:
- Navigate TF-M's CMake build structure
- Customize platform configurations
- Add custom source files and libraries
- Control build options and profiles
- Debug build issues effectively

---

### Background: TF-M Build Architecture

TF-M uses a hierarchical CMake build system:

```
┌────────────────────────────────────────────────────────┐
│ TF-M Build System Structure                            │
├────────────────────────────────────────────────────────┤
│                                                        │
│  trusted-firmware-m/                                   │
│    │                                                   │
│    ├─ CMakeLists.txt (Root)                            │
│    │   └─ Sets global configuration                   │
│    │                                                   │
│    ├─ platform/                                        │
│    │   └─ ext/target/<vendor>/<platform>/              │
│    │       ├─ CMakeLists.txt (Platform-specific)       │
│    │       ├─ config.cmake (Platform defaults)         │
│    │       ├─ partition/ (Platform manifest)           │
│    │       └─ Device/ (HAL, startup)                   │
│    │                                                   │
│    ├─ secure_fw/                                       │
│    │   ├─ CMakeLists.txt (Secure firmware)             │
│    │   ├─ spm/ (SPM core)                              │
│    │   └─ partitions/ (Secure partitions)              │
│    │       ├─ crypto/                                  │
│    │       ├─ initial_attestation/                     │
│    │       ├─ internal_trusted_storage/                │
│    │       └─ protected_storage/                       │
│    │                                                   │
│    └─ lib/ (Libraries)                                 │
│        ├─ ext/mcuboot/                                 │
│        └─ ext/mbedtls/                                 │
│                                                        │
└────────────────────────────────────────────────────────┘
```

**Key CMake Variables:**

```cmake
# Platform selection
TFM_PLATFORM           # e.g., "stm/stm32u585xx"

# Build profiles
CMAKE_BUILD_TYPE       # Debug | Release | RelWithDebInfo | MinSizeRel

# TF-M Profiles (predefined configurations)
TFM_PROFILE            # profile_small | profile_medium | profile_large

# Isolation levels
TFM_ISOLATION_LEVEL    # 1 | 2 | 3

# Secure services
TFM_PARTITION_CRYPTO               # ON | OFF
TFM_PARTITION_INITIAL_ATTESTATION  # ON | OFF
TFM_PARTITION_PROTECTED_STORAGE    # ON | OFF
TFM_PARTITION_INTERNAL_TRUSTED_STORAGE # ON | OFF
TFM_PARTITION_PLATFORM             # ON | OFF
TFM_PARTITION_FIRMWARE_UPDATE      # ON | OFF

# MCUboot configuration
BL2                    # TRUE | FALSE (enable MCUboot bootloader)
MCUBOOT_IMAGE_NUMBER   # 1 | 2 (number of updatable images)

# Crypto backend
TFM_CRYPTO_DRIVER      # CC312 | OBERON | MBED_TLS
CRYPTO_HW_ACCELERATOR  # ON | OFF

# Logging
TFM_LOG_LEVEL          # TFM_LOG_LEVEL_SILENCE | INFO | DEBUG | ERROR
```

---

### Exercise 5.1: Explore Build Configuration

**Task:** Examine the build configuration for STM32U5

**Steps:**

1. Navigate to the STM32U5 platform directory:
```bash
cd ~/tfm_workspace/trusted-firmware-m
cd platform/ext/target/stm/stm32u585xx
```

2. Read the platform config file:
```bash
cat config.cmake
```

**Expected output (excerpt):**
```cmake
set(TFM_PLATFORM_STM32U585XX    ON)
set(CRYPTO_HW_ACCELERATOR       ON)  # STM32U5 has AES/PKA accelerators
set(TFM_CRYPTO_DRIVER           STM32_CRYPTO)

# Memory layout
set(FLASH_BASE_ADDRESS          0x0C000000)
set(FLASH_SIZE                  0x00200000)  # 2MB
set(RAM_BASE_ADDRESS            0x30000000)
set(RAM_SIZE                    0x000C0000)  # 768KB

# Bootloader settings
set(BL2_HEADER_SIZE             0x400)
set(BL2_TRAILER_SIZE            0x400)
```

3. Examine the linker script:
```bash
cat device/source/armclang/stm32u585xx_s.sct
```

**Questions:**
- Q1: What is the total flash size available?
  **Answer:** 2MB (0x200000)

- Q2: Which crypto accelerators are enabled?
  **Answer:** STM32_CRYPTO (includes AES, PKA, HASH engines)

- Q3: What is the bootloader header size?
  **Answer:** 0x400 (1024 bytes)

---

### Exercise 5.2: Build with Different Profiles

**Task:** Build TF-M with different profiles and compare sizes

**Profile Comparison:**

| Profile | Size | Features | Use Case |
|---------|------|----------|----------|
| profile_small | ~40KB | Minimal crypto, ITS only | Constrained devices |
| profile_medium | ~80KB | Full crypto, ITS+PS | Typical IoT |
| profile_large | ~140KB | All services, attestation | Feature-rich |

**Steps:**

1. Build with profile_small:
```bash
cd ~/tfm_workspace
mkdir build_small && cd build_small

cmake ../trusted-firmware-m \
    -DTFM_PLATFORM=stm/stm32u585xx \
    -DTFM_PROFILE=profile_small \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -GNinja

ninja
```

2. Check binary size:
```bash
arm-none-eabi-size bin/tfm_s.axf
```

**Expected output:**
```
   text    data     bss     dec     hex filename
  38420    1024    4096   43540    aa14 bin/tfm_s.axf
```

3. Build with profile_medium:
```bash
cd ~/tfm_workspace
mkdir build_medium && cd build_medium

cmake ../trusted-firmware-m \
    -DTFM_PLATFORM=stm/stm32u585xx \
    -DTFM_PROFILE=profile_medium \
    -DCMAKE_BUILD_TYPE=Release \
    -GNinja

ninja

arm-none-eabi-size bin/tfm_s.axf
```

**Expected output:**
```
   text    data     bss     dec     hex filename
  78624    2048    8192   88864   15b40 bin/tfm_s.axf
```

4. Compare features enabled:
```bash
# Check config in build directory
cat config.log | grep TFM_PARTITION
```

**Expected output (profile_small):**
```
TFM_PARTITION_CRYPTO=OFF
TFM_PARTITION_INITIAL_ATTESTATION=OFF
TFM_PARTITION_PROTECTED_STORAGE=OFF
TFM_PARTITION_INTERNAL_TRUSTED_STORAGE=ON
```

**Expected output (profile_medium):**
```
TFM_PARTITION_CRYPTO=ON
TFM_PARTITION_INITIAL_ATTESTATION=ON
TFM_PARTITION_PROTECTED_STORAGE=ON
TFM_PARTITION_INTERNAL_TRUSTED_STORAGE=ON
```

**Analysis:**
- profile_medium is ~2x larger than profile_small
- Crypto service adds ~30KB
- Attestation adds ~10KB
- Choose profile based on flash budget and requirements

---

### Exercise 5.3: Custom Build Configuration

**Task:** Create a custom configuration for your GPS tracker project

**Requirements:**
- Enable crypto, ITS, PS, attestation, FWU
- Disable debugging features (production build)
- Enable hardware crypto acceleration
- Set custom flash layout

**Steps:**

1. Create a custom config file:
```bash
cd ~/tfm_workspace
cat > custom_tracker_config.cmake << 'EOF'
# Custom configuration for GPS Tracker project

# Platform
set(TFM_PLATFORM "stm/stm32u585xx" CACHE STRING "Platform")

# Build type
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type")

# Isolation level (2 = PSA RoT + App RoT separated)
set(TFM_ISOLATION_LEVEL 2 CACHE STRING "Isolation level")

# Enable required services
set(TFM_PARTITION_CRYPTO ON CACHE BOOL "Crypto service")
set(TFM_PARTITION_INTERNAL_TRUSTED_STORAGE ON CACHE BOOL "ITS")
set(TFM_PARTITION_PROTECTED_STORAGE ON CACHE BOOL "PS")
set(TFM_PARTITION_INITIAL_ATTESTATION ON CACHE BOOL "Attestation")
set(TFM_PARTITION_FIRMWARE_UPDATE ON CACHE BOOL "FWU service")
set(TFM_PARTITION_PLATFORM ON CACHE BOOL "Platform service")

# Bootloader
set(BL2 TRUE CACHE BOOL "Enable MCUboot")
set(MCUBOOT_IMAGE_NUMBER 1 CACHE STRING "Single image")
set(MCUBOOT_UPGRADE_STRATEGY "SWAP_USING_SCRATCH" CACHE STRING "Swap mode")

# Hardware acceleration
set(CRYPTO_HW_ACCELERATOR ON CACHE BOOL "Use STM32 crypto accelerators")

# Logging (minimal for production)
set(TFM_LOG_LEVEL TFM_LOG_LEVEL_ERROR CACHE STRING "Log level")
set(MCUBOOT_LOG_LEVEL "ERROR" CACHE STRING "MCUboot log level")

# Security features
set(TFM_DUMMY_PROVISIONING OFF CACHE BOOL "Real provisioning")
set(PLATFORM_DEFAULT_CRYPTO_KEYS OFF CACHE BOOL "No default keys")

# Memory optimization
set(TFM_CODE_SHARING ON CACHE BOOL "Share code between partitions")

# Flash layout (adjust for OTA)
set(FLASH_S_PARTITION_SIZE 0x60000 CACHE STRING "384KB secure FW")
set(FLASH_NS_PARTITION_SIZE 0x80000 CACHE STRING "512KB non-secure app")

EOF
```

2. Build with custom configuration:
```bash
mkdir build_tracker && cd build_tracker

cmake ../trusted-firmware-m \
    -C ../custom_tracker_config.cmake \
    -GNinja

ninja
```

3. Verify configuration:
```bash
# Check that all required services are enabled
grep "TFM_PARTITION" CMakeCache.txt | grep "=ON"
```

**Expected output:**
```
TFM_PARTITION_CRYPTO:BOOL=ON
TFM_PARTITION_INTERNAL_TRUSTED_STORAGE:BOOL=ON
TFM_PARTITION_PROTECTED_STORAGE:BOOL=ON
TFM_PARTITION_INITIAL_ATTESTATION:BOOL=ON
TFM_PARTITION_FIRMWARE_UPDATE:BOOL=ON
TFM_PARTITION_PLATFORM:BOOL=ON
```

4. Check memory usage:
```bash
arm-none-eabi-size bin/tfm_s.axf bin/tfm_ns.bin
```

**Expected output:**
```
   text    data     bss     dec     hex filename
  98304    2048   12288  112640   1b800 bin/tfm_s.axf   (Secure)
  65536    1024    8192   74752   12400 bin/tfm_ns.bin  (Non-Secure)
```

**Explanation:**
- Secure firmware: ~98KB (fits in 384KB allocation)
- Non-secure app: ~65KB (fits in 512KB allocation)
- Remaining flash for OTA secondary slot

---

### Exercise 5.4: Add Custom Source Files

**Task:** Add a custom secure service to the build

**Scenario:** Create a simple "device info" service that provides hardware information to non-secure code.

**Steps:**

1. Create directory structure:
```bash
cd ~/tfm_workspace/trusted-firmware-m
mkdir -p secure_fw/partitions/device_info
cd secure_fw/partitions/device_info
```

2. Create service source file:
```bash
cat > device_info.c << 'EOF'
/*
 * Device Information Service
 * Provides hardware info to non-secure world
 */

#include "psa/service.h"
#include "tfm_sp_log.h"
#include <stdint.h>
#include <string.h>

/* Service signals */
#define DEVICE_INFO_SIGNAL  (1U << 0)

/* Device info structure */
typedef struct {
    uint32_t device_id;
    uint32_t hardware_version;
    uint32_t bootloader_version;
    uint8_t  serial_number[16];
} device_info_t;

/* Service implementation */
psa_status_t tfm_device_info_get(device_info_t *info)
{
    if (!info) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    /* Fill with platform-specific values */
    info->device_id = 0x12345678;
    info->hardware_version = 0x00020001;  /* v2.1 */
    info->bootloader_version = 0x00010800;  /* v1.8.0 */

    /* Read unique device serial from STM32 UID register */
    uint32_t *uid = (uint32_t *)0x0BFA0700;  /* STM32U5 UID base */
    memcpy(info->serial_number, uid, 12);

    LOG_MSG("Device info requested");

    return PSA_SUCCESS;
}

/* Service entry point */
void device_info_service_entry(void)
{
    psa_signal_t signals;

    while (1) {
        signals = psa_wait(DEVICE_INFO_SIGNAL, PSA_BLOCK);

        if (signals & DEVICE_INFO_SIGNAL) {
            psa_msg_t msg;
            psa_status_t status;
            device_info_t info;

            /* Read message */
            psa_get(DEVICE_INFO_SIGNAL, &msg);

            /* Get device info */
            status = tfm_device_info_get(&info);

            /* Write response */
            if (status == PSA_SUCCESS) {
                psa_write(msg.handle, 0, &info, sizeof(info));
            }

            /* Complete request */
            psa_reply(msg.handle, status);
        }
    }
}
EOF
```

3. Create partition manifest:
```bash
cat > tfm_device_info.yaml << 'EOF'
{
  "psa_framework_version": 1.0,
  "name": "TFM_SP_DEVICE_INFO",
  "type": "APPLICATION-ROT",
  "priority": "NORMAL",
  "entry_point": "device_info_service_entry",
  "stack_size": "0x0400",
  "services": [
    {
      "name": "TFM_DEVICE_INFO_SERVICE",
      "sid": "0x00000070",
      "non_secure_clients": true,
      "version": 1,
      "version_policy": "STRICT"
    }
  ],
  "mmio_regions": [
    {
      "name": "TFM_PERIPHERAL_UID",
      "permission": "READ-ONLY"
    }
  ]
}
EOF
```

4. Create CMakeLists.txt:
```bash
cat > CMakeLists.txt << 'EOF'
#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if (NOT TFM_PARTITION_DEVICE_INFO)
    return()
endif()

cmake_minimum_required(VERSION 3.15)

add_library(tfm_psa_rot_partition_device_info STATIC)

target_sources(tfm_psa_rot_partition_device_info
    PRIVATE
        device_info.c
)

target_include_directories(tfm_psa_rot_partition_device_info
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/interface/include
)

target_link_libraries(tfm_psa_rot_partition_device_info
    PRIVATE
        tfm_secure_api
        psa_interface
        platform_s
        tfm_sprt
)

target_compile_definitions(tfm_psa_rot_partition_device_info
    PRIVATE
        TFM_PARTITION_DEVICE_INFO
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_psa_rot_partition_device_info
)

target_compile_definitions(tfm_partition_defs
    INTERFACE
        TFM_PARTITION_DEVICE_INFO
)
EOF
```

5. Add to parent CMakeLists.txt:
```bash
cd ~/tfm_workspace/trusted-firmware-m/secure_fw/partitions
cat >> CMakeLists.txt << 'EOF'

# Device Info Service
add_subdirectory(device_info)
EOF
```

6. Enable in config file:
```bash
echo 'set(TFM_PARTITION_DEVICE_INFO ON CACHE BOOL "Device info service")' \
    >> ~/tfm_workspace/custom_tracker_config.cmake
```

7. Rebuild:
```bash
cd ~/tfm_workspace/build_tracker
cmake ../trusted-firmware-m -C ../custom_tracker_config.cmake -GNinja
ninja
```

8. Verify service is included:
```bash
arm-none-eabi-nm bin/tfm_s.axf | grep device_info
```

**Expected output:**
```
0c0045a0 T device_info_service_entry
0c004620 T tfm_device_info_get
```

**Success!** Your custom service is now part of the TF-M build.

---

### Exercise 5.5: Build Troubleshooting

**Common Build Issues and Solutions:**

**Problem 1: "CMake Error: Platform not found"**
```
CMake Error: TFM_PLATFORM is not set or invalid
```

**Solution:**
```bash
# Always specify platform explicitly
cmake ../trusted-firmware-m \
    -DTFM_PLATFORM=stm/stm32u585xx \
    ...
```

---

**Problem 2: "Linker error: region 'FLASH' overflowed"**
```
region `FLASH' overflowed by 8192 bytes
```

**Solution:**
```bash
# Reduce features or increase partition size
# Option 1: Disable unused services
set(TFM_PARTITION_PROTECTED_STORAGE OFF)

# Option 2: Increase flash allocation
set(FLASH_S_PARTITION_SIZE 0x70000)  # Increase from 0x60000

# Option 3: Use MinSizeRel build
cmake ... -DCMAKE_BUILD_TYPE=MinSizeRel
```

---

**Problem 3: "Undefined reference to 'psa_crypto_init'"**
```
tfm_ns.axf: undefined reference to `psa_crypto_init'
```

**Solution:**
```bash
# Enable crypto partition
set(TFM_PARTITION_CRYPTO ON)

# Link against PSA API library in non-secure CMakeLists.txt
target_link_libraries(tfm_ns PRIVATE tfm_api_ns)
```

---

**Problem 4: Clean build after configuration change**
```bash
# Configuration changes don't always trigger rebuild
# Solution: Clean and rebuild
rm -rf build_tracker
mkdir build_tracker && cd build_tracker
cmake ../trusted-firmware-m -C ../custom_tracker_config.cmake -GNinja
ninja
```

---

### Lab 5 Summary

**What you learned:**
- ✅ Navigate TF-M's CMake build system
- ✅ Use different build profiles (small/medium/large)
- ✅ Create custom configurations
- ✅ Add custom secure partitions
- ✅ Troubleshoot common build issues

**Key takeaways:**
1. TF-M uses hierarchical CMake configuration
2. Profiles provide predefined feature sets
3. Custom partitions require manifest + CMake integration
4. Memory constraints require careful size budgeting

**Files created:**
- `custom_tracker_config.cmake` - Custom configuration
- `secure_fw/partitions/device_info/*` - Custom service

---

## Lab 6: Configuration Profiles

**Duration:** 2 hours
**Difficulty:** Intermediate
**Goal:** Master TF-M configuration profiles and create optimized builds

### Learning Objectives

- Understand TF-M configuration profiles
- Create custom profiles for specific use cases
- Optimize memory and code size
- Balance security features with resource constraints

---

### Background: TF-M Profiles

TF-M provides three standard profiles:

```
┌──────────────────────────────────────────────────────────────┐
│ TF-M Profile Comparison                                      │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  profile_small (Minimal)                                     │
│    Flash: ~40KB | RAM: ~20KB                                 │
│    ├─ Isolation Level: 1                                     │
│    ├─ Services: ITS only                                     │
│    ├─ Crypto: Minimal (hash, AES-128)                        │
│    └─ Use case: Ultra-constrained devices                    │
│                                                              │
│  profile_medium (Balanced)                                   │
│    Flash: ~80KB | RAM: ~40KB                                 │
│    ├─ Isolation Level: 2                                     │
│    ├─ Services: Crypto, ITS, PS, Attestation                 │
│    ├─ Crypto: Full PSA Crypto API                            │
│    └─ Use case: Typical IoT devices                          │
│                                                              │
│  profile_large (Full-Featured)                               │
│    Flash: ~140KB | RAM: ~80KB                                │
│    ├─ Isolation Level: 3                                     │
│    ├─ Services: All (+ FWU, Platform)                        │
│    ├─ Crypto: Full + HW acceleration                         │
│    └─ Use case: Feature-rich secure applications             │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

---

### Exercise 6.1: Profile Feature Matrix

**Task:** Create a feature comparison table for all profiles

**Steps:**

1. Create comparison script:
```bash
cat > ~/tfm_workspace/compare_profiles.sh << 'EOF'
#!/bin/bash

echo "Building all profiles..."

PROFILES=("profile_small" "profile_medium" "profile_large")

for profile in "${PROFILES[@]}"; do
    echo "Building $profile..."

    BUILD_DIR="build_${profile}"
    rm -rf "$BUILD_DIR"
    mkdir "$BUILD_DIR" && cd "$BUILD_DIR"

    cmake ../trusted-firmware-m \
        -DTFM_PLATFORM=stm/stm32u585xx \
        -DTFM_PROFILE=$profile \
        -DCMAKE_BUILD_TYPE=MinSizeRel \
        -GNinja \
        > /dev/null 2>&1

    ninja > /dev/null 2>&1

    # Extract size
    SIZE=$(arm-none-eabi-size bin/tfm_s.axf | tail -1 | awk '{print $1}')

    # Extract enabled services
    CRYPTO=$(grep "TFM_PARTITION_CRYPTO:BOOL" CMakeCache.txt | cut -d= -f2)
    ATTEST=$(grep "TFM_PARTITION_INITIAL_ATTESTATION:BOOL" CMakeCache.txt | cut -d= -f2)
    PS=$(grep "TFM_PARTITION_PROTECTED_STORAGE:BOOL" CMakeCache.txt | cut -d= -f2)
    FWU=$(grep "TFM_PARTITION_FIRMWARE_UPDATE:BOOL" CMakeCache.txt | cut -d= -f2)
    ISOLATION=$(grep "TFM_ISOLATION_LEVEL:STRING" CMakeCache.txt | cut -d= -f2)

    echo "$profile,$SIZE,$CRYPTO,$ATTEST,$PS,$FWU,$ISOLATION"

    cd ..
done
EOF

chmod +x ~/tfm_workspace/compare_profiles.sh
~/tfm_workspace/compare_profiles.sh
```

**Expected output:**
```
profile_small,38420,OFF,OFF,OFF,OFF,1
profile_medium,78624,ON,ON,ON,OFF,2
profile_large,138240,ON,ON,ON,ON,3
```

2. Create formatted table:
```
| Profile        | Flash (KB) | Crypto | Attest | PS  | FWU | Isolation |
|----------------|------------|--------|--------|-----|-----|-----------|
| profile_small  | 38         | ✗      | ✗      | ✗   | ✗   | Level 1   |
| profile_medium | 77         | ✓      | ✓      | ✓   | ✗   | Level 2   |
| profile_large  | 135        | ✓      | ✓      | ✓   | ✓   | Level 3   |
```

**Analysis:**
- **profile_small:** Only 38KB but minimal features
- **profile_medium:** Good balance for typical IoT (77KB)
- **profile_large:** Full features but requires 135KB flash

---

### Exercise 6.2: Create a Custom "Tracker" Profile

**Task:** Create an optimized profile for the GPS tracker project

**Requirements:**
- Crypto (for TLS and signatures)
- ITS (for device keys)
- PS (for configuration data)
- Attestation (for device identity)
- FWU (for OTA updates)
- Isolation Level 2 (balance security/size)
- Minimize code size

**Steps:**

1. Create custom profile config:
```bash
cat > ~/tfm_workspace/profile_tracker.cmake << 'EOF'
#-------------------------------------------------------------------------------
# Custom Profile: GPS Tracker
# Optimized for OTA updates over 4G with minimal size
#-------------------------------------------------------------------------------

# Isolation level (2 = good security without L3 overhead)
set(TFM_ISOLATION_LEVEL                     2           CACHE STRING    "Isolation level")

# Build for size
set(CMAKE_BUILD_TYPE                        MinSizeRel  CACHE STRING    "Build type")

# Enable required services
set(TFM_PARTITION_CRYPTO                    ON          CACHE BOOL      "Crypto")
set(TFM_PARTITION_INTERNAL_TRUSTED_STORAGE  ON          CACHE BOOL      "ITS")
set(TFM_PARTITION_PROTECTED_STORAGE         ON          CACHE BOOL      "PS")
set(TFM_PARTITION_INITIAL_ATTESTATION       ON          CACHE BOOL      "Attestation")
set(TFM_PARTITION_FIRMWARE_UPDATE           ON          CACHE BOOL      "FWU")
set(TFM_PARTITION_PLATFORM                  ON          CACHE BOOL      "Platform")

# Crypto optimizations
set(CRYPTO_HW_ACCELERATOR                   ON          CACHE BOOL      "Use HW accel")
set(TFM_CRYPTO_ASYM_SIGN_MODULE_ENABLED     ON          CACHE BOOL      "ECDSA")
set(TFM_CRYPTO_ASYM_ENCRYPT_MODULE_ENABLED  OFF         CACHE BOOL      "No RSA")
set(TFM_CRYPTO_KEY_DERIVATION_MODULE_ENABLED ON         CACHE BOOL      "HKDF")

# Minimize non-essential features
set(TFM_CRYPTO_RNG_MODULE_DISABLED          OFF         CACHE BOOL      "Keep RNG")
set(TFM_CODE_SHARING                        ON          CACHE BOOL      "Share code")

# Logging (minimal for production)
set(TFM_LOG_LEVEL                           TFM_LOG_LEVEL_ERROR CACHE STRING "Errors only")
set(TFM_SPM_LOG_LEVEL                       TFM_LOG_LEVEL_ERROR CACHE STRING "Errors only")

# Bootloader
set(BL2                                     TRUE        CACHE BOOL      "MCUboot")
set(MCUBOOT_IMAGE_NUMBER                    1           CACHE STRING    "Single image")
set(MCUBOOT_UPGRADE_STRATEGY                "SWAP_USING_SCRATCH" CACHE STRING "Swap mode")
set(MCUBOOT_LOG_LEVEL                       "ERROR"     CACHE STRING    "Errors only")

# Security features
set(TFM_DUMMY_PROVISIONING                  OFF         CACHE BOOL      "Real provisioning")
set(PLATFORM_DEFAULT_CRYPTO_KEYS            OFF         CACHE BOOL      "No defaults")
set(MCUBOOT_HW_KEY                          ON          CACHE BOOL      "HW-derived key")

# Memory layout (optimized for 2MB flash)
set(BL2_HEADER_SIZE                         0x400       CACHE STRING    "1KB header")
set(BL2_TRAILER_SIZE                        0x400       CACHE STRING    "1KB trailer")
set(FLASH_S_PARTITION_SIZE                  0x60000     CACHE STRING    "384KB secure")
set(FLASH_NS_PARTITION_SIZE                 0x80000     CACHE STRING    "512KB non-sec")

EOF
```

2. Build with custom profile:
```bash
cd ~/tfm_workspace
mkdir build_profile_tracker && cd build_profile_tracker

cmake ../trusted-firmware-m \
    -DTFM_PLATFORM=stm/stm32u585xx \
    -C ../profile_tracker.cmake \
    -GNinja

ninja
```

3. Analyze size:
```bash
arm-none-eabi-size bin/tfm_s.axf
```

**Expected output:**
```
   text    data     bss     dec     hex filename
  92160    1536   10240  103936   19600 bin/tfm_s.axf
```

**Analysis:**
- **Size:** 92KB (between medium and large)
- **Features:** All required services enabled
- **Optimization:** HW acceleration + code sharing saves ~10KB
- **Result:** Fits comfortably in 384KB partition

4. Verify all services:
```bash
# Check enabled partitions
grep "TFM_PARTITION.*=ON" CMakeCache.txt
```

**Expected output:**
```
TFM_PARTITION_CRYPTO:BOOL=ON
TFM_PARTITION_INTERNAL_TRUSTED_STORAGE:BOOL=ON
TFM_PARTITION_PROTECTED_STORAGE:BOOL=ON
TFM_PARTITION_INITIAL_ATTESTATION:BOOL=ON
TFM_PARTITION_FIRMWARE_UPDATE:BOOL=ON
TFM_PARTITION_PLATFORM:BOOL=ON
```

✅ **Success!** Custom profile created with optimal size/feature balance.

---

### Exercise 6.3: Memory Budget Analysis

**Task:** Analyze memory usage breakdown

**Steps:**

1. Generate memory map:
```bash
cd ~/tfm_workspace/build_profile_tracker
arm-none-eabi-nm --size-sort --radix=d bin/tfm_s.axf | tail -20
```

**Expected output (top 20 symbols by size):**
```
0000001024 B psa_key_storage
0000002048 B crypto_context_pool
0000002560 B tfm_spm_partition_db
0000004096 B spm_boundary
0000008192 B tfm_stack
0000012288 T mbedtls_aes_encrypt
0000016384 T psa_hash_compute
...
```

2. Create size breakdown:
```bash
arm-none-eabi-objdump -h bin/tfm_s.axf | grep -A 1 "LOAD"
```

**Expected output:**
```
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         0x015800  0c000000  0c000000  00010000  2**5
  1 .data         0x000600  30000000  0c015800  00025800  2**3
  2 .bss          0x002800  30000600  0c016000  00000000  2**3
```

**Breakdown:**
```
Component              | Flash (KB) | RAM (KB) | Percentage
-----------------------|------------|----------|------------
Text (code)            | 86         | -        | 93%
RO Data (constants)    | 3          | -        | 3%
Data (initialized)     | 1.5        | 1.5      | 2%
BSS (uninitialized)    | -          | 10       | -
-----------------------|------------|----------|------------
TOTAL                  | 90.5       | 11.5     | 100%
```

3. Partition breakdown:
```bash
# Use build map file
cat bin/tfm_s.map | grep "^tfm_" | awk '{print $1, $2}'
```

**Estimated per-partition sizes:**
```
Partition                  | Flash (KB)
---------------------------|------------
SPM Core                   | 12
Crypto                     | 35
ITS                        | 8
PS                         | 10
Attestation                | 12
FWU                        | 6
Platform                   | 4
Shared libraries           | 5
---------------------------|------------
TOTAL                      | 92
```

**Key insights:**
- Crypto is largest partition (35KB = 38% of total)
- SPM core overhead is reasonable (12KB)
- Storage services are lightweight (18KB combined)

---

### Exercise 6.4: Extreme Size Optimization

**Task:** Create absolute minimum size build for resource-constrained device

**Scenario:** Device with only 128KB flash, needs basic crypto and storage

**Steps:**

1. Create ultra-minimal profile:
```bash
cat > ~/tfm_workspace/profile_minimal.cmake << 'EOF'
# Ultra-minimal profile - absolute minimum viable configuration

set(TFM_ISOLATION_LEVEL                     1           CACHE STRING    "Minimal isolation")
set(CMAKE_BUILD_TYPE                        MinSizeRel  CACHE STRING    "Size")

# Only essential services
set(TFM_PARTITION_CRYPTO                    ON          CACHE BOOL      "Crypto")
set(TFM_PARTITION_INTERNAL_TRUSTED_STORAGE  ON          CACHE BOOL      "ITS")
set(TFM_PARTITION_PROTECTED_STORAGE         OFF         CACHE BOOL      "No PS")
set(TFM_PARTITION_INITIAL_ATTESTATION       OFF         CACHE BOOL      "No attest")
set(TFM_PARTITION_FIRMWARE_UPDATE           OFF         CACHE BOOL      "No FWU")
set(TFM_PARTITION_PLATFORM                  OFF         CACHE BOOL      "No platform")

# Minimal crypto
set(TFM_CRYPTO_ASYM_SIGN_MODULE_ENABLED     ON          CACHE BOOL      "ECDSA only")
set(TFM_CRYPTO_ASYM_ENCRYPT_MODULE_ENABLED  OFF         CACHE BOOL      "No RSA")
set(TFM_CRYPTO_CIPHER_MODULE_ENABLED        ON          CACHE BOOL      "AES")
set(TFM_CRYPTO_AEAD_MODULE_ENABLED          ON          CACHE BOOL      "GCM")

# No logging
set(TFM_LOG_LEVEL                           TFM_LOG_LEVEL_SILENCE CACHE STRING "Silent")

# No bootloader (single image)
set(BL2                                     FALSE       CACHE BOOL      "No MCUboot")

# Code sharing
set(TFM_CODE_SHARING                        ON          CACHE BOOL      "Share code")

EOF
```

2. Build:
```bash
mkdir build_minimal && cd build_minimal
cmake ../trusted-firmware-m \
    -DTFM_PLATFORM=stm/stm32u585xx \
    -C ../profile_minimal.cmake \
    -GNinja
ninja
arm-none-eabi-size bin/tfm_s.axf
```

**Expected output:**
```
   text    data     bss     dec     hex filename
  28672     512    6144   35328    8a00 bin/tfm_s.axf
```

**Result:** Only 28KB! Achievable through:
- Isolation Level 1 (no MPU overhead)
- Only Crypto + ITS
- No logging
- No bootloader
- Aggressive optimization

---

### Lab 6 Summary

**What you learned:**
- ✅ TF-M configuration profiles (small/medium/large)
- ✅ Create custom profiles for specific use cases
- ✅ Analyze memory usage and partition sizes
- ✅ Extreme size optimization techniques

**Key techniques:**
1. Choose isolation level based on security vs size trade-off
2. Enable only required services
3. Use hardware acceleration when available
4. Disable logging in production builds
5. Enable code sharing between partitions

**Profiles created:**
- `profile_tracker.cmake` - GPS tracker (92KB)
- `profile_minimal.cmake` - Ultra-minimal (28KB)

---

## Lab 7: Custom Platform Configuration

**Duration:** 2-3 hours
**Difficulty:** Advanced
**Goal:** Configure TF-M for custom hardware platform

### Learning Objectives

- Understand platform-specific configuration
- Configure memory layout for custom flash/RAM
- Set up hardware crypto accelerators
- Configure peripherals and MMIO regions

---

### Background: Platform Configuration Files

Each platform has several configuration files:

```
platform/ext/target/<vendor>/<platform>/
├── config.cmake              # Build configuration
├── partition/                # Memory layout
│   ├── flash_layout.h        # Flash addresses
│   └── region_defs.h         # Memory regions
├── Device/
│   ├── Config/               # Device configuration
│   └── Source/
│       ├── startup_*.c       # Vector table, Reset_Handler
│       └── system_*.c        # SystemInit()
└── cmsis_drivers/            # CMSIS drivers for UART, Flash, etc.
```

---

### Exercise 7.1: Understanding Flash Layout

**Task:** Analyze and modify flash layout for custom requirements

**Scenario:** You have a custom STM32U5-based board with:
- 1MB flash (not 2MB like Nucleo board)
- Need OTA update support
- Limited RAM (256KB instead of 768KB)

**Steps:**

1. Read current flash layout:
```bash
cd ~/tfm_workspace/trusted-firmware-m
cat platform/ext/target/stm/stm32u585xx/partition/flash_layout.h
```

**Current layout (2MB flash):**
```c
/* Flash layout on STM32U585 (2MB flash):
 *
 * 0x0C00_0000  Bootloader (BL2) - 64KB
 * 0x0C01_0000  Secure Image Primary - 384KB
 * 0x0C07_0000  Non-Secure Image Primary - 512KB
 * 0x0C0F_0000  Secure Image Secondary - 384KB (OTA)
 * 0x0C15_0000  Non-Secure Image Secondary - 512KB (OTA)
 * 0x0C1D_0000  Scratch Area - 64KB
 * 0x0C1E_0000  Internal Trusted Storage - 16KB
 * 0x0C1E_4000  OTP / NV Counters - 8KB
 */

#define FLASH_BASE_ADDRESS              0x0C000000
#define FLASH_AREA_BL2_OFFSET           0x0
#define FLASH_AREA_BL2_SIZE             0x10000      /* 64KB */

#define FLASH_AREA_0_OFFSET             0x10000      /* Primary Secure */
#define FLASH_AREA_0_SIZE               0x60000      /* 384KB */

#define FLASH_AREA_1_OFFSET             0x70000      /* Primary NS */
#define FLASH_AREA_1_SIZE               0x80000      /* 512KB */

#define FLASH_AREA_2_OFFSET             0xF0000      /* Secondary Secure */
#define FLASH_AREA_2_SIZE               0x60000      /* 384KB */

#define FLASH_AREA_3_OFFSET             0x150000     /* Secondary NS */
#define FLASH_AREA_3_SIZE               0x80000      /* 512KB */

#define FLASH_AREA_SCRATCH_OFFSET       0x1D0000     /* Scratch */
#define FLASH_AREA_SCRATCH_SIZE         0x10000      /* 64KB */

#define FLASH_AREA_ITS_OFFSET           0x1E0000     /* ITS */
#define FLASH_AREA_ITS_SIZE             0x4000       /* 16KB */

#define FLASH_AREA_OTP_OFFSET           0x1E4000     /* OTP/Counters */
#define FLASH_AREA_OTP_SIZE             0x2000       /* 8KB */
```

2. Create custom layout for 1MB flash:
```bash
cat > ~/tfm_workspace/custom_flash_layout_1mb.h << 'EOF'
/*
 * Custom Flash Layout for 1MB STM32U5 variant
 * Total: 1MB (0x100000 bytes)
 */

#define FLASH_BASE_ADDRESS              0x0C000000
#define FLASH_TOTAL_SIZE                0x100000     /* 1MB */

/* Layout:
 * 0x0C00_0000  BL2 - 40KB (reduced from 64KB)
 * 0x0C00_A000  Primary Secure - 200KB (reduced)
 * 0x0C03_C000  Primary NS - 256KB (reduced)
 * 0x0C07_C000  Secondary Secure - 200KB (OTA)
 * 0x0C0A_E000  Secondary NS - 256KB (OTA)
 * 0x0C0E_E000  Scratch - 32KB (reduced)
 * 0x0C0F_6000  ITS - 16KB
 * 0x0C0F_A000  PS - 16KB
 * 0x0C0F_E000  OTP - 8KB
 */

/* Bootloader (BL2) - 40KB */
#define FLASH_AREA_BL2_OFFSET           0x0
#define FLASH_AREA_BL2_SIZE             0xA000       /* 40KB */

/* Primary Secure Image - 200KB */
#define FLASH_AREA_0_OFFSET             0xA000
#define FLASH_AREA_0_SIZE               0x32000      /* 200KB */

/* Primary Non-Secure Image - 256KB */
#define FLASH_AREA_1_OFFSET             0x3C000
#define FLASH_AREA_1_SIZE               0x40000      /* 256KB */

/* Secondary Secure Image (OTA slot) - 200KB */
#define FLASH_AREA_2_OFFSET             0x7C000
#define FLASH_AREA_2_SIZE               0x32000      /* 200KB */

/* Secondary Non-Secure Image (OTA slot) - 256KB */
#define FLASH_AREA_3_OFFSET             0xAE000
#define FLASH_AREA_3_SIZE               0x40000      /* 256KB */

/* Scratch area for image swap - 32KB */
#define FLASH_AREA_SCRATCH_OFFSET       0xEE000
#define FLASH_AREA_SCRATCH_SIZE         0x8000       /* 32KB */

/* Internal Trusted Storage - 16KB */
#define FLASH_AREA_ITS_OFFSET           0xF6000
#define FLASH_AREA_ITS_SIZE             0x4000       /* 16KB */

/* Protected Storage - 16KB */
#define FLASH_AREA_PS_OFFSET            0xFA000
#define FLASH_AREA_PS_SIZE              0x4000       /* 16KB */

/* OTP / NV Counters - 8KB */
#define FLASH_AREA_OTP_OFFSET           0xFE000
#define FLASH_AREA_OTP_SIZE             0x2000       /* 8KB */

/* Verify total fits in 1MB */
#if (FLASH_AREA_OTP_OFFSET + FLASH_AREA_OTP_SIZE) > FLASH_TOTAL_SIZE
#error "Flash layout exceeds 1MB!"
#endif

EOF
```

3. Visualize the layout:
```bash
cat > ~/tfm_workspace/visualize_flash.py << 'EOF'
#!/usr/bin/env python3
"""Visualize flash layout"""

layout = [
    ("BL2",              0x00000, 0x0A000, "Bootloader"),
    ("Primary Secure",   0x0A000, 0x32000, "Active S FW"),
    ("Primary NS",       0x3C000, 0x40000, "Active NS App"),
    ("Secondary Secure", 0x7C000, 0x32000, "OTA S slot"),
    ("Secondary NS",     0xAE000, 0x40000, "OTA NS slot"),
    ("Scratch",          0xEE000, 0x08000, "Swap buffer"),
    ("ITS",              0xF6000, 0x04000, "Secure storage"),
    ("PS",               0xFA000, 0x04000, "Data storage"),
    ("OTP",              0xFE000, 0x02000, "Counters"),
]

print("Flash Layout (1MB):\n")
print("Address        Size      Name                Description")
print("-" * 70)

for name, offset, size, desc in layout:
    addr = 0x0C000000 + offset
    size_kb = size // 1024
    print(f"0x{addr:08X}   {size_kb:3}KB     {name:18} {desc}")

total_used = sum(size for _, _, size, _ in layout)
print("-" * 70)
print(f"Total Used: {total_used // 1024}KB / 1024KB ({total_used * 100 // 0x100000}%)")

EOF

chmod +x ~/tfm_workspace/visualize_flash.py
python3 ~/tfm_workspace/visualize_flash.py
```

**Expected output:**
```
Flash Layout (1MB):

Address        Size      Name                Description
----------------------------------------------------------------------
0x0C000000    40KB     BL2                 Bootloader
0x0C00A000   200KB     Primary Secure      Active S FW
0x0C03C000   256KB     Primary NS          Active NS App
0x0C07C000   200KB     Secondary Secure    OTA S slot
0x0C0AE000   256KB     Secondary NS        OTA NS slot
0x0C0EE000    32KB     Scratch             Swap buffer
0x0C0F6000    16KB     ITS                 Secure storage
0x0C0FA000    16KB     PS                  Data storage
0x0C0FE000     8KB     OTP                 Counters
----------------------------------------------------------------------
Total Used: 1024KB / 1024KB (100%)
```

**Analysis:**
- Efficient use of 1MB flash
- OTA still supported with dual banks
- Scratch reduced to 32KB (still adequate)
- All storage areas preserved

---

### Exercise 7.2: RAM Configuration

**Task:** Configure RAM layout for 256KB RAM (vs 768KB default)

**Steps:**

1. Read current RAM configuration:
```bash
cat platform/ext/target/stm/stm32u585xx/partition/region_defs.h | grep RAM
```

**Current (768KB RAM):**
```c
#define RAM_BASE_ADDRESS                0x30000000
#define RAM_SIZE                        0x000C0000  /* 768KB */

/* RAM partitioning:
 * Secure:     384KB
 * Non-Secure: 384KB
 */
#define S_RAM_SIZE                      0x00060000  /* 384KB */
#define NS_RAM_SIZE                     0x00060000  /* 384KB */
```

2. Create 256KB configuration:
```bash
cat > ~/tfm_workspace/custom_ram_256kb.h << 'EOF'
/*
 * RAM Configuration for 256KB variant
 */

#define RAM_BASE_ADDRESS                0x30000000
#define RAM_SIZE                        0x00040000  /* 256KB */

/* RAM partitioning (optimized):
 * Secure:     128KB  (was 384KB)
 * Non-Secure: 128KB  (was 384KB)
 *
 * Breakdown:
 * Secure:
 *   - SPM Stack: 8KB
 *   - Partition Stacks: 24KB (6 partitions × 4KB)
 *   - Partition Data: 32KB
 *   - Shared: 64KB
 *
 * Non-Secure:
 *   - Application Stack: 16KB
 *   - Application Heap: 64KB
 *   - Application BSS: 48KB
 */

#define S_RAM_ALIAS_BASE                0x30000000
#define S_RAM_SIZE                      0x00020000  /* 128KB */

#define NS_RAM_ALIAS_BASE               0x20020000  /* Non-secure alias */
#define NS_RAM_SIZE                     0x00020000  /* 128KB */

/* Stack sizes (reduced for 256KB RAM) */
#define S_MSP_STACK_SIZE                0x00002000  /* 8KB (was 16KB) */
#define S_PSP_STACK_SIZE                0x00002000  /* 8KB (was 16KB) */

/* Partition stack sizes */
#define CRYPTO_STACK_SIZE               0x00002000  /* 8KB (was 16KB) */
#define ITS_STACK_SIZE                  0x00001000  /* 4KB (was 8KB) */
#define PS_STACK_SIZE                   0x00001000  /* 4KB (was 8KB) */
#define ATTEST_STACK_SIZE               0x00001000  /* 4KB (was 8KB) */
#define PLATFORM_STACK_SIZE             0x00001000  /* 4KB */
#define FWU_STACK_SIZE                  0x00001000  /* 4KB */

EOF
```

3. Verify RAM usage fits:
```bash
cat > ~/tfm_workspace/check_ram_usage.py << 'EOF'
#!/usr/bin/env python3
"""Check if RAM allocation fits in 256KB"""

# Secure RAM allocation
secure = {
    "SPM Stack": 8,
    "Crypto Stack": 8,
    "ITS Stack": 4,
    "PS Stack": 4,
    "Attestation Stack": 4,
    "Platform Stack": 4,
    "FWU Stack": 4,
    "Partition Data": 32,
    "Shared Buffers": 32,
}

# Non-Secure RAM allocation
non_secure = {
    "App Stack": 16,
    "App Heap": 64,
    "App BSS": 48,
}

print("Secure RAM Allocation:")
print("-" * 40)
for name, size in secure.items():
    print(f"  {name:20} {size:3} KB")
print("-" * 40)
secure_total = sum(secure.values())
print(f"  Total:               {secure_total:3} KB / 128 KB")

print("\nNon-Secure RAM Allocation:")
print("-" * 40)
for name, size in non_secure.items():
    print(f"  {name:20} {size:3} KB")
print("-" * 40)
ns_total = sum(non_secure.values())
print(f"  Total:               {ns_total:3} KB / 128 KB")

total = secure_total + ns_total
print(f"\nGrand Total: {total} KB / 256 KB")

if secure_total <= 128 and ns_total <= 128:
    print("✓ Allocation fits in 256KB RAM")
else:
    print("✗ Allocation exceeds 256KB RAM")

EOF

chmod +x ~/tfm_workspace/check_ram_usage.py
python3 ~/tfm_workspace/check_ram_usage.py
```

**Expected output:**
```
Secure RAM Allocation:
----------------------------------------
  SPM Stack              8 KB
  Crypto Stack           8 KB
  ITS Stack              4 KB
  PS Stack               4 KB
  Attestation Stack      4 KB
  Platform Stack         4 KB
  FWU Stack              4 KB
  Partition Data        32 KB
  Shared Buffers        32 KB
----------------------------------------
  Total:               100 KB / 128 KB

Non-Secure RAM Allocation:
----------------------------------------
  App Stack             16 KB
  App Heap              64 KB
  App BSS               48 KB
----------------------------------------
  Total:               128 KB / 128 KB

Grand Total: 228 KB / 256 KB
✓ Allocation fits in 256KB RAM
```

**Success!** Configuration fits with 28KB margin.

---

### Exercise 7.3: Hardware Crypto Accelerator Configuration

**Task:** Configure STM32U5 hardware crypto accelerators

**Background:** STM32U5 has several crypto accelerators:
- **AES** hardware engine
- **PKA** (Public Key Accelerator) for ECC operations
- **HASH** hardware for SHA-256
- **RNG** (True Random Number Generator)

**Steps:**

1. Create crypto accelerator configuration:
```bash
cat > ~/tfm_workspace/crypto_hw_config.cmake << 'EOF'
#-------------------------------------------------------------------------------
# STM32U5 Hardware Crypto Accelerator Configuration
#-------------------------------------------------------------------------------

# Enable hardware acceleration
set(CRYPTO_HW_ACCELERATOR                   ON          CACHE BOOL      "Enable HW crypto")

# Specify STM32 crypto driver
set(TFM_CRYPTO_DRIVER                       "STM32_CRYPTO" CACHE STRING  "Use STM32 driver")

# Configure which operations use hardware

# AES operations (use hardware AES engine)
set(STM32_CRYPTO_AES_HW                     ON          CACHE BOOL      "HW AES")
set(STM32_CRYPTO_AES_MODES                  "CBC CTR GCM" CACHE STRING  "AES modes")

# Hash operations (use hardware HASH peripheral)
set(STM32_CRYPTO_HASH_HW                    ON          CACHE BOOL      "HW HASH")
set(STM32_CRYPTO_HASH_ALGOS                 "SHA256"    CACHE STRING    "Hash algos")

# PKA for ECC (use Public Key Accelerator)
set(STM32_CRYPTO_PKA_HW                     ON          CACHE BOOL      "HW PKA")
set(STM32_CRYPTO_ECC_CURVES                 "SECP256R1" CACHE STRING    "ECC curves")

# RNG (use True RNG)
set(STM32_CRYPTO_RNG_HW                     ON          CACHE BOOL      "HW RNG")

# Fallback to software for unsupported operations
set(STM32_CRYPTO_SW_FALLBACK                ON          CACHE BOOL      "SW fallback")

EOF
```

2. Build with HW acceleration:
```bash
cd ~/tfm_workspace
mkdir build_hw_crypto && cd build_hw_crypto

