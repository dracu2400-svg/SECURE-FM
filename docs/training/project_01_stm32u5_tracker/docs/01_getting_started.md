# Project 1: Secure GPS Tracker with TF-M

## Complete Implementation Guide for NUCLEO-U545RE-Q

**Target Hardware:**
- NUCLEO-U545RE-Q (STM32U545RET6Q)
- SimCom A7672SA 4G LTE + GPS Module
- X-Nucleo-IQS4A1 (Multi-Sensor Board with IMU)

**For:** Beginners to embedded security and TF-M

---

## 📋 Table of Contents

1. [Project Overview](#1-project-overview)
2. [Hardware Setup](#2-hardware-setup)
3. [Architecture Explained](#3-architecture-explained)
4. [Step-by-Step Implementation](#4-step-by-step-implementation)
5. [How TF-M is Used](#5-how-tfm-is-used)
6. [Testing and Verification](#6-testing-and-verification)
7. [Troubleshooting](#7-troubleshooting)

---

## 1. Project Overview

### 1.1 What We're Building

A **secure GPS tracker** that:
- ✅ Tracks real-time GPS location
- ✅ Detects motion using IMU (accelerometer + gyroscope)
- ✅ Sends data to cloud via 4G LTE
- ✅ Stores data securely using TF-M
- ✅ Updates firmware over-the-air (OTA)
- ✅ Resists attacks (tampering, debugging, flash readout)

**Use Case:** Vehicle tracking, asset tracking, personal safety devices

### 1.2 Why Use TF-M?

**Without TF-M (insecure):**
```
❌ GPS keys stored in plain flash → Attacker can clone device
❌ No secure boot → Attacker can install malicious firmware
❌ Debug port open → Attacker can extract all data
❌ No encryption → Cloud credentials visible in flash
```

**With TF-M (secure):**
```
✅ GPS keys in TF-M Secure Storage (ITS) → Cannot extract
✅ Secure boot with MCUboot → Only signed firmware runs
✅ Debug port locked → Cannot access via SWD/JTAG
✅ TLS credentials encrypted → Safe cloud communication
```

### 1.3 Learning Objectives

By completing this project, you will learn:
- How to use TF-M PSA Crypto API
- How to store secrets securely (ITS/PS)
- How to implement secure boot
- How to use attestation
- How to integrate sensors with secure firmware
- How to send data to cloud securely

---

## 2. Hardware Setup

### 2.1 Bill of Materials

| Component | Part Number | Price | Where to Buy |
|-----------|-------------|-------|--------------|
| **Main Board** | NUCLEO-U545RE-Q | ~$25 | ST.com, Mouser, DigiKey |
| **4G + GPS Module** | SimCom A7672SA | ~$30 | AliExpress, Alibaba |
| **Sensor Board** | X-Nucleo-IQS4A1 | ~$50 | ST.com |
| **Antenna** | 4G LTE Antenna | ~$5 | Amazon |
| **Antenna** | GPS Antenna (active) | ~$10 | Amazon |
| **SIM Card** | IoT SIM Card | ~$5/mo | Hologram, Twilio |
| **Power** | USB Cable or Battery | ~$5 | Amazon |
| **Total** | | **~$130** | |

### 2.2 Hardware Connections

#### 2.2.1 NUCLEO-U545RE-Q Overview

```
┌─────────────────────────────────────────────────┐
│         NUCLEO-U545RE-Q                         │
│  ┌──────────────────────────────────────────┐   │
│  │  STM32U545RET6Q                          │   │
│  │  - ARM Cortex-M33 @ 160 MHz              │   │
│  │  - TrustZone-M enabled                   │   │
│  │  - 512 KB Flash, 256 KB RAM              │   │
│  │  - Hardware crypto accelerator           │   │
│  └──────────────────────────────────────────┘   │
│                                                  │
│  Arduino Connectors:                             │
│  - CN5: D0-D15 (Digital)                        │
│  - CN6: A0-A5 (Analog)                          │
│  - CN7: More I/O                                │
│                                                  │
│  Morpho Connectors:                              │
│  - CN11/CN12: All pins                          │
│                                                  │
│  ST-Link V3:                                     │
│  - USB for programming and debug                │
│  - Virtual COM port                             │
└─────────────────────────────────────────────────┘
```

#### 2.2.2 X-Nucleo-IQS4A1 Sensor Board

The X-Nucleo-IQS4A1 plugs directly onto the NUCLEO board via Arduino headers!

**What's on X-Nucleo-IQS4A1:**
- **LSM6DSO:** 6-axis IMU (3-axis accelerometer + 3-axis gyroscope)
- **LIS2MDL:** 3-axis magnetometer
- **LPS22HH:** Pressure/temperature sensor
- **STTS751:** High-accuracy temperature sensor

**Connection:**
```
Just plug X-Nucleo-IQS4A1 onto NUCLEO board Arduino headers!

┌─────────────────────────────┐
│   X-Nucleo-IQS4A1           │
│   (Sensor Board)            │
│   - LSM6DSO (IMU)           │
│   - LIS2MDL (Mag)           │
│   - LPS22HH (Pressure)      │
└──────────┬──────────────────┘
           │ (Arduino Headers)
           ↓
┌─────────────────────────────┐
│   NUCLEO-U545RE-Q            │
│   (Main Board)              │
└─────────────────────────────┘
```

**I2C Connection (automatic via headers):**
- SDA: PB9 (I2C1_SDA)
- SCL: PB8 (I2C1_SCL)
- INT1: PD11 (LSM6DSO interrupt)
- INT2: PF3 (LIS2MDL interrupt)

#### 2.2.3 SimCom A7672SA Module

**Pin Connections:**

| A7672SA Pin | NUCLEO Pin | Function | Notes |
|-------------|------------|----------|-------|
| TXD | PA10 (USART1_RX) | UART TX | AT commands |
| RXD | PA9 (USART1_TX) | UART RX | AT commands |
| PWRKEY | PB0 | Power key | Low pulse to power on |
| RESET | PB1 | Reset | Low pulse to reset |
| STATUS | PB2 | Status | High when module is on |
| VDD | 5V | Power | 5V @ 2A required |
| GND | GND | Ground | Common ground |
| ANT_MAIN | | 4G Antenna | External antenna |
| GPS_ANT | | GPS Antenna | Active GPS antenna |

**Wiring Diagram:**
```
NUCLEO-U545RE-Q              SimCom A7672SA
┌─────────────┐              ┌──────────────┐
│             │              │              │
│ PA9 (TX) ───┼──────────────┼→ RXD         │
│ PA10 (RX)←──┼──────────────┼─ TXD         │
│             │              │              │
│ PB0 ────────┼──────────────┼→ PWRKEY      │
│ PB1 ────────┼──────────────┼→ RESET       │
│ PB2 ←───────┼──────────────┼─ STATUS      │
│             │              │              │
│ 5V ─────────┼──────────────┼→ VDD         │
│ GND ────────┼──────────────┼→ GND         │
│             │              │              │
└─────────────┘              └───┬──────┬───┘
                                 │      │
                              [4G ANT][GPS ANT]
```

**IMPORTANT:** A7672SA requires 5V @ 2A. Use external power supply!

### 2.3 Physical Assembly Steps

**Step 1:** Plug X-Nucleo-IQS4A1 onto NUCLEO board
```
1. Align Arduino headers (CN5, CN6, CN7)
2. Press firmly until seated
3. Check no bent pins
```

**Step 2:** Connect A7672SA module
```
1. Solder wires or use breadboard
2. Connect as per wiring diagram above
3. Double-check 5V power (critical!)
4. Connect antennas before powering on
```

**Step 3:** Insert SIM card into A7672SA
```
1. Power off module
2. Insert nano-SIM card
3. Ensure correct orientation
```

**Step 4:** Connect antennas
```
1. 4G antenna to ANT_MAIN
2. GPS antenna to GPS_ANT
3. Ensure tight connection
```

**Step 5:** Power connections
```
1. USB to NUCLEO (for programming + debug)
2. 5V 2A to A7672SA (dedicated power!)
3. LED LD1 on NUCLEO should light up
```

---

## 3. Architecture Explained

### 3.1 System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│                     Cloud Server                             │
│                  (AWS IoT / Azure IoT)                       │
│                                                              │
│  Receives: Location, Motion, Battery, Temperature           │
│  Sends: Configuration, OTA updates                          │
│                                                              │
└───────────────────────┬──────────────────────────────────────┘
                        │
                        │ HTTPS/MQTT over TLS 1.3
                        │ (Encrypted with TF-M PSA Crypto)
                        │
┌───────────────────────▼──────────────────────────────────────┐
│                                                              │
│               SimCom A7672SA Module                          │
│               (4G LTE + GPS)                                 │
│                                                              │
│  - Connects to cellular network                             │
│  - Gets GPS position                                        │
│  - Sends data to cloud                                      │
│                                                              │
└───────────────────────┬──────────────────────────────────────┘
                        │
                        │ AT Commands over UART
                        │
┌───────────────────────▼──────────────────────────────────────┐
│                                                              │
│            NUCLEO-U545RE-Q (STM32U545)                       │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │         Non-Secure World                               │ │
│  │  ┌──────────────────────────────────────────────────┐  │ │
│  │  │ Application Code                                 │  │ │
│  │  │ - GPS parser                                     │  │ │
│  │  │ - Motion detection logic                         │  │ │
│  │  │ - Cloud communication                            │  │ │
│  │  │ - User interface                                 │  │ │
│  │  └──────────────────────────────────────────────────┘  │ │
│  │           ↓ PSA API Calls                              │ │
│  └────────────┼────────────────────────────────────────────┘ │
│               │                                              │
│  ═════════════╪═══════════════════════════════════════════  │
│               │ TrustZone Boundary                           │
│  ═════════════╪═══════════════════════════════════════════  │
│               │                                              │
│  ┌────────────▼────────────────────────────────────────────┐ │
│  │         Secure World (TF-M)                            │ │
│  │                                                         │ │
│  │  ┌─────────────┐  ┌──────────────┐  ┌──────────────┐  │ │
│  │  │ Crypto      │  │ Secure       │  │ Attestation  │  │ │
│  │  │ Service     │  │ Storage      │  │ Service      │  │ │
│  │  │             │  │ (ITS/PS)     │  │              │  │ │
│  │  │ - TLS keys  │  │ - Device ID  │  │ - Prove      │  │ │
│  │  │ - Signing   │  │ - GPS key    │  │   identity   │  │ │
│  │  │ - Hashing   │  │ - Cloud creds│  │              │  │ │
│  │  └─────────────┘  └──────────────┘  └──────────────┘  │ │
│  │                                                         │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                              │
└───────────────────────┬──────────────────────────────────────┘
                        │
                        │ I2C
                        │
┌───────────────────────▼──────────────────────────────────────┐
│                                                              │
│            X-Nucleo-IQS4A1 Sensor Board                      │
│                                                              │
│  - LSM6DSO: 6-axis IMU (accel + gyro)                       │
│  - LIS2MDL: 3-axis magnetometer                             │
│  - LPS22HH: Pressure + temperature                          │
│                                                              │
│  Provides: Motion detection, orientation, shock detection   │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### 3.2 Data Flow Example

**Scenario:** Device detects motion and sends location to cloud

```
1. LSM6DSO (IMU) detects motion
   ↓
2. Interrupt triggers on PD11
   ↓
3. Non-Secure app reads accelerometer via I2C
   ↓
4. App calls psa_hash_compute() to hash GPS data
   ↓
5. TF-M Crypto service computes SHA-256 (in Secure world)
   ↓
6. App requests GPS position from A7672SA
   ↓
7. A7672SA returns NMEA sentences via UART
   ↓
8. App parses GPS data
   ↓
9. App calls psa_its_get() to retrieve cloud credentials
   ↓
10. TF-M ITS service returns encrypted credentials (Secure)
   ↓
11. App encrypts payload with psa_aead_encrypt()
   ↓
12. TF-M Crypto uses AES-GCM (hardware accelerated)
   ↓
13. App sends HTTPS POST to cloud via A7672SA
   ↓
14. Cloud receives: {lat, lon, motion_detected, timestamp}
```

**Key Point:** All cryptographic operations happen in TF-M Secure world!

### 3.3 How TF-M Protects This Project

| Threat | Without TF-M | With TF-M |
|--------|--------------|-----------|
| **Key Extraction** | Attacker dumps flash → gets cloud credentials | ✅ Keys in TF-M ITS (encrypted, cannot extract) |
| **Firmware Tampering** | Attacker installs malware firmware | ✅ MCUboot verifies signatures (only signed FW runs) |
| **Debug Access** | Attacker connects debugger → reads RAM | ✅ Debug ports locked (RDP Level 2) |
| **GPS Spoofing** | Attacker sends fake GPS data | ✅ Attestation proves device authenticity |
| **Replay Attacks** | Attacker replays old location data | ✅ Timestamps signed with TF-M crypto |
| **Cloning** | Attacker clones device | ✅ Device-unique keys (hardware UID) |

---

## 4. Step-by-Step Implementation

### 4.1 Project Structure

```
project_01_stm32u5_tracker/
├── docs/
│   ├── 01_getting_started.md          ← You are here!
│   ├── 02_hardware_setup.md
│   ├── 03_software_architecture.md
│   ├── 04_testing_guide.md
│   └── 05_deployment.md
├── src/
│   ├── drivers/
│   │   ├── simcom_a7672sa/
│   │   │   ├── simcom_a7672sa.c       ← 4G + GPS driver
│   │   │   ├── simcom_a7672sa.h
│   │   │   ├── at_parser.c            ← AT command parser
│   │   │   ├── gps_parser.c           ← NMEA parser
│   │   │   └── gps_parser.h
│   │   └── x_nucleo_iqs4a1/
│   │       ├── lsm6dso_driver.c       ← IMU driver
│   │       ├── lsm6dso_driver.h
│   │       ├── lis2mdl_driver.c       ← Magnetometer
│   │       ├── lps22hh_driver.c       ← Pressure sensor
│   │       └── sensor_fusion.c        ← Combine sensors
│   ├── services/
│   │   ├── location_service.c         ← GPS location management
│   │   ├── motion_service.c           ← Motion detection
│   │   ├── cloud_service.c            ← Cloud communication
│   │   └── ota_service.c              ← OTA updates
│   ├── secure/
│   │   ├── secure_storage.c           ← TF-M ITS/PS wrapper
│   │   ├── secure_crypto.c            ← TF-M Crypto wrapper
│   │   └── attestation.c              ← Device attestation
│   ├── app/
│   │   ├── main.c                     ← Main application
│   │   ├── app_config.h
│   │   └── state_machine.c            ← App state machine
│   ├── platform/
│   │   ├── stm32u5_hal_config.c       ← HAL initialization
│   │   ├── uart.c                     ← UART driver
│   │   ├── i2c.c                      ← I2C driver
│   │   └── gpio.c                     ← GPIO driver
│   └── CMakeLists.txt
└── README.md
```

### 4.2 Implementation Phases

We'll implement in **5 phases**, each building on the previous:

```
Phase 1: Basic Setup
├─ Initialize TF-M
├─ Setup UART for A7672SA
├─ Power on module
└─ Send first AT command
   Result: Module responds "OK"

Phase 2: GPS Integration
├─ Request GPS data
├─ Parse NMEA sentences
├─ Display location on serial console
└─ Store location in TF-M Secure Storage
   Result: See your GPS coordinates!

Phase 3: IMU Integration (X-Nucleo-IQS4A1)
├─ Initialize I2C
├─ Configure LSM6DSO (IMU)
├─ Read accelerometer data
├─ Detect motion
└─ Trigger events on movement
   Result: Device detects when you move it!

Phase 4: Cloud Integration
├─ Configure 4G connection
├─ Establish TLS connection to cloud
├─ Send location + motion data
├─ Receive commands from cloud
└─ Use TF-M Crypto for all encryption
   Result: Data visible on cloud dashboard!

Phase 5: OTA Updates
├─ Receive firmware update via 4G
├─ Verify signature with TF-M
├─ Install update with MCUboot
└─ Rollback if update fails
   Result: Update firmware remotely!
```

---

## 5. How TF-M is Used

### 5.1 TF-M Integration Points

#### Point 1: Secure Key Storage

**Where:** Storing cloud credentials, GPS API keys

**Code Example:**
```c
#include "psa/storage.h"

/**
 * Store cloud credentials securely
 * This data is encrypted by TF-M and cannot be read even if flash is dumped
 */
void store_cloud_credentials(const char *api_key)
{
    psa_storage_uid_t uid = 1001;  // Unique ID for cloud credentials

    // Store in TF-M Internal Trusted Storage (ITS)
    psa_status_t status = psa_its_set(
        uid,                        // Unique identifier
        strlen(api_key),            // Data length
        api_key,                    // Data to store
        PSA_STORAGE_FLAG_NONE       // Flags
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Cloud credentials stored securely in TF-M ITS\n");
    }
}

/**
 * Retrieve cloud credentials
 */
void get_cloud_credentials(char *api_key, size_t max_len)
{
    psa_storage_uid_t uid = 1001;
    size_t data_len;

    // Retrieve from TF-M ITS
    psa_status_t status = psa_its_get(
        uid,
        0,                          // Offset
        max_len,                    // Max length
        api_key,                    // Output buffer
        &data_len                   // Actual length
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Retrieved cloud credentials from TF-M\n");
    }
}
```

**Why TF-M Here?**
- Data encrypted with device-unique key
- Key never leaves Secure world
- Even with flash dump, attacker cannot decrypt

#### Point 2: Cryptographic Operations

**Where:** Encrypting GPS data before sending to cloud

**Code Example:**
```c
#include "psa/crypto.h"

/**
 * Encrypt GPS location data before sending to cloud
 * Uses TF-M PSA Crypto API (hardware-accelerated AES-GCM)
 */
psa_status_t encrypt_location_data(
    const uint8_t *plaintext,
    size_t plaintext_len,
    uint8_t *ciphertext,
    size_t ciphertext_size,
    size_t *ciphertext_len)
{
    // Get encryption key from TF-M Secure Storage
    psa_key_id_t key_id = 2001;  // Cloud encryption key

    // Generate random nonce
    uint8_t nonce[12];
    psa_generate_random(nonce, sizeof(nonce));

    // Encrypt with AES-256-GCM (TF-M uses hardware accelerator!)
    psa_status_t status = psa_aead_encrypt(
        key_id,                     // Encryption key (in Secure world)
        PSA_ALG_GCM,                // Algorithm: AES-GCM
        nonce, sizeof(nonce),       // Nonce
        NULL, 0,                    // No additional authenticated data
        plaintext, plaintext_len,   // Input data
        ciphertext, ciphertext_size, // Output buffer
        ciphertext_len              // Output length
    );

    if (status == PSA_SUCCESS) {
        printf("✓ GPS data encrypted with TF-M Crypto (AES-GCM)\n");
    }

    return status;
}
```

**Why TF-M Here?**
- Uses STM32U5 hardware crypto accelerator (fast!)
- Key stays in Secure world (never exposed)
- Constant-time implementation (side-channel resistant)

#### Point 3: Device Attestation

**Where:** Proving device authenticity to cloud

**Code Example:**
```c
#include "psa/initial_attestation.h"

/**
 * Generate attestation token to prove device is genuine
 * Cloud verifies this before accepting data
 */
psa_status_t generate_attestation_token(
    uint8_t *token,
    size_t token_size,
    size_t *token_len)
{
    // Challenge from cloud (prevents replay)
    uint8_t challenge[32];
    get_challenge_from_cloud(challenge, sizeof(challenge));

    // TF-M generates signed attestation token
    psa_status_t status = psa_initial_attest_get_token(
        challenge, sizeof(challenge),
        token, token_size,
        token_len
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Attestation token generated by TF-M\n");
        printf("  Token includes:\n");
        printf("  - Device unique ID\n");
        printf("  - Firmware hash\n");
        printf("  - Security lifecycle state\n");
        printf("  - Signed with device private key\n");
    }

    return status;
}
```

**Why TF-M Here?**
- Proves firmware hasn't been tampered with
- Device private key never leaves Secure world
- Cloud can verify signature with public key

#### Point 4: Secure Boot

**Where:** Verifying firmware at boot

**This happens automatically in MCUboot (BL2):**
```c
/**
 * MCUboot verifies firmware signature before running it
 * (This code is in MCUboot, not your application)
 */
int boot_go(void)
{
    // 1. MCUboot reads firmware from flash
    image_header_t *hdr = (image_header_t *)FIRMWARE_ADDR;

    // 2. Compute SHA-256 hash of firmware
    uint8_t hash[32];
    sha256(hdr + 1, hdr->img_size, hash);

    // 3. Get public key from TF-M/OTP
    psa_key_id_t public_key = get_root_public_key();

    // 4. Verify ECDSA signature
    psa_status_t status = psa_verify_hash(
        public_key,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),
        hash, 32,
        hdr->signature, 64
    );

    if (status != PSA_SUCCESS) {
        printf("❌ Firmware signature invalid! Not booting.\n");
        return -1;  // Stay in bootloader
    }

    printf("✓ Firmware signature valid\n");

    // 5. Check rollback protection
    if (hdr->version < get_nv_counter()) {
        printf("❌ Rollback attack detected!\n");
        return -1;
    }

    // 6. All checks passed - jump to firmware
    jump_to_app(FIRMWARE_ADDR);
}
```

**Why TF-M Here?**
- Only signed firmware can run
- Prevents malware installation
- Rollback protection (cannot downgrade)

### 5.2 Summary: TF-M Usage in Project

| Feature | TF-M Service Used | Benefit |
|---------|-------------------|---------|
| **Cloud Credentials** | ITS (Internal Trusted Storage) | Encrypted storage, cannot extract |
| **GPS Encryption** | PSA Crypto (AES-GCM) | Hardware-accelerated, secure |
| **Device Identity** | Attestation | Proves authenticity to cloud |
| **Firmware Updates** | Secure Boot (MCUboot) | Only signed firmware runs |
| **Random Numbers** | PSA Crypto (RNG) | True random (hardware TRNG) |
| **Key Derivation** | PSA Crypto (HKDF) | Derive keys from master secret |

**Key Insight:** Without TF-M, all these security features would need to be implemented manually (error-prone!) or wouldn't exist at all (insecure!).

---

## 6. Testing and Verification

### 6.1 Phase 1 Test: Basic Communication

**Test 1.1: Module Powers On**
```c
// In main.c
void test_module_power_on(void)
{
    printf("\n=== Test 1.1: Power On A7672SA ===\n");

    // Power on module
    simcom_power_on();

    // Wait for module to boot
    HAL_Delay(5000);

    // Send AT command
    char response[128];
    simcom_send_at_command("AT", response, sizeof(response), 1000);

    if (strstr(response, "OK")) {
        printf("✓ PASS: Module responded to AT command\n");
    } else {
        printf("❌ FAIL: No response from module\n");
    }
}
```

**Expected Output:**
```
=== Test 1.1: Power On A7672SA ===
Powering on module...
Waiting 5 seconds for boot...
Sending AT command...
Received: AT

OK

✓ PASS: Module responded to AT command
```

**Test 1.2: SIM Card Detected**
```c
void test_sim_card(void)
{
    printf("\n=== Test 1.2: SIM Card Detection ===\n");

    char response[128];
    simcom_send_at_command("AT+CPIN?", response, sizeof(response), 1000);

    if (strstr(response, "READY")) {
        printf("✓ PASS: SIM card detected and ready\n");
    } else {
        printf("❌ FAIL: SIM card not ready\n");
        printf("Response: %s\n", response);
    }
}
```

**Expected Output:**
```
=== Test 1.2: SIM Card Detection ===
Sending AT+CPIN?...
Received: +CPIN: READY

OK

✓ PASS: SIM card detected and ready
```

### 6.2 Phase 2 Test: GPS Location

**Test 2.1: GPS Fix Acquired**
```c
void test_gps_fix(void)
{
    printf("\n=== Test 2.1: GPS Fix ===\n");
    printf("Move device near window for GPS signal...\n");

    // Start GPS
    simcom_gps_start();

    // Wait up to 60 seconds for fix
    int timeout = 60;
    while (timeout-- > 0) {
        simcom_gps_data_t gps_data;
        simcom_gps_get_position(&gps_data);

        if (gps_data.valid && gps_data.fix_type == SIMCOM_GPS_FIX_3D) {
            printf("✓ PASS: GPS fix acquired!\n");
            printf("  Latitude:  %.6f°\n", gps_data.latitude);
            printf("  Longitude: %.6f°\n", gps_data.longitude);
            printf("  Altitude:  %.1f m\n", gps_data.altitude);
            printf("  Satellites: %d\n", gps_data.satellites);
            return;
        }

        printf("  Waiting for GPS fix... (%d satellites)\n", gps_data.satellites);
        HAL_Delay(1000);
    }

    printf("❌ FAIL: No GPS fix after 60 seconds\n");
}
```

**Expected Output:**
```
=== Test 2.1: GPS Fix ===
Move device near window for GPS signal...
Starting GPS...
  Waiting for GPS fix... (0 satellites)
  Waiting for GPS fix... (3 satellites)
  Waiting for GPS fix... (5 satellites)
✓ PASS: GPS fix acquired!
  Latitude:  37.422408°
  Longitude: -122.084068°
  Altitude:  15.2 m
  Satellites: 8
```

**Test 2.2: GPS Data Stored Securely in TF-M**
```c
void test_gps_secure_storage(void)
{
    printf("\n=== Test 2.2: Secure GPS Storage ===\n");

    // Get current GPS position
    simcom_gps_data_t gps_data;
    simcom_gps_get_position(&gps_data);

    // Store in TF-M ITS
    psa_storage_uid_t uid = 3001;
    psa_status_t status = psa_its_set(
        uid,
        sizeof(gps_data),
        &gps_data,
        PSA_STORAGE_FLAG_NONE
    );

    if (status == PSA_SUCCESS) {
        printf("✓ GPS data stored in TF-M Secure Storage\n");

        // Retrieve and verify
        simcom_gps_data_t retrieved;
        size_t data_len;
        psa_its_get(uid, 0, sizeof(retrieved), &retrieved, &data_len);

        if (memcmp(&gps_data, &retrieved, sizeof(gps_data)) == 0) {
            printf("✓ PASS: Data retrieved correctly\n");
        } else {
            printf("❌ FAIL: Retrieved data doesn't match\n");
        }
    } else {
        printf("❌ FAIL: Failed to store in TF-M\n");
    }
}
```

### 6.3 Phase 3 Test: Motion Detection

**Test 3.1: IMU Communication**
```c
void test_imu_communication(void)
{
    printf("\n=== Test 3.1: IMU Communication ===\n");

    // Initialize LSM6DSO
    lsm6dso_ctx_t ctx;
    lsm6dso_init(&ctx);

    // Read WHO_AM_I register
    uint8_t whoami;
    lsm6dso_device_id_get(&ctx, &whoami);

    if (whoami == LSM6DSO_ID) {
        printf("✓ PASS: LSM6DSO detected (WHO_AM_I = 0x%02X)\n", whoami);
    } else {
        printf("❌ FAIL: Wrong WHO_AM_I: 0x%02X (expected 0x%02X)\n",
               whoami, LSM6DSO_ID);
    }
}
```

**Test 3.2: Accelerometer Reading**
```c
void test_accelerometer(void)
{
    printf("\n=== Test 3.2: Accelerometer ===\n");

    lsm6dso_ctx_t ctx;

    // Read accelerometer
    int16_t accel_raw[3];
    lsm6dso_acceleration_raw_get(&ctx, accel_raw);

    // Convert to mg (milli-g)
    float accel_mg[3];
    for (int i = 0; i < 3; i++) {
        accel_mg[i] = lsm6dso_from_fs2_to_mg(accel_raw[i]);
    }

    printf("Accelerometer data:\n");
    printf("  X: %6.2f mg\n", accel_mg[0]);
    printf("  Y: %6.2f mg\n", accel_mg[1]);
    printf("  Z: %6.2f mg (should be ~1000 mg = 1g)\n", accel_mg[2]);

    // Check if Z-axis is approximately 1g (device laying flat)
    if (fabs(accel_mg[2] - 1000.0f) < 200.0f) {
        printf("✓ PASS: Z-axis reads approximately 1g\n");
    } else {
        printf("⚠ WARNING: Unexpected acceleration values\n");
    }
}
```

**Test 3.3: Motion Detection**
```c
void test_motion_detection(void)
{
    printf("\n=== Test 3.3: Motion Detection ===\n");
    printf("Shake the device when prompted...\n");

    // Configure motion detection threshold
    lsm6dso_ctx_t ctx;
    lsm6dso_wkup_threshold_set(&ctx, 2);  // 2 * 15.625 mg = 31.25 mg

    printf("Device is now monitoring for motion.\n");
    printf("Please shake the device...\n");

    // Wait for motion interrupt
    int timeout = 10;
    while (timeout-- > 0) {
        if (HAL_GPIO_ReadPin(IMU_INT1_GPIO_Port, IMU_INT1_Pin) == GPIO_PIN_SET) {
            printf("✓ PASS: Motion detected via interrupt!\n");
            return;
        }
        HAL_Delay(1000);
        printf(".");
        fflush(stdout);
    }

    printf("\n❌ FAIL: No motion detected in 10 seconds\n");
}
```

**Expected Output (when you shake device):**
```
=== Test 3.3: Motion Detection ===
Shake the device when prompted...
Device is now monitoring for motion.
Please shake the device...
....✓ PASS: Motion detected via interrupt!
```

### 6.4 Phase 4 Test: Cloud Communication

**Test 4.1: Connect to Cloud**
```c
void test_cloud_connection(void)
{
    printf("\n=== Test 4.1: Cloud Connection ===\n");

    // Activate data connection
    simcom_activate_data();

    // Get IP address
    simcom_network_info_t info;
    simcom_get_network_info(&info);
    printf("IP Address: %s\n", info.ip_address);

    // Open HTTPS connection to cloud
    simcom_socket_config_t config = {
        .protocol = SIMCOM_PROTO_TCP,
        .remote_host = "api.mycloud.com",
        .remote_port = 443,
        .use_tls = true,
        .tls_version = SIMCOM_TLS_VERSION_1_3
    };

    simcom_socket_t socket;
    psa_status_t status = simcom_socket_open(&config, &socket);

    if (status == SIMCOM_OK) {
        printf("✓ PASS: TLS connection established\n");
        simcom_socket_close(&socket);
    } else {
        printf("❌ FAIL: Could not connect to cloud\n");
    }
}
```

**Test 4.2: Send Encrypted Data**
```c
void test_send_encrypted_data(void)
{
    printf("\n=== Test 4.2: Send Encrypted Location ===\n");

    // Get current location
    simcom_gps_data_t gps;
    simcom_gps_get_position(&gps);

    // Format JSON payload
    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,\"time\":%lu}",
             gps.latitude, gps.longitude, gps.altitude, gps.timestamp);

    printf("Payload: %s\n", payload);

    // Encrypt with TF-M
    uint8_t ciphertext[512];
    size_t ciphertext_len;

    encrypt_location_data(
        (uint8_t *)payload, strlen(payload),
        ciphertext, sizeof(ciphertext),
        &ciphertext_len
    );

    printf("Encrypted length: %zu bytes\n", ciphertext_len);

    // Send HTTPS POST
    char response[1024];
    int http_code;

    simcom_http_request(
        SIMCOM_HTTP_POST,
        "https://api.mycloud.com/location",
        NULL,  // No extra headers
        ciphertext, ciphertext_len,
        (uint8_t *)response, sizeof(response),
        &http_code
    );

    if (http_code == 200) {
        printf("✓ PASS: Data sent successfully\n");
        printf("Server response: %s\n", response);
    } else {
        printf("❌ FAIL: HTTP error %d\n", http_code);
    }
}
```

### 6.5 Complete Test Sequence

**Run all tests:**
```c
int main(void)
{
    // Initialize hardware
    HAL_Init();
    SystemClock_Config();

    // Initialize TF-M
    tfm_ns_interface_init();

    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  GPS Tracker - Complete Test Suite            ║\n");
    printf("║  NUCLEO-U545RE-Q + X-Nucleo-IQS4A1 + A7672SA   ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("\n");

    // Phase 1 Tests
    printf("━━━ Phase 1: Basic Communication ━━━\n");
    test_module_power_on();
    test_sim_card();

    // Phase 2 Tests
    printf("\n━━━ Phase 2: GPS ━━━\n");
    test_gps_fix();
    test_gps_secure_storage();

    // Phase 3 Tests
    printf("\n━━━ Phase 3: Motion Detection ━━━\n");
    test_imu_communication();
    test_accelerometer();
    test_motion_detection();

    // Phase 4 Tests
    printf("\n━━━ Phase 4: Cloud Communication ━━━\n");
    test_cloud_connection();
    test_send_encrypted_data();

    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  All Tests Complete!                           ║\n");
    printf("╚════════════════════════════════════════════════╝\n");

    // Run main application
    run_tracker_application();
}
```

---

## 7. Troubleshooting

### 7.1 A7672SA Module Issues

**Problem:** Module doesn't power on

**Solutions:**
```
1. Check 5V power supply (must provide 2A!)
2. Hold PWRKEY low for 2 seconds
3. Check STATUS pin goes high when on
4. Verify antennas connected before power on
```

**Problem:** No AT response

**Solutions:**
```
1. Check UART connections (TX↔RX, RX↔TX)
2. Verify baud rate (115200)
3. Add CR+LF terminators: "AT\r\n"
4. Check module is powered (STATUS high)
```

**Problem:** No GPS fix

**Solutions:**
```
1. Move device outdoors or near window
2. Ensure GPS antenna connected
3. Wait 30-60 seconds for cold start
4. Check NMEA sentences: AT+CGNSINF
```

### 7.2 X-Nucleo-IQS4A1 Issues

**Problem:** WHO_AM_I register returns wrong value

**Solutions:**
```
1. Check board is firmly seated on NUCLEO
2. Verify I2C pull-ups enabled
3. Check I2C address (LSM6DSO: 0x6A or 0x6B)
4. Scan I2C bus: i2cdetect
```

**Problem:** No motion interrupt

**Solutions:**
```
1. Check INT1 pin connection (PD11)
2. Verify interrupt enabled in LSM6DSO
3. Check threshold not too high
4. Enable pull-up on INT pin
```

### 7.3 TF-M Issues

**Problem:** psa_its_set() returns error

**Solutions:**
```
1. Check TF-M is initialized: tfm_ns_interface_init()
2. Verify ITS partition enabled in build
3. Check UID is valid (non-zero)
4. Ensure enough flash space for ITS
```

**Problem:** psa_crypto_init() fails

**Solutions:**
```
1. Check Crypto partition enabled
2. Verify mbedTLS configured correctly
3. Check RAM not exhausted
4. Enable hardware crypto accelerator
```

---

**Next:** [Part 2: Complete Driver Implementation](02_driver_implementation.md)

This guide continues with full source code for all drivers, services, and the complete application!
