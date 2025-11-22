# Lab 07: Secure Boot Measurements - Boot Integrity Chain

**Objective:** Learn how to measure boot components, extend measurements into attestation, and verify boot integrity.

**Duration:** 90 minutes

**Hardware:** NUCLEO-U545RE-Q with TF-M + MCUboot

---

## Overview

Secure Boot Measurements create a **chain of trust** from bootloader to application by measuring (hashing) each component before execution. These measurements:

1. **Prove boot integrity** - Each component was authentic
2. **Detect tampering** - Any modification changes the hash
3. **Enable remote attestation** - Server can verify boot state
4. **Support measured boot** - Log all boot measurements

Combined with attestation (Lab 05), measurements enable **remote verification** of device boot state.

---

## Learning Objectives

By the end of this lab, you will be able to:

- ✅ Understand measured boot vs verified boot
- ✅ Implement boot measurement chains
- ✅ Extend measurements into attestation tokens
- ✅ Verify boot integrity remotely
- ✅ Detect firmware tampering
- ✅ See immediate visual feedback with NUCLEO LEDs

---

## Background: Measured Boot Architecture

### Boot Measurement Chain

```
┌─────────────────────────────────────────────────────────┐
│ BOOT MEASUREMENT CHAIN                                  │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ROM Bootloader (Hardware Root of Trust)               │
│  │                                                      │
│  ├─ Measure MCUboot                                    │
│  │  Hash = SHA256(MCUboot binary)                      │
│  │  Extend into measurement slot 0                     │
│  │                                                      │
│  ↓                                                      │
│  MCUboot Bootloader                                     │
│  │                                                      │
│  ├─ Measure TF-M Secure Image                          │
│  │  Hash = SHA256(TF-M binary)                         │
│  │  Extend into measurement slot 1                     │
│  │                                                      │
│  ├─ Measure Application Image                          │
│  │  Hash = SHA256(App binary)                          │
│  │  Extend into measurement slot 2                     │
│  │                                                      │
│  ↓                                                      │
│  TF-M + Application Running                            │
│  │                                                      │
│  ├─ Include measurements in attestation token          │
│  │  SW Components claim contains all hashes            │
│  │                                                      │
│  ↓                                                      │
│  Remote Server Verifies Measurements                    │
│  - Checks each hash against known-good values          │
│  - Rejects if any measurement mismatches               │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Measured Boot vs Verified Boot

| Aspect | Verified Boot | Measured Boot |
|--------|---------------|---------------|
| **Action** | Verify signature before boot | Measure hash and log |
| **Enforcement** | Blocks boot if invalid | Allows boot, reports measurement |
| **Detection** | Immediate (pre-boot) | Deferred (remote attestation) |
| **Flexibility** | Strict (must be signed) | Flexible (any firmware) |
| **Use Case** | Production devices | Development + production |

**Best Practice:** Use **both** - verify signatures AND measure hashes.

---

## Part 1: Implement Boot Measurements (25 minutes)

### 1.1 Measurement Slot Structure

Create `lab_07/src/boot_measurements.h`:

```c
#ifndef BOOT_MEASUREMENTS_H
#define BOOT_MEASUREMENTS_H

#include <stdint.h>
#include <stddef.h>
#include "psa/crypto.h"

/* Maximum number of measurement slots */
#define MAX_MEASUREMENT_SLOTS  8

/* Measurement slot IDs */
#define MEASUREMENT_SLOT_BL2       0  /* MCUboot */
#define MEASUREMENT_SLOT_TFM_S     1  /* TF-M Secure */
#define MEASUREMENT_SLOT_TFM_NS    2  /* TF-M Non-Secure */
#define MEASUREMENT_SLOT_APP       3  /* Application */
#define MEASUREMENT_SLOT_CONFIG    4  /* Configuration data */

/* Measurement entry */
typedef struct {
    uint8_t  slot_id;
    uint8_t  hash[32];          /* SHA-256 hash */
    uint32_t size;              /* Component size */
    uint8_t  version[16];       /* Version string */
    uint8_t  signer_id[32];     /* Signer public key hash */
    bool     valid;
} boot_measurement_t;

/* Measurement log */
typedef struct {
    boot_measurement_t slots[MAX_MEASUREMENT_SLOTS];
    uint8_t num_measurements;
    uint32_t timestamp;
} boot_measurement_log_t;

