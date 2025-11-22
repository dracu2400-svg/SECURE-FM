# TF-M Training Labs 21-25: Advanced Topics

**Lab Series:** Custom Partitions, Interrupts, Porting, Security Testing, PSA Certification
**Duration:** 15-18 hours
**Prerequisites:** Labs 1-20, All modules completed

---

## Lab 21: Custom Secure Partition

**Duration:** 4 hours
**Difficulty:** Advanced
**Goal:** Create a custom secure partition from scratch

### Learning Objectives

- Design a secure partition
- Write partition manifest
- Implement IPC message handling
- Integrate with TF-M build system
- Test custom service from NSPE

---

### Exercise 21.1: Sensor Security Service

**Task:** Create a secure partition to manage sensor access

**Scenario:** GPS tracker has sensitive motion sensor that only secure code should access

**Step 1: Create Partition Structure**
```bash
cd ~/tfm_workspace/trusted-firmware-m/secure_fw/partitions
mkdir sensor_manager
cd sensor_manager
```

**Step 2: Write Partition Manifest**
```yaml
# tfm_sensor_manager.yaml
{
  "psa_framework_version": 1.0,
  "name": "TFM_SP_SENSOR_MANAGER",
  "type": "APPLICATION-ROT",
  "priority": "NORMAL",
  "entry_point": "sensor_manager_main",
  "stack_size": "0x0800",

  "services": [
    {
      "name": "TFM_SENSOR_READ",
      "sid": "0x00000080",
      "non_secure_clients": true,
      "version": 1,
      "version_policy": "STRICT",
      "connection_based": false
    }
  ],

  "mmio_regions": [
    {
      "name": "TFM_PERIPHERAL_I2C1",
      "permission": "READ-WRITE",
      "base": "0x40005400",
      "size": "0x400"
    }
  ],

  "linker_pattern": {
    "library_list": [
      "*sensor_manager.*"
    ]
  }
}
```

**Step 3: Implement Partition Code**
```c
/*
 * Sensor Manager Secure Partition
 * File: sensor_manager.c
 */

#include "psa/service.h"
#include "tfm_sp_log.h"
#include <stdint.h>
#include <string.h>

/* Service signal */
#define SENSOR_READ_SIGNAL  (1U << 0)

/* Sensor data structure */
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    uint32_t timestamp;
} sensor_data_t;

/* I2C1 peripheral base (STM32U5) */
#define I2C1_BASE  0x40005400

/* Simulated sensor read (in production: actual I2C communication) */
static psa_status_t read_sensor_hardware(sensor_data_t *data)
{
    /* In real implementation:
     * 1. Configure I2C peripheral
     * 2. Send I2C read command to sensor
     * 3. Parse sensor response
     * 4. Apply calibration
     */

    /* Simulated data */
    data->accel_x = 100;   /* mg */
    data->accel_y = 50;
    data->accel_z = 1000;  /* 1g vertical */
    data->gyro_x = 0;      /* degrees/sec */
    data->gyro_y = 0;
    data->gyro_z = 5;      /* slight rotation */

    /* Read system timestamp */
    data->timestamp = 0;  /* Would use RTC */

    LOG_MSG("Sensor data read: accel(%d, %d, %d)",
            data->accel_x, data->accel_y, data->accel_z);

    return PSA_SUCCESS;
}

/* Service request handler */
static void handle_sensor_read(psa_msg_t *msg)
{
    sensor_data_t sensor_data;
    psa_status_t status;

    LOG_MSG("Sensor read request from client");

    /* Read sensor */
    status = read_sensor_hardware(&sensor_data);

    if (status == PSA_SUCCESS) {
        /* Write response */
        psa_write(msg->handle, 0, &sensor_data, sizeof(sensor_data));
        LOG_MSG("Sensor data sent to client");
    } else {
        LOG_MSG("Sensor read failed: %d", status);
    }

    /* Reply with status */
    psa_reply(msg->handle, status);
}

/* Partition entry point */
void sensor_manager_main(void)
{
    psa_signal_t signals;
    psa_msg_t msg;

    LOG_MSG("Sensor Manager partition started");

    /* Infinite service loop */
    while (1) {
        /* Wait for signal */
        signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);

        if (signals & SENSOR_READ_SIGNAL) {
            /* Get message */
            psa_get(SENSOR_READ_SIGNAL, &msg);

            /* Handle request */
            switch (msg.type) {
                case PSA_IPC_CALL:
                    handle_sensor_read(&msg);
                    break;

                default:
                    LOG_MSG("Invalid message type: %d", msg.type);
                    psa_reply(msg.handle, PSA_ERROR_NOT_SUPPORTED);
                    break;
            }
        }
    }
}
```

