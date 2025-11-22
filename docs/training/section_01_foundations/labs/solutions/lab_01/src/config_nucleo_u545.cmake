#-------------------------------------------------------------------------------
# TF-M Configuration for NUCLEO-U545RE-Q
# Lab 01 Solution - Platform Configuration Template
#-------------------------------------------------------------------------------
#
# This file contains the CMake configuration for building TF-M on the
# NUCLEO-U545RE-Q development board (STM32U545RET6Q).
#
# Usage:
#   cmake <TFM_ROOT> -C config_nucleo_u545.cmake
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Platform Selection
#-------------------------------------------------------------------------------
# Note: STM32U545 support may not be in upstream TF-M yet, so we use
# STM32L552 as a template (both are Cortex-M33 with TrustZone)
#-------------------------------------------------------------------------------
set(TFM_PLATFORM "stm/nucleo_l552ze_q" CACHE STRING "Platform" FORCE)

#-------------------------------------------------------------------------------
# Toolchain Configuration
#-------------------------------------------------------------------------------
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
set(TFM_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/toolchain_GNUARM.cmake" CACHE FILEPATH "Toolchain file" FORCE)

#-------------------------------------------------------------------------------
# TrustZone and Isolation Configuration
#-------------------------------------------------------------------------------
# Isolation Level 1: No isolation between secure partitions (smallest)
# Isolation Level 2: PSA Level 2 - Isolation with SPM (recommended)
# Isolation Level 3: PSA Level 3 - Full isolation with MPU (largest)
#-------------------------------------------------------------------------------
set(TFM_ISOLATION_LEVEL 2 CACHE STRING "Isolation level" FORCE)

#-------------------------------------------------------------------------------
# Configuration Profile
#-------------------------------------------------------------------------------
# profile_small:  Minimal features, smallest size (~60 KB)
# profile_medium: Balanced features/size (~100 KB)
# profile_large:  All features enabled (~150 KB)
#-------------------------------------------------------------------------------
set(TFM_PROFILE "profile_medium" CACHE STRING "Configuration profile" FORCE)

#-------------------------------------------------------------------------------
# PSA Secure Services
#-------------------------------------------------------------------------------
# Enable core PSA services required for most applications
#-------------------------------------------------------------------------------
set(TFM_PARTITION_CRYPTO ON CACHE BOOL "Cryptographic services" FORCE)
set(TFM_PARTITION_INTERNAL_TRUSTED_STORAGE ON CACHE BOOL "Internal Trusted Storage" FORCE)
set(TFM_PARTITION_PROTECTED_STORAGE ON CACHE BOOL "Protected Storage" FORCE)
set(TFM_PARTITION_PLATFORM ON CACHE BOOL "Platform services" FORCE)
set(TFM_PARTITION_INITIAL_ATTESTATION ON CACHE BOOL "Attestation service" FORCE)
set(TFM_PARTITION_FIRMWARE_UPDATE ON CACHE BOOL "Firmware update service" FORCE)

#-------------------------------------------------------------------------------
# MCUboot Bootloader (BL2)
#-------------------------------------------------------------------------------
# MCUboot provides secure boot and OTA firmware update capabilities
#-------------------------------------------------------------------------------
set(BL2 ON CACHE BOOL "Enable MCUboot bootloader" FORCE)

# Image configuration
set(MCUBOOT_IMAGE_NUMBER 1 CACHE STRING "Number of images (1 or 2)" FORCE)
# For single image: Both S and NS in one combined image
# For dual image: Separate S and NS images (more flexible)

# Upgrade strategy
set(MCUBOOT_UPGRADE_STRATEGY "SWAP_USING_SCRATCH" CACHE STRING "Upgrade strategy" FORCE)
# Options:
#   - SWAP_USING_SCRATCH: Uses scratch area for swapping (safest, requires more flash)
#   - OVERWRITE_ONLY: Direct overwrite (smallest, no rollback)
#   - SWAP_USING_MOVE: Swap without scratch (mid-size, complex)

# Security features
set(MCUBOOT_HW_KEY ON CACHE BOOL "Use hardware-derived keys" FORCE)
set(MCUBOOT_SIGNATURE_TYPE "EC-P256" CACHE STRING "Signature algorithm" FORCE)
# Options: EC-P256 (recommended), RSA-2048, RSA-3072

#-------------------------------------------------------------------------------
# Cryptography Configuration
#-------------------------------------------------------------------------------
# Hardware acceleration significantly improves crypto performance on STM32U5
#-------------------------------------------------------------------------------
set(CRYPTO_HW_ACCELERATOR ON CACHE BOOL "Use STM32 hardware crypto" FORCE)

# Crypto library backend
set(TFM_MBEDCRYPTO_PLATFORM_EXTRA_CONFIG_PATH "${CMAKE_SOURCE_DIR}/platform/ext/common/template/crypto_hw_accelerator_config_extra.h" CACHE FILEPATH "Crypto config" FORCE)

#-------------------------------------------------------------------------------
# Platform Services Configuration
#-------------------------------------------------------------------------------
set(PLATFORM_DEFAULT_UART_STDOUT ON CACHE BOOL "Enable UART stdout" FORCE)
set(PLATFORM_DEFAULT_NV_COUNTERS ON CACHE BOOL "Enable NV counters" FORCE)
set(PLATFORM_DEFAULT_ROTPK ON CACHE BOOL "Enable ROTPK" FORCE)
set(PLATFORM_DEFAULT_IAK ON CACHE BOOL "Enable IAK" FORCE)

#-------------------------------------------------------------------------------
# Debug and Testing Configuration
#-------------------------------------------------------------------------------
# Disable for production builds to save space
#-------------------------------------------------------------------------------
set(TEST_S ON CACHE BOOL "Enable secure tests" FORCE)
set(TEST_NS ON CACHE BOOL "Enable non-secure tests" FORCE)

# Regression test suites
set(TEST_S_CRYPTO ON CACHE BOOL "Crypto tests" FORCE)
set(TEST_S_ITS ON CACHE BOOL "ITS tests" FORCE)
set(TEST_S_PS ON CACHE BOOL "PS tests" FORCE)
set(TEST_S_ATTESTATION ON CACHE BOOL "Attestation tests" FORCE)
set(TEST_S_PLATFORM ON CACHE BOOL "Platform tests" FORCE)

#-------------------------------------------------------------------------------
# Memory Configuration for STM32U545RET6Q
#-------------------------------------------------------------------------------
# Flash: 512 KB (0x0800_0000 - 0x080F_FFFF)
# RAM:   256 KB (0x2000_0000 - 0x2003_FFFF)
#-------------------------------------------------------------------------------
# Note: Memory layout is defined in platform-specific files:
#   - platform/ext/target/stm/nucleo_l552ze_q/partition/flash_layout.h
#   - platform/ext/target/stm/nucleo_l552ze_q/partition/region_defs.h
#
# For custom memory layout (Lab 07), you would modify these files
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Logging Configuration
#-------------------------------------------------------------------------------
set(TFM_SPM_LOG_LEVEL TFM_SPM_LOG_LEVEL_INFO CACHE STRING "SPM log level" FORCE)
# Options: SILENCE, ERROR, INFO, DEBUG

set(TFM_PARTITION_LOG_LEVEL TFM_PARTITION_LOG_LEVEL_INFO CACHE STRING "Partition log level" FORCE)
# Same options as SPM log level

#-------------------------------------------------------------------------------
# Stack and Heap Sizes
#-------------------------------------------------------------------------------
# These affect RAM usage - tune based on application requirements
#-------------------------------------------------------------------------------
set(CONFIG_TFM_SPM_THREAD_STACK_SIZE 0x800 CACHE STRING "SPM thread stack size" FORCE)
set(CONFIG_TFM_CONN_HANDLE_MAX_NUM 8 CACHE STRING "Max connection handles" FORCE)

#-------------------------------------------------------------------------------
# Optional Features
#-------------------------------------------------------------------------------
# These can be disabled to reduce size for production builds
#-------------------------------------------------------------------------------
set(TFM_CODE_SHARING OFF CACHE BOOL "Code sharing between S and NS" FORCE)
set(TFM_NS_CLIENT_IDENTIFICATION ON CACHE BOOL "NS client ID tracking" FORCE)

#-------------------------------------------------------------------------------
# Security Hardening Options
#-------------------------------------------------------------------------------
set(TFM_DUMMY_PROVISIONING ON CACHE BOOL "Use dummy provisioning (DEV ONLY)" FORCE)
# WARNING: Set to OFF for production! Uses hardcoded test keys.

#-------------------------------------------------------------------------------
# Documentation
#-------------------------------------------------------------------------------
# This configuration provides:
#   - Secure boot with ECDSA-P256 signature verification
#   - TrustZone isolation (Level 2)
#   - All major PSA services enabled
#   - Hardware crypto acceleration
#   - OTA firmware update support
#   - Development testing enabled
#
# Total approximate sizes:
#   - BL2 (MCUboot):        ~40 KB
#   - TF-M Secure:          ~100 KB (without tests: ~80 KB)
#   - TF-M Non-Secure:      Variable (application-dependent)
#
# Remaining flash for application: ~200 KB
# Remaining RAM for application:   ~100 KB (after TF-M allocations)
#-------------------------------------------------------------------------------