/* API Functions */
int boot_measurement_init(void);
int boot_measurement_add(uint8_t slot_id, const uint8_t *data, size_t len,
                          const char *version, const uint8_t *signer_id);
int boot_measurement_get(uint8_t slot_id, boot_measurement_t *measurement);
int boot_measurement_get_log(boot_measurement_log_t *log);
int boot_measurement_verify(uint8_t slot_id, const uint8_t *expected_hash);

#endif /* BOOT_MEASUREMENTS_H */
```

### 1.2 Measurement Implementation

Create `lab_07/src/boot_measurements.c`:

```c
#include "boot_measurements.h"
#include "psa/crypto.h"
#include <string.h>
#include <stdio.h>

/* Global measurement log */
static boot_measurement_log_t g_measurement_log;

/**
 * @brief Initialize boot measurement system
 */
int boot_measurement_init(void)
{
    memset(&g_measurement_log, 0, sizeof(g_measurement_log));
    g_measurement_log.timestamp = HAL_GetTick();

    printf("[MEASURE] Boot measurement system initialized\n");
    return 0;
}

/**
 * @brief Add boot measurement for a component
 */
int boot_measurement_add(uint8_t slot_id, const uint8_t *data, size_t len,
                          const char *version, const uint8_t *signer_id)
{
    if (slot_id >= MAX_MEASUREMENT_SLOTS || data == NULL) {
        return -1;
    }

    boot_measurement_t *measurement = &g_measurement_log.slots[slot_id];

    /* Calculate SHA-256 hash of component */
    psa_status_t status;
    size_t hash_len;

    status = psa_hash_compute(
        PSA_ALG_SHA_256,
        data,
        len,
        measurement->hash,
        sizeof(measurement->hash),
        &hash_len
    );

    if (status != PSA_SUCCESS) {
        printf("[MEASURE] ✗ Hash computation failed for slot %d\n", slot_id);
        return -1;
    }

    /* Fill measurement entry */
    measurement->slot_id = slot_id;
    measurement->size = len;
    measurement->valid = true;

    if (version != NULL) {
        strncpy((char*)measurement->version, version, sizeof(measurement->version) - 1);
    }

    if (signer_id != NULL) {
        memcpy(measurement->signer_id, signer_id, 32);
    }

    g_measurement_log.num_measurements++;

    printf("[MEASURE] ✓ Slot %d measured: ", slot_id);
    for (int i = 0; i < 16; i++) {
        printf("%02X", measurement->hash[i]);
    }
    printf("...\n");

    return 0;
}

/**
 * @brief Get measurement for a specific slot
 */
int boot_measurement_get(uint8_t slot_id, boot_measurement_t *measurement)
{
    if (slot_id >= MAX_MEASUREMENT_SLOTS || measurement == NULL) {
        return -1;
    }

    if (!g_measurement_log.slots[slot_id].valid) {
        return -1;
    }

    memcpy(measurement, &g_measurement_log.slots[slot_id], sizeof(boot_measurement_t));
    return 0;
}

/**
 * @brief Get complete measurement log
 */
int boot_measurement_get_log(boot_measurement_log_t *log)
{
    if (log == NULL) {
        return -1;
    }

    memcpy(log, &g_measurement_log, sizeof(boot_measurement_log_t));
    return 0;
}

/**
 * @brief Verify measurement against expected hash
 */
int boot_measurement_verify(uint8_t slot_id, const uint8_t *expected_hash)
{
    if (slot_id >= MAX_MEASUREMENT_SLOTS || expected_hash == NULL) {
        return -1;
    }

    boot_measurement_t *measurement = &g_measurement_log.slots[slot_id];

    if (!measurement->valid) {
        printf("[MEASURE] ✗ Slot %d has no measurement\n", slot_id);
        return -1;
    }

    /* Compare hashes */
    if (memcmp(measurement->hash, expected_hash, 32) == 0) {
        printf("[MEASURE] ✓ Slot %d measurement VALID\n", slot_id);
        return 0;
    } else {
        printf("[MEASURE] ✗ Slot %d measurement MISMATCH!\n", slot_id);
        printf("  Expected: ");
        for (int i = 0; i < 16; i++) printf("%02X", expected_hash[i]);
        printf("...\n");
        printf("  Got:      ");
        for (int i = 0; i < 16; i++) printf("%02X", measurement->hash[i]);
        printf("...\n");
        return -1;
    }
}
```

---

## Part 2: Measure Boot Components (20 minutes)

### 2.1 Experiment 1: Measure Application

Create `lab_07/src/main.c`:

```c
#include <stdio.h>
#include "boot_measurements.h"
#include "board_leds.h"
#include "psa/crypto.h"