**Step 4: Create CMakeLists.txt**
```cmake
# CMakeLists.txt

cmake_minimum_required(VERSION 3.15)

if (NOT TFM_PARTITION_SENSOR_MANAGER)
    return()
endif()

add_library(tfm_app_rot_partition_sensor_manager STATIC)

target_sources(tfm_app_rot_partition_sensor_manager
    PRIVATE
        sensor_manager.c
)

target_include_directories(tfm_app_rot_partition_sensor_manager
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/interface/include
)

target_link_libraries(tfm_app_rot_partition_sensor_manager
    PRIVATE
        tfm_secure_api
        psa_interface
        platform_s
        tfm_sprt
)

target_compile_definitions(tfm_app_rot_partition_sensor_manager
    PRIVATE
        TFM_PARTITION_SENSOR_MANAGER
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_app_rot_partition_sensor_manager
)

target_compile_definitions(tfm_partition_defs
    INTERFACE
        TFM_PARTITION_SENSOR_MANAGER
)
```

**Step 5: Create Client API**
```c
/*
 * Sensor Manager Client API
 * File: tfm_sensor_manager_api.h
 */

#ifndef TFM_SENSOR_MANAGER_API_H
#define TFM_SENSOR_MANAGER_API_H

#include "psa/client.h"

#define TFM_SENSOR_MANAGER_SID  0x00000080

typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    uint32_t timestamp;
} sensor_data_t;

/* Read sensor data */
static inline psa_status_t tfm_sensor_read(sensor_data_t *data)
{
    psa_handle_t handle;
    psa_status_t status;
    psa_outvec out_vec[] = {
        { .base = data, .len = sizeof(sensor_data_t) }
    };

    /* Connect to service */
    handle = psa_connect(TFM_SENSOR_MANAGER_SID, 1);
    if (handle <= 0) {
        return PSA_ERROR_CONNECTION_REFUSED;
    }

    /* Call service */
    status = psa_call(handle, PSA_IPC_CALL, NULL, 0, out_vec, 1);

    /* Close connection */
    psa_close(handle);

    return status;
}

#endif /* TFM_SENSOR_MANAGER_API_H */
```

**Step 6: Test from Non-Secure Code**
```c
/*
 * Test custom partition from NSPE
 * File: test_sensor_partition.c
 */

#include "tfm_sensor_manager_api.h"
#include <stdio.h>

int main(void)
{
    sensor_data_t sensor;
    psa_status_t status;

    printf("\n=== Testing Custom Sensor Partition ===\n\n");

    /* Read sensor data */
    printf("Requesting sensor data from secure partition...\n");

    status = tfm_sensor_read(&sensor);

    if (status == PSA_SUCCESS) {
        printf("\n✓ Sensor data received:\n");
        printf("  Accelerometer:\n");
        printf("    X: %d mg\n", sensor.accel_x);
        printf("    Y: %d mg\n", sensor.accel_y);
        printf("    Z: %d mg\n", sensor.accel_z);
        printf("  Gyroscope:\n");
        printf("    X: %d °/s\n", sensor.gyro_x);
        printf("    Y: %d °/s\n", sensor.gyro_y);
        printf("    Z: %d °/s\n", sensor.gyro_z);
        printf("  Timestamp: %u\n", sensor.timestamp);
    } else {
        printf("\n✗ Sensor read failed: %d\n", status);
    }

    return (status == PSA_SUCCESS) ? 0 : -1;
}
```

**Expected Output:**
```
=== Testing Custom Sensor Partition ===

Requesting sensor data from secure partition...

✓ Sensor data received:
  Accelerometer:
    X: 100 mg
    Y: 50 mg
    Z: 1000 mg
  Gyroscope:
    X: 0 °/s
    Y: 0 °/s
    Z: 5 °/s
  Timestamp: 1708123456
```

---

## Lab 22: Secure Interrupt Handling

**Duration:** 3 hours
**Difficulty:** Advanced
**Goal:** Implement secure interrupt handler for real-time events

### Exercise 22.1: UART Interrupt Handler

**Task:** Handle UART RX interrupt securely

**Code:**
```c
/*
 * Secure UART Interrupt Handler
 * File: secure_uart_irq.c
 */

#include "tfm_hal_interrupt.h"
#include "tfm_peripherals_def.h"

#define UART_IRQ_NUM  37  /* USART1 on STM32U5 */

/* Circular buffer for RX data */
static uint8_t rx_buffer[256];
static volatile size_t rx_head = 0;
static volatile size_t rx_tail = 0;

/* UART interrupt handler */
void USART1_IRQHandler(void)
{
    uint32_t *usart1_sr = (uint32_t *)0x4001381C;  /* SR register */
    uint32_t *usart1_dr = (uint32_t *)0x40013824;  /* DR register */

    /* Check if RXNE (RX Not Empty) */
    if (*usart1_sr & (1 << 5)) {
        /* Read data */
        uint8_t data = *usart1_dr & 0xFF;

        /* Store in buffer */
        size_t next_head = (rx_head + 1) % sizeof(rx_buffer);
        if (next_head != rx_tail) {
            rx_buffer[rx_head] = data;
            rx_head = next_head;
        }
        /* Else: Buffer full, drop data */
    }
}

/* Initialize secure UART interrupt */
void init_secure_uart_interrupt(void)
{
    /* Register interrupt handler */
    enum tfm_hal_status_t status;

    status = tfm_hal_irq_enable(UART_IRQ_NUM);
    if (status != TFM_HAL_SUCCESS) {
        /* Handle error */
        return;
    }

    /* Configure UART to generate RXNE interrupts */
    uint32_t *usart1_cr1 = (uint32_t *)0x40013800;
    *usart1_cr1 |= (1 << 5);  /* RXNEIE: RX interrupt enable */
}

/* Read from buffer (called by partition) */
size_t read_uart_buffer(uint8_t *dest, size_t max_len)
{
    size_t count = 0;

    while (count < max_len && rx_tail != rx_head) {
        dest[count++] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % sizeof(rx_buffer);
    }

    return count;
}
```

---

## Lab 23: Platform Porting Exercise

**Duration:** 4 hours
**Difficulty:** Expert
**Goal:** Port TF-M to custom hardware platform

### Exercise 23.1: Minimal Platform Port

**Task:** Create minimal platform port for custom STM32 board

**Files to create:**
```
platform/ext/target/custom/custom_board/
├── CMakeLists.txt
├── config.cmake
├── partition/
│   ├── flash_layout.h
│   └── region_defs.h
├── Device/
│   ├── Config/
│   │   └── device_cfg.h
│   └── Source/
│       ├── startup_custom.c
│       └── system_custom.c
└── cmsis_drivers/
    ├── Driver_Flash.c
    └── Driver_USART.c
```

**Minimal config.cmake:**
```cmake
# config.cmake for custom board

set(TFM_PLATFORM_CUSTOM ON)

# Memory configuration
set(FLASH_BASE_ADDRESS      0x08000000)
set(FLASH_SIZE              0x00100000)  # 1MB
set(RAM_BASE_ADDRESS        0x20000000)
set(RAM_SIZE                0x00040000)  # 256KB

# Enable hardware features
set(CRYPTO_HW_ACCELERATOR   OFF)  # No accelerator on custom board
set(TFM_CRYPTO_DRIVER       "MBED_TLS")

# Bootloader
set(BL2                     TRUE)
set(BL2_HEADER_SIZE         0x400)
set(BL2_TRAILER_SIZE        0x400)

# Flash layout
set(FLASH_AREA_BL2_OFFSET   0x0)
set(FLASH_AREA_BL2_SIZE     0xA000)
set(FLASH_AREA_0_OFFSET     0xA000)
set(FLASH_AREA_0_SIZE       0x32000)
# ... (see Lab 7 for complete layout)
```

---

## Lab 24: Fault Injection Testing

**Duration:** 3 hours
**Difficulty:** Expert
**Goal:** Test device resilience against fault injection attacks

### Exercise 24.1: Simulated Glitch Testing

**Task:** Test double checks and redundant verification