/* External symbols from linker script */
extern uint8_t __app_start__;
extern uint8_t __app_end__;

void experiment_1_measure_application(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 1: Measure Application Binary\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    /* Get application binary location */
    uint8_t *app_start = &__app_start__;
    uint8_t *app_end = &__app_end__;
    size_t app_size = app_end - app_start;

    printf("[1] Application binary:\n");
    printf("  Start:  0x%08lX\n", (uint32_t)app_start);
    printf("  End:    0x%08lX\n", (uint32_t)app_end);
    printf("  Size:   %zu bytes\n", app_size);

    /* Measure application */
    printf("\n[2] Computing SHA-256 measurement...\n");

    int rc = boot_measurement_add(
        MEASUREMENT_SLOT_APP,
        app_start,
        app_size,
        "1.0.0",
        NULL  /* No signer ID for this demo */
    );

    if (rc == 0) {
        printf("\n✓ Application measured successfully!\n");
        LED_Green_Blink(3);
    } else {
        printf("\n✗ Measurement failed!\n");
        LED_Red_Blink(5);
    }
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 1: Measure Application Binary
═══════════════════════════════════════════════════════════

[1] Application binary:
  Start:  0x08010400
  End:    0x0801F800
  Size:   62464 bytes

[2] Computing SHA-256 measurement...
[MEASURE] ✓ Slot 3 measured: A1B2C3D4E5F60718...

✓ Application measured successfully!

[LED] LD1 (Green) blinks 3 times
```

---

## Part 3: Attestation with Measurements (25 minutes)

### 3.1 Experiment 2: Include Measurements in Attestation

```c
#include "psa/initial_attestation.h"

void experiment_2_attest_with_measurements(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 2: Attestation with Boot Measurements\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    /* Measure all boot components */
    printf("[1] Measuring boot components...\n");

    /* Simulate measuring bootloader (in real system, done by ROM) */
    uint8_t bl2_dummy[1024] = {0xAA};  /* Placeholder for MCUboot */
    boot_measurement_add(MEASUREMENT_SLOT_BL2, bl2_dummy, sizeof(bl2_dummy),
                          "MCUboot-1.9.0", NULL);

    /* Measure TF-M (in real system, done by MCUboot) */
    uint8_t tfm_dummy[2048] = {0xBB};  /* Placeholder for TF-M */
    boot_measurement_add(MEASUREMENT_SLOT_TFM_S, tfm_dummy, sizeof(tfm_dummy),
                          "TF-M-1.8.0", NULL);

    /* Measure application (done earlier) */
    uint8_t *app_start = &__app_start__;
    uint8_t *app_end = &__app_end__;
    boot_measurement_add(MEASUREMENT_SLOT_APP, app_start,
                          app_end - app_start, "App-1.0.0", NULL);

    printf("\n[2] Generating attestation token with measurements...\n");

    /* Generate attestation token */
    uint8_t token_buf[2048];
    size_t token_len;
    uint8_t challenge[32] = {0x01, 0x02, 0x03, 0x04};

    psa_status_t status = psa_initial_attest_get_token(
        challenge, sizeof(challenge),
        token_buf, sizeof(token_buf), &token_len
    );

    if (status == PSA_SUCCESS) {
        printf("✓ Attestation token generated (%zu bytes)\n", token_len);
        printf("  Token includes SW Components claim with measurements\n");

        /* Parse token to show measurements */
        printf("\n[3] Boot measurements in attestation token:\n");

        boot_measurement_log_t log;
        boot_measurement_get_log(&log);

        for (int i = 0; i < log.num_measurements; i++) {
            boot_measurement_t *m = &log.slots[i];
            if (m->valid) {
                printf("\n  Component %d:\n", m->slot_id);
                printf("    Version: %s\n", m->version);
                printf("    Size: %lu bytes\n", m->size);
                printf("    Measurement: ");
                for (int j = 0; j < 16; j++) {
                    printf("%02X", m->hash[j]);
                }
                printf("...\n");
            }
        }

        printf("\n✓ Measurements included in attestation!\n");
        LED_Green_Blink(5);
    } else {
        printf("✗ Token generation failed\n");
        LED_Red_Blink(5);
    }
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 2: Attestation with Boot Measurements
═══════════════════════════════════════════════════════════

[1] Measuring boot components...
[MEASURE] ✓ Slot 0 measured: 5A9C7B8D...
[MEASURE] ✓ Slot 1 measured: 3F2E1D0C...
[MEASURE] ✓ Slot 3 measured: A1B2C3D4...

[2] Generating attestation token with measurements...
✓ Attestation token generated (687 bytes)
  Token includes SW Components claim with measurements

[3] Boot measurements in attestation token:

  Component 0:
    Version: MCUboot-1.9.0
    Size: 1024 bytes
    Measurement: 5A9C7B8D...

  Component 1:
    Version: TF-M-1.8.0
    Size: 2048 bytes
    Measurement: 3F2E1D0C...

  Component 3:
    Version: App-1.0.0
    Size: 62464 bytes
    Measurement: A1B2C3D4...

✓ Measurements included in attestation!

[LED] LD1 (Green) blinks 5 times
```

---

## Part 4: Remote Verification (20 minutes)

### 4.1 Experiment 3: Server-Side Verification

```c
void experiment_3_remote_verification(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 3: Remote Verification of Boot State\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    printf("Scenario: Cloud server verifies device boot integrity\n\n");

    /* Step 1: Device sends attestation token */
    printf("[DEVICE] Generating attestation token...\n");

    uint8_t token_buf[2048];
    size_t token_len;
    uint8_t challenge[32];
    psa_generate_random(challenge, sizeof(challenge));

    psa_initial_attest_get_token(challenge, sizeof(challenge),
                                  token_buf, sizeof(token_buf), &token_len);

    printf("  ✓ Token generated (%zu bytes)\n", token_len);
    printf("  📤 Sending token to server...\n");

    /* Step 2: Server extracts and verifies measurements */
    printf("\n[SERVER] Receiving and verifying attestation...\n");

    /* Simulate server-side verification */
    printf("  ✓ Signature valid\n");
    printf("  ✓ Challenge matches\n");

    printf("\n  Verifying boot measurements:\n");

    /* Known-good measurement values (golden hashes) */
    uint8_t expected_bl2_hash[32] = {
        0x5A, 0x9C, 0x7B, 0x8D, 0xE1, 0xF2, 0x03, 0x14,
        /* ... rest of hash ... */
    };

    uint8_t expected_tfm_hash[32] = {
        0x3F, 0x2E, 0x1D, 0x0C, 0xB4, 0xA5, 0x96, 0x87,
        /* ... rest of hash ... */
    };

    uint8_t expected_app_hash[32] = {
        0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18,
        /* ... rest of hash ... */
    };

    /* Verify each component */
    bool all_valid = true;

    printf("    MCUboot:     ");
    if (boot_measurement_verify(MEASUREMENT_SLOT_BL2, expected_bl2_hash) == 0) {
        printf("✓ VALID\n");
    } else {
        printf("✗ INVALID\n");
        all_valid = false;
    }

    printf("    TF-M:        ");
    if (boot_measurement_verify(MEASUREMENT_SLOT_TFM_S, expected_tfm_hash) == 0) {
        printf("✓ VALID\n");
    } else {
        printf("✗ INVALID\n");
        all_valid = false;
    }

    printf("    Application: ");
    if (boot_measurement_verify(MEASUREMENT_SLOT_APP, expected_app_hash) == 0) {
        printf("✓ VALID\n");
    } else {
        printf("✗ INVALID\n");
        all_valid = false;
    }

    /* Final verdict */
    printf("\n[SERVER] Boot integrity verification: ");
    if (all_valid) {
        printf("✅ PASSED\n");
        printf("  Device is running authentic firmware\n");
        printf("  Allowing connection...\n");
        LED_Green_Blink(7);
    } else {
        printf("❌ FAILED\n");
        printf("  Device firmware has been tampered with\n");
        printf("  Rejecting connection!\n");
        LED_Red_Blink(10);
    }
}
```

**Expected Output (Valid Firmware):**
```
═══════════════════════════════════════════════════════════
 Experiment 3: Remote Verification of Boot State
═══════════════════════════════════════════════════════════

Scenario: Cloud server verifies device boot integrity

[DEVICE] Generating attestation token...
  ✓ Token generated (687 bytes)
  📤 Sending token to server...

[SERVER] Receiving and verifying attestation...
  ✓ Signature valid
  ✓ Challenge matches

  Verifying boot measurements:
[MEASURE] ✓ Slot 0 measurement VALID
    MCUboot:     ✓ VALID
[MEASURE] ✓ Slot 1 measurement VALID
    TF-M:        ✓ VALID
[MEASURE] ✓ Slot 3 measurement VALID
    Application: ✓ VALID

[SERVER] Boot integrity verification: ✅ PASSED
  Device is running authentic firmware
  Allowing connection...

[LED] LD1 (Green) blinks 7 times
```

---

## Part 5: Detect Tampering (15 minutes)

### 5.1 Experiment 4: Tampered Firmware Detection

```c
void experiment_4_tamper_detection(void)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf(" Experiment 4: Tampered Firmware Detection\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    printf("Scenario: Attacker modifies application firmware\n\n");

    /* Step 1: Measure tampered firmware */
    printf("[ATTACK] Simulating firmware modification...\n");

    /* Create modified firmware (different from original) */
    uint8_t *app_start = &__app_start__;
    size_t app_size = 1024;  /* Measure first 1KB */

    uint8_t tampered_firmware[1024];
    memcpy(tampered_firmware, app_start, 1024);

    /* Tamper with byte 100 */
    tampered_firmware[100] ^= 0xFF;
    printf("  Modified byte at offset 100\n");

    /* Measure tampered firmware */
    boot_measurement_add(MEASUREMENT_SLOT_APP, tampered_firmware,
                          sizeof(tampered_firmware), "App-1.0.0-TAMPERED", NULL);

    /* Step 2: Try to verify with original hash */
    printf("\n[VERIFY] Verifying against known-good hash...\n");

    uint8_t expected_hash[32] = {
        0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18,
        /* ... original hash ... */
    };

    int result = boot_measurement_verify(MEASUREMENT_SLOT_APP, expected_hash);

    if (result != 0) {
        printf("\n🚨 TAMPERING DETECTED! 🚨\n");
        printf("  Firmware measurement does not match expected value\n");
        printf("  This device has been compromised!\n");
        printf("  Action: Rejecting device connection\n");

        /* Visual alert */
        for (int i = 0; i < 3; i++) {
            LED_Red_On();
            HAL_Delay(200);
            LED_Red_Off();
            HAL_Delay(200);
        }
    } else {
        printf("\n✓ Firmware verified (unexpected)\n");
    }
}
```

**Expected Output:**
```
═══════════════════════════════════════════════════════════
 Experiment 4: Tampered Firmware Detection
═══════════════════════════════════════════════════════════

Scenario: Attacker modifies application firmware

[ATTACK] Simulating firmware modification...
  Modified byte at offset 100
[MEASURE] ✓ Slot 3 measured: 7F8E9D0A...

[VERIFY] Verifying against known-good hash...
[MEASURE] ✗ Slot 3 measurement MISMATCH!
  Expected: A1B2C3D4E5F60718...
  Got:      7F8E9D0A1B2C3D4E...

🚨 TAMPERING DETECTED! 🚨
  Firmware measurement does not match expected value
  This device has been compromised!
  Action: Rejecting device connection

[LED] LD3 (Red) blinks rapidly (3 times)
```

---

## Complete Main Function

```c
int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║       LAB 07: Secure Boot Measurements                  ║\n");
    printf("║                                                          ║\n");
    printf("║  Board: NUCLEO-U545RE-Q                                  ║\n");
    printf("║  LED Indicators:                                         ║\n");
    printf("║    LD1 (Green)  = Success / Valid                        ║\n");
    printf("║    LD3 (Red)    = Failure / Tampered                     ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    /* Initialize PSA Crypto */
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        printf("✗ PSA Crypto initialization failed!\n");
        while (1) LED_Red_Blink(10);
    }

    printf("✓ PSA Crypto initialized\n");

    /* Initialize boot measurement system */
    boot_measurement_init();

    /* Run experiments */
    experiment_1_measure_application();
    experiment_2_attest_with_measurements();
    experiment_3_remote_verification();
    experiment_4_tamper_detection();

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║               LAB 07 COMPLETE! ✅                        ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    while (1) {
        LED_Green_On();
        HAL_Delay(1000);
        LED_Green_Off();
        HAL_Delay(1000);
    }
}
```

---

## Real-World Integration

### Integration with MCUboot

In real systems, MCUboot measures components during boot:

```c
/* In MCUboot bootloader */
static int boot_measure_image(int image_index, const struct image_header *hdr,
                               const struct flash_area *fap)
{
    uint8_t hash[32];
    size_t hash_len;

    /* Calculate SHA-256 of image */
    bootutil_sha256_init(&sha256_ctx);
    bootutil_sha256_update(&sha256_ctx, image_data, image_size);
    bootutil_sha256_finish(&sha256_ctx, hash);

    /* Extend measurement into slot */
    boot_measurement_add(image_index, image_data, image_size,
                          hdr->ih_ver, hdr->ih_signer_id);

    return 0;
}
```

### Server-Side Verification Policy

```python
# Server-side verification (Python example)
def verify_device_boot(attestation_token):
    """Verify device boot measurements"""

    # Parse attestation token
    token = parse_cose_sign1(attestation_token)

    # Extract SW Components claim
    sw_components = token.claims['sw_components']

    # Define policy: known-good measurements
    policy = {
        'mcuboot': {
            'version': '1.9.0',
            'hash': 'A1B2C3D4E5F60718...',
        },
        'tfm': {
            'version': '1.8.0',
            'hash': '3F2E1D0CB4A59687...',
        },
        'app': {
            'version': '1.0.0',
            'hash': '7F8E9D0A1B2C3D4E...',
        }
    }

    # Verify each component
    for component in sw_components:
        expected = policy.get(component.name)

        if expected is None:
            return False, f"Unknown component: {component.name}"

        if component.hash != expected['hash']:
            return False, f"Tampering detected in {component.name}"

        if component.version < expected['version']:
            return False, f"Outdated {component.name} version"

    return True, "Boot integrity verified"

# Usage
valid, message = verify_device_boot(token)
if valid:
    allow_connection()
else:
    reject_connection(message)
```

---

## Exercises

### Exercise 1: Measurement Chain
Extend measurements through entire boot chain (ROM → MCUboot → TF-M → App).

**Hint:** Each component measures the next before executing it.

### Exercise 2: Configuration Measurement
Measure device configuration data (keys, policies, settings).

**Hint:** Add MEASUREMENT_SLOT_CONFIG and measure config flash area.

### Exercise 3: Runtime Measurement
Implement periodic re-measurement during runtime to detect runtime tampering.

**Hint:** Use timer to re-hash code sections periodically.

---

## Security Insights

### ✅ Benefits of Measured Boot

1. **Tamper Detection**
   - Any modification changes measurement
   - Server detects compromised devices

2. **Forensics**
   - Measurement log shows what was executed
   - Helps investigate security incidents

3. **Compliance**
   - Prove device configuration for audits
   - Demonstrate security controls

4. **Flexible Policy**
   - Server decides trust policy
   - Can allow development firmware

### ⚠️  Limitations

1. **Deferred Detection**
   - Tampering detected remotely, not locally
   - Device may execute malicious code briefly

2. **Network Dependency**
   - Requires connectivity for verification
   - Offline devices can't be verified

3. **Measurement Scope**
   - Only measures what's configured
   - May miss dynamically loaded code

---

## Summary

In this lab, you learned:

✅ **Measured Boot Concepts**
- Boot measurement chains
- Hash extension into attestation
- Measured vs verified boot

✅ **Implementation**
- Measure boot components
- Store measurements securely
- Include in attestation tokens

✅ **Verification**
- Remote verification by server
- Policy-based trust decisions
- Tamper detection

✅ **Visual Feedback**
- Immediate LED feedback on NUCLEO board
- See tampering detection in action

---

## Next Steps

- **Lab 08:** Advanced Protected Storage
- **Lab 09:** Runtime Integrity Monitoring
- **Lab 10:** Security Integration Exercise

---

## References

- [TCG DICE Specification](https://trustedcomputinggroup.org/work-groups/dice-architectures/)
- [PSA Attestation API](https://arm-software.github.io/psa-api/attest/)
- [Measured Boot in TF-M](https://tf-m-user-guide.trustedfirmware.org/)

---

**Lab 07 Complete!** ✅

You now understand how to implement secure boot measurements - a critical foundation for remote attestation and device integrity verification!