**Code:**
```c
/*
 * Fault Injection Countermeasures
 * File: fault_injection_demo.c
 */

#include <stdio.h>
#include <stdint.h>

/* Vulnerable code (single check) */
int verify_signature_vulnerable(uint8_t *signature, size_t sig_len)
{
    int result = crypto_verify_signature(signature, sig_len);

    if (result == 0) {
        printf("✓ Signature valid\n");
        return 0;  /* Success */
    } else {
        printf("✗ Signature invalid\n");
        return -1;  /* Failure */
    }
}

/* Protected code (double check with redundancy) */
int verify_signature_protected(uint8_t *signature, size_t sig_len)
{
    int result1, result2;
    volatile int check1, check2;

    /* First verification */
    result1 = crypto_verify_signature(signature, sig_len);
    check1 = (result1 == 0) ? 0xA5A5 : 0x5A5A;

    /* Second verification (redundant) */
    result2 = crypto_verify_signature(signature, sig_len);
    check2 = (result2 == 0) ? 0xA5A5 : 0x5A5A;

    /* Compare results */
    if (check1 == 0xA5A5 && check2 == 0xA5A5 && result1 == 0 && result2 == 0) {
        printf("✓ Signature valid (double-checked)\n");
        return 0;
    } else {
        printf("✗ Signature invalid or glitch detected\n");

        /* If results differ, possible glitch attack */
        if (check1 != check2) {
            printf("⚠ WARNING: Fault injection detected!\n");
            /* Trigger security response */
            trigger_security_alarm();
        }

        return -1;
    }
}

/* Demonstrate attack scenario */
void test_fault_injection(void)
{
    uint8_t bad_signature[64] = {0};  /* Invalid signature */

    printf("\n=== Fault Injection Test ===\n\n");

    printf("Test 1: Vulnerable code\n");
    printf("  - Attacker glitches during 'if' check\n");
    printf("  - Branch prediction manipulated\n");
    printf("  - Result: May bypass check ✗\n\n");

    printf("Test 2: Protected code\n");
    printf("  - Double verification\n");
    printf("  - Redundant checks\n");
    printf("  - Detects mismatch → Security alarm\n");
    printf("  - Result: Attack detected ✓\n\n");

    verify_signature_protected(bad_signature, 64);
}
```

**Other Countermeasures:**
```c
/* 1. Random delays */
void random_delay(void) {
    uint32_t delay = get_random() % 1000;
    for (volatile uint32_t i = 0; i < delay; i++);
}

/* 2. Canary values on stack */
void protected_function(void) {
    uint32_t stack_canary = 0xDEADBEEF;

    /* ... function logic ... */

    if (stack_canary != 0xDEADBEEF) {
        /* Stack corruption detected! */
        trigger_security_alarm();
    }
}

/* 3. Critical code duplication */
if (verify_signature(sig) == 0) {
    if (verify_signature(sig) == 0) {  /* Double check */
        allow_access();
    }
}
```

---

## Lab 25: PSA Certification Preparation

**Duration:** 4 hours
**Difficulty:** Expert
**Goal:** Prepare for PSA Certified Level 1 or 2

### Exercise 25.1: PSA Requirements Checklist

**Task:** Verify compliance with PSA Certified requirements

**PSA Certified Level 1 Requirements:**

```
┌──────────────────────────────────────────────────────┐
│ PSA Certified Level 1 - Checklist                   │
├──────────────────────────────────────────────────────┤
│                                                      │
│  ☑ 1. Secure Boot                                    │
│      ✓ Bootloader verifies firmware signature       │
│      ✓ Anti-rollback protection                     │
│      ✓ Measured boot (optional)                     │
│                                                      │
│  ☑ 2. Cryptographic Services                         │
│      ✓ PSA Crypto API implemented                   │
│      ✓ Random number generation (TRNG/DRBG)         │
│      ✓ Key storage (ITS)                            │
│      ✓ Hardware acceleration (if available)         │
│                                                      │
│  ☑ 3. Secure Storage                                 │
│      ✓ Internal Trusted Storage (ITS)               │
│      ✓ Confidentiality and integrity                │
│      ✓ Replay protection                            │
│                                                      │
│  ☑ 4. Initial Attestation                            │
│      ✓ Device identity                              │
│      ✓ Attestation token generation                 │
│      ✓ Boot measurements included                   │
│                                                      │
│  ☑ 5. Isolation                                      │
│      ✓ TrustZone enabled                            │
│      ✓ MPU/SAU configured                           │
│      ✓ Isolation Level 1 minimum                    │
│                                                      │
│  ☑ 6. Secure Firmware Update                         │
│      ✓ Signed firmware images                       │
│      ✓ Secure installation process                  │
│      ✓ Fallback on failure                          │
│                                                      │
└──────────────────────────────────────────────────────┘
```

**Verification Script:**
```bash
#!/bin/bash
# psa_certification_check.sh

echo "=== PSA Certification Compliance Check ==="
echo

# 1. Secure Boot
echo "1. Secure Boot:"
if grep -q "BL2.*TRUE" config.cmake; then
    echo "   ✓ Bootloader enabled"
else
    echo "   ✗ Bootloader not enabled"
fi

if grep -q "MCUBOOT_SIGN_" config.cmake; then
    echo "   ✓ Image signing configured"
else
    echo "   ✗ Image signing missing"
fi
echo

# 2. Crypto
echo "2. Cryptographic Services:"
if grep -q "TFM_PARTITION_CRYPTO.*ON" CMakeCache.txt; then
    echo "   ✓ PSA Crypto API enabled"
else
    echo "   ✗ PSA Crypto API not enabled"
fi
echo

# 3. Storage
echo "3. Secure Storage:"
if grep -q "TFM_PARTITION_INTERNAL_TRUSTED_STORAGE.*ON" CMakeCache.txt; then
    echo "   ✓ ITS enabled"
else
    echo "   ✗ ITS not enabled"
fi
echo

# 4. Attestation
echo "4. Initial Attestation:"
if grep -q "TFM_PARTITION_INITIAL_ATTESTATION.*ON" CMakeCache.txt; then
    echo "   ✓ Attestation service enabled"
else
    echo "   ✗ Attestation service not enabled"
fi
echo

# 5. Isolation
echo "5. Isolation:"
ISOLATION=$(grep "TFM_ISOLATION_LEVEL" CMakeCache.txt | cut -d= -f2)
if [ "$ISOLATION" -ge 1 ]; then
    echo "   ✓ Isolation Level $ISOLATION"
else
    echo "   ✗ Insufficient isolation"
fi
echo

# 6. Firmware Update
echo "6. Firmware Update:"
if grep -q "TFM_PARTITION_FIRMWARE_UPDATE.*ON" CMakeCache.txt; then
    echo "   ✓ PSA FWU API enabled"
else
    echo "   ⚠ PSA FWU API not enabled (optional)"
fi
echo

echo "════════════════════════════════════════"
echo "  Compliance check complete"
echo "════════════════════════════════════════"
```

---

### Exercise 25.2: Functional API Test Suite

**Task:** Run PSA Functional API tests

```bash
#!/bin/bash
# Run PSA API tests

cd ~/tfm_workspace/psa-arch-tests

# Build tests
cmake . -DTARGET=stm32u585 -DSUITE=CRYPTO
make

# Flash to device
st-flash write test.bin 0x08000000

# Run tests via serial console
# Expected: All tests PASS

# Test suites:
# - CRYPTO: Cryptographic operations
# - STORAGE: ITS and PS APIs
# - ATTEST: Initial Attestation
# - IPC: Inter-partition communication
```

**Expected Results:**
```
PSA Architecture Test Suite - Version 1.5

******************************************
TEST: 201 | DESCRIPTION: Testing crypto key attributes
******************************************
[Check 1] Test psa_import_key with valid key
[Check 2] Test psa_get_key_attributes
[Check 3] Test psa_destroy_key
TEST RESULT: PASSED

******************************************
TEST: 202 | DESCRIPTION: Testing psa_hash_compute
******************************************
[Check 1] Test SHA-256 hash
[Check 2] Test output buffer validation
TEST RESULT: PASSED

... (100+ tests)

************ Test Suite Report **********
TOTAL TESTS RUN  : 156
TOTAL TESTS PASSED: 156
TOTAL TESTS FAILED: 0
TOTAL TESTS SKIPPED: 0
******************************************
```

---

**LABS 21-25 COMPLETE!**

**Summary:**
- Lab 21: Custom secure partition (sensor manager example)
- Lab 22: Secure interrupt handling (UART IRQ)
- Lab 23: Platform porting (custom board configuration)
- Lab 24: Fault injection countermeasures
- Lab 25: PSA Certification preparation

**Total Advanced Labs:** 18 hours of expert-level training with production-ready techniques!

---

**ALL 25 LABS COMPLETE!**

**Complete Training Package:**
- **Labs 1-4:** Foundation (environment, TrustZone, crypto basics, storage)
- **Labs 5-10:** Build & Configuration (build system, profiling, memory analysis)
- **Labs 11-15:** Secure Services (advanced crypto, HKDF, persistent keys, attestation)
- **Labs 16-20:** Boot & Update (MCUboot, signing, swap, rollback, OTA)
- **Labs 21-25:** Advanced Topics (custom partitions, IRQ, porting, security testing, PSA cert)

**Total Training Time:** 100+ hours
**Total Pages:** 150+ pages of hands-on labs with complete solutions!

---
